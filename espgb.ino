#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "driver/i2s.h"

// ADD these three lines right before the peanut_gb include block
// Forward-declare so peanut_gb.h can see them at include time
extern "C" uint8_t audio_read(uint16_t addr);
extern "C" void    audio_write(uint16_t addr, uint8_t val);

extern "C" {
#define PEANUT_GB_IMPLEMENTATION
#define ENABLE_SOUND 1
#include "peanut_gb.h"
}

// Include ROM manager and webpage
#include "rom_manager.h"
#include "webpage.h"

// Global variables for current game
const unsigned char* game_rom = nullptr;
uint32_t game_rom_size = 0;
int current_game = 0;

//////////////////// WIFI ////////////////////

const char* ssid     = "ESP32-GameBoy";
const char* password = "12345678";

WebServer server(80);
volatile uint8_t wifi_keys = 0;

//////////////////// DISPLAY ////////////////////

#define TFT_CS  5
#define TFT_DC  2
#define TFT_RST 4
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

//////////////////// I2S / MAX98357A ////////////////////

#define I2S_BCLK   26
#define I2S_LRC    25
#define I2S_DOUT   22
#define I2S_PORT   I2S_NUM_0
#define SAMPLE_RATE 22050

#define AUDIO_BUF_SAMPLES 1024
static int16_t  audioBuf[AUDIO_BUF_SAMPLES * 2];
static volatile int audioBufHead = 0;
static volatile int audioBufTail = 0;

//////////////////// GB APU ////////////////////
// peanut_gb calls audio_read / audio_write from the CPU R/W path.
// We store all registers and synthesise audio from them each sample.

static uint8_t apuReg[0x30]; // indexed by addr - 0xFF10

// ---- Duty table: steps HIGH out of 8 ----
static const uint8_t DUTY[4] = { 1, 2, 4, 6 }; // 12.5 / 25 / 50 / 75 %

// ---- Per-channel state ----
struct SquareCh {
  bool    on;
  uint8_t dutyIdx;
  int     vol, envVol0;
  bool    envAdd;
  int     envPer, envTimer;
  int     freqReg;
  uint32_t phase, phaseInc; // 8-step fixed-point (<<13)
  int     lenTimer;
  bool    lenOn;
};

struct WaveCh {
  bool    on, dacOn;
  int     outLevel;         // 0=mute 1=100% 2=50% 3=25%
  int     freqReg;
  uint32_t phase, phaseInc; // 32-sample fixed-point (<<8)
  int     lenTimer;
  bool    lenOn;
};

struct NoiseCh {
  bool    on;
  int     vol, envVol0;
  bool    envAdd;
  int     envPer, envTimer;
  int     clkShift, divCode;
  bool    shortMode;
  uint16_t lfsr;
  uint32_t clkAcc, clkPer;
  int     lenTimer;
  bool    lenOn;
};

static SquareCh aCh1, aCh2;
static WaveCh   aCh3;
static NoiseCh  aCh4;
static bool     apuOn = true;

// ---- Frame sequencer (512 Hz → length/envelope clocking) ----
static int fsCounter = 0;
static int fsStep    = 0;
#define FS_PERIOD (SAMPLE_RATE / 512)   // ~43 samples

// ---- Phase increment helpers ----
static uint32_t sqInc(int f)
{
  if (f >= 2048) return 0;
  return (uint32_t)(131072.0f / (2048 - f) * 8.0f / SAMPLE_RATE * 8192.0f);
}
static uint32_t wvInc(int f)
{
  if (f >= 2048) return 0;
  return (uint32_t)(65536.0f / (2048 - f) * 32.0f / SAMPLE_RATE * 256.0f);
}
static uint32_t noiseClkPer(int shift, int div)
{
  float d = div == 0 ? 0.5f : (float)div;
  float hz = 524288.0f / d / (float)(1 << (shift + 1));
  if (hz < 1.0f) return 0xFFFFFF;
  return (uint32_t)(SAMPLE_RATE / hz * 256.0f);
}

// ---- Trigger functions ----
static void trigCh1()
{
  aCh1.on      = true;
  aCh1.vol     = aCh1.envVol0;
  aCh1.envTimer = aCh1.envPer;
  if (!aCh1.lenTimer) aCh1.lenTimer = 64;
  aCh1.phaseInc = sqInc(aCh1.freqReg);
}
static void trigCh2()
{
  aCh2.on      = true;
  aCh2.vol     = aCh2.envVol0;
  aCh2.envTimer = aCh2.envPer;
  if (!aCh2.lenTimer) aCh2.lenTimer = 64;
  aCh2.phaseInc = sqInc(aCh2.freqReg);
}
static void trigCh3()
{
  aCh3.on    = true;
  aCh3.phase = 0;
  if (!aCh3.lenTimer) aCh3.lenTimer = 256;
  aCh3.phaseInc = wvInc(aCh3.freqReg);
}
static void trigCh4()
{
  aCh4.on       = true;
  aCh4.vol      = aCh4.envVol0;
  aCh4.envTimer  = aCh4.envPer;
  aCh4.lfsr     = 0x7FFF;
  if (!aCh4.lenTimer) aCh4.lenTimer = 64;
  aCh4.clkPer   = noiseClkPer(aCh4.clkShift, aCh4.divCode);
  aCh4.clkAcc   = 0;
}

// ---- Frame sequencer tick ----
static void tickFS()
{
  fsStep = (fsStep + 1) & 7;

  if (!(fsStep & 1))
  {
    if (aCh1.lenOn && aCh1.lenTimer > 0 && --aCh1.lenTimer == 0) aCh1.on = false;
    if (aCh2.lenOn && aCh2.lenTimer > 0 && --aCh2.lenTimer == 0) aCh2.on = false;
    if (aCh3.lenOn && aCh3.lenTimer > 0 && --aCh3.lenTimer == 0) aCh3.on = false;
    if (aCh4.lenOn && aCh4.lenTimer > 0 && --aCh4.lenTimer == 0) aCh4.on = false;
  }

  if (fsStep == 7)
  {
    auto tickEnv = [](int& vol, int envVol0, bool envAdd, int envPer, int& envTimer, bool on)
    {
      if (!on || envPer == 0) return;
      if (--envTimer <= 0)
      {
        envTimer = envPer;
        if (envAdd && vol < 15) vol++;
        else if (!envAdd && vol > 0) vol--;
      }
    };
    tickEnv(aCh1.vol, aCh1.envVol0, aCh1.envAdd, aCh1.envPer, aCh1.envTimer, aCh1.on);
    tickEnv(aCh2.vol, aCh2.envVol0, aCh2.envAdd, aCh2.envPer, aCh2.envTimer, aCh2.on);
    tickEnv(aCh4.vol, aCh4.envVol0, aCh4.envAdd, aCh4.envPer, aCh4.envTimer, aCh4.on);
  }
}

// ---- Generate one stereo sample ----
static void apuNextSample(int16_t* L, int16_t* R)
{
  if (!apuOn) { *L = *R = 0; return; }

  if (++fsCounter >= FS_PERIOD) { fsCounter = 0; tickFS(); }

  uint8_t nr51 = apuReg[0x25];
  int ml = 0, mr = 0;

  if (aCh1.on)
  {
    aCh1.phase += aCh1.phaseInc;
    int s = ((aCh1.phase >> 13) & 7) < DUTY[aCh1.dutyIdx] ? aCh1.vol : 0;
    if (nr51 & 0x10) ml += s;
    if (nr51 & 0x01) mr += s;
  }

  if (aCh2.on)
  {
    aCh2.phase += aCh2.phaseInc;
    int s = ((aCh2.phase >> 13) & 7) < DUTY[aCh2.dutyIdx] ? aCh2.vol : 0;
    if (nr51 & 0x20) ml += s;
    if (nr51 & 0x02) mr += s;
  }

  if (aCh3.on && aCh3.dacOn)
  {
    aCh3.phase += aCh3.phaseInc;
    int pos  = (aCh3.phase >> 8) & 31;
    uint8_t wb = apuReg[0x20 + (pos >> 1)];
    int nib  = (pos & 1) ? (wb & 0x0F) : (wb >> 4);
    int s = 0;
    switch (aCh3.outLevel)
    {
      case 1: s = nib;     break;
      case 2: s = nib >> 1; break;
      case 3: s = nib >> 2; break;
    }
    if (nr51 & 0x40) ml += s;
    if (nr51 & 0x04) mr += s;
  }

  if (aCh4.on)
  {
    aCh4.clkAcc += 256;
    while (aCh4.clkAcc >= aCh4.clkPer)
    {
      aCh4.clkAcc -= aCh4.clkPer;
      uint16_t x = (aCh4.lfsr ^ (aCh4.lfsr >> 1)) & 1;
      aCh4.lfsr = (aCh4.lfsr >> 1) | (x << 14);
      if (aCh4.shortMode) aCh4.lfsr = (aCh4.lfsr & ~0x40) | (x << 6);
    }
    int s = (~aCh4.lfsr & 1) ? aCh4.vol : 0;
    if (nr51 & 0x80) ml += s;
    if (nr51 & 0x08) mr += s;
  }

  uint8_t nr50 = apuReg[0x24];
  int vl = ((nr50 >> 4) & 7) + 1;
  int vr = (nr50 & 7) + 1;

  *L = (int16_t)((ml * vl * 32767) / (60 * 8));
  *R = (int16_t)((mr * vr * 32767) / (60 * 8));
}

// ---- audio_read: called by peanut_gb CPU read path ----
extern "C" uint8_t audio_read(uint16_t addr)
{
  if (addr < 0xFF10 || addr > 0xFF3F) return 0xFF;
  return apuReg[addr - 0xFF10];
}

// ---- audio_write: called by peanut_gb CPU write path ----
extern "C" void audio_write(uint16_t addr, uint8_t val)
{
  if (addr < 0xFF10 || addr > 0xFF3F) return;
  apuReg[addr - 0xFF10] = val;

  switch (addr)
  {
    case 0xFF11: aCh1.dutyIdx = (val>>6)&3; aCh1.lenTimer = 64-(val&63); break;
    case 0xFF12:
      aCh1.envVol0 = (val>>4)&15; aCh1.envAdd = (val>>3)&1;
      aCh1.envPer  = val&7;
      if (!(val & 0xF8)) aCh1.on = false;
      break;
    case 0xFF13: aCh1.freqReg = (aCh1.freqReg & 0x700) | val; break;
    case 0xFF14:
      aCh1.freqReg = (aCh1.freqReg & 0xFF) | ((val&7)<<8);
      aCh1.lenOn   = (val>>6)&1;
      if (val & 0x80) trigCh1();
      break;
    case 0xFF16: aCh2.dutyIdx = (val>>6)&3; aCh2.lenTimer = 64-(val&63); break;
    case 0xFF17:
      aCh2.envVol0 = (val>>4)&15; aCh2.envAdd = (val>>3)&1;
      aCh2.envPer  = val&7;
      if (!(val & 0xF8)) aCh2.on = false;
      break;
    case 0xFF18: aCh2.freqReg = (aCh2.freqReg & 0x700) | val; break;
    case 0xFF19:
      aCh2.freqReg = (aCh2.freqReg & 0xFF) | ((val&7)<<8);
      aCh2.lenOn   = (val>>6)&1;
      if (val & 0x80) trigCh2();
      break;
    case 0xFF1A: aCh3.dacOn = (val>>7)&1; if (!aCh3.dacOn) aCh3.on = false; break;
    case 0xFF1B: aCh3.lenTimer = 256 - val; break;
    case 0xFF1C: aCh3.outLevel = (val>>5)&3; break;
    case 0xFF1D: aCh3.freqReg = (aCh3.freqReg & 0x700) | val; break;
    case 0xFF1E:
      aCh3.freqReg = (aCh3.freqReg & 0xFF) | ((val&7)<<8);
      aCh3.lenOn   = (val>>6)&1;
      if (val & 0x80) trigCh3();
      break;
    case 0xFF20: aCh4.lenTimer = 64-(val&63); break;
    case 0xFF21:
      aCh4.envVol0 = (val>>4)&15; aCh4.envAdd = (val>>3)&1;
      aCh4.envPer  = val&7;
      if (!(val & 0xF8)) aCh4.on = false;
      break;
    case 0xFF22:
      aCh4.clkShift  = (val>>4)&15;
      aCh4.shortMode = (val>>3)&1;
      aCh4.divCode   = val&7;
      break;
    case 0xFF23:
      aCh4.lenOn = (val>>6)&1;
      if (val & 0x80) trigCh4();
      break;
    case 0xFF26:
      apuOn = (val>>7)&1;
      if (!apuOn)
      {
        memset(apuReg, 0, 0x20);
        aCh1.on = aCh2.on = aCh3.on = aCh4.on = false;
      }
      break;
  }
}

//////////////////// EMULATOR ////////////////////

struct gb_s gb;
uint16_t scaledRow[320];

#define FRAME_TIME_MS 16

void lcd_draw(struct gb_s* gb, const uint8_t* pixels, const uint_fast8_t line)
{
  for (int x = 0; x < LCD_WIDTH; x++)
  {
    uint8_t p = pixels[x] & 3;
    uint16_t c;
    switch (p)
    {
      case 0: c = 0xFFFF; break;
      case 1: c = 0xAD55; break;
      case 2: c = 0x52AA; break;
      default: c = 0x0000; break;
    }
    scaledRow[x*2] = scaledRow[x*2+1] = c;
  }
  int y0 = ((int)line * 240) / 144;
  int y1 = ((int)(line+1) * 240) / 144;
  for (int sy = y0; sy < y1; sy++)
    tft.drawRGBBitmap(0, sy, scaledRow, 320, 1);
}

//////////////////// ROM ACCESS ////////////////////

uint8_t rom_read(struct gb_s* gb, const uint_fast32_t addr) 
{ 
  if (addr < game_rom_size) {
    return game_rom[addr];
  }
  return 0xFF;
}

uint8_t cart_ram_read(struct gb_s* gb, const uint_fast32_t addr) { return 0; }
void    cart_ram_write(struct gb_s* gb, const uint_fast32_t addr, const uint8_t val) {}

void gb_error(struct gb_s* gb, const enum gb_error_e err, const uint16_t addr)
{
  Serial.print("GB ERROR: "); Serial.println(err);
}

//////////////////// GAME SELECTION ////////////////////

void load_game(int game_index) {
  if (game_index >= 0 && game_index < GAME_COUNT) {
    GameROM game;
    memcpy_P(&game, &GAMES[game_index], sizeof(GameROM));
    
    game_rom = game.data;
    game_rom_size = game.size;
    current_game = game_index;
    
    Serial.print("Loading: ");
    Serial.println(game.name);
    
    tft.fillScreen(0x0000);
    tft.setTextColor(0x07FF);
    tft.setCursor(0, 0);
    tft.print("Loading: ");
    tft.println(game.name);
    
    enum gb_init_error_e ret = gb_init(&gb, rom_read, cart_ram_read, cart_ram_write, gb_error, NULL);
    if (ret != GB_INIT_NO_ERROR)
    {
      Serial.print("ROM FAILED: ");
      Serial.println((int)ret);
      tft.setTextColor(0xF800);
      tft.print("ROM FAILED: ");
      tft.println((int)ret);
      while(1);
    }
    
    gb_init_lcd(&gb, lcd_draw);
  }
}

//////////////////// I2S INIT ////////////////////

void initI2S()
{
  i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 64,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
  };
  i2s_pin_config_t pins = {
    .bck_io_num   = I2S_BCLK,
    .ws_io_num    = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE,
  };
  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
}

//////////////////// AUDIO TASK (Core 0) ////////////////////

void audioTask(void* param)
{
  const int CHUNK = 32;
  int16_t out[CHUNK * 2];

  for (;;)
  {
    int count = 0;
    while (audioBufTail != audioBufHead && count < CHUNK)
    {
      out[count*2]   = audioBuf[audioBufTail*2];
      out[count*2+1] = audioBuf[audioBufTail*2+1];
      audioBufTail = (audioBufTail + 1) % AUDIO_BUF_SAMPLES;
      count++;
    }

    if (count < CHUNK)
    {
      while (count < CHUNK)
      {
        apuNextSample(&out[count*2], &out[count*2+1]);
        count++;
      }
    }

    size_t written = 0;
    i2s_write(I2S_PORT, out, CHUNK * 2 * sizeof(int16_t), &written, portMAX_DELAY);
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

//////////////////// WEB HANDLERS ////////////////////

void handleRoot() { server.send_P(200, "text/html", WEBPAGE); }
void pressUp()     { wifi_keys |= JOYPAD_UP;     server.send(200, "text/plain", "OK"); }
void pressDown()   { wifi_keys |= JOYPAD_DOWN;   server.send(200, "text/plain", "OK"); }
void pressLeft()   { wifi_keys |= JOYPAD_LEFT;   server.send(200, "text/plain", "OK"); }
void pressRight()  { wifi_keys |= JOYPAD_RIGHT;  server.send(200, "text/plain", "OK"); }
void pressA()      { wifi_keys |= JOYPAD_A;      server.send(200, "text/plain", "OK"); }
void pressB()      { wifi_keys |= JOYPAD_B;      server.send(200, "text/plain", "OK"); }
void pressStart()  { wifi_keys |= JOYPAD_START;  server.send(200, "text/plain", "OK"); }
void pressSelect() { wifi_keys |= JOYPAD_SELECT; server.send(200, "text/plain", "OK"); }
void releaseKeys() { wifi_keys = 0;              server.send(200, "text/plain", "OK"); }

//////////////////// SETUP ////////////////////

void setup()
{
  Serial.begin(115200);
  
  memset(apuReg, 0, sizeof(apuReg));
  memset(&aCh1, 0, sizeof(aCh1));
  memset(&aCh2, 0, sizeof(aCh2));
  memset(&aCh3, 0, sizeof(aCh3));
  memset(&aCh4, 0, sizeof(aCh4));
  aCh4.lfsr = 0x7FFF;

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(0x0000);
  tft.setTextColor(0x07FF); tft.setTextSize(2);
  tft.setCursor(60, 90); tft.print("ESP32 GameBoy");
  tft.setTextSize(1); tft.setTextColor(0x8410);

  tft.setCursor(85, 116); tft.print("Starting audio...");
  initI2S();
  xTaskCreatePinnedToCore(audioTask, "audio", 2048, NULL, 1, NULL, 0);

  tft.setCursor(85, 130); tft.print("Starting WiFi...");
  WiFi.softAP(ssid, password);
  tft.setTextColor(0xFFFF);
  tft.setCursor(70, 144); tft.print("SSID: "); tft.print(ssid);
  tft.setCursor(70, 158); tft.print("IP:   "); tft.print(WiFi.softAPIP());

  // Web server routes
  server.on("/", handleRoot);
  server.on("/up", pressUp);
  server.on("/down", pressDown);
  server.on("/left", pressLeft);
  server.on("/right", pressRight);
  server.on("/a", pressA);
  server.on("/b", pressB);
  server.on("/start", pressStart);
  server.on("/select", pressSelect);
  server.on("/release", releaseKeys);
  server.begin();

  tft.setTextColor(0x8410);
  tft.setCursor(85, 172); tft.print("Loading ROM...");

  // Load first game
  load_game(0);
}

//////////////////// LOOP ////////////////////

void loop()
{
  server.handleClient();

  uint32_t frame_start = millis();
  gb.direct.joypad = ~wifi_keys;
  gb_run_frame(&gb);

  uint32_t elapsed = millis() - frame_start;
  if (elapsed < FRAME_TIME_MS)
    delay(FRAME_TIME_MS - elapsed);
  else
    delay(1);
}
