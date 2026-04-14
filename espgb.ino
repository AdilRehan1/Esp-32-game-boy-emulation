#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "driver/i2s.h"

// Include our separated modules
#include "sd_manager.h"

extern "C" uint8_t audio_read(uint16_t addr);
extern "C" void    audio_write(uint16_t addr, uint8_t val);

extern "C" {
#define PEANUT_GB_IMPLEMENTATION
#define ENABLE_SOUND 1
#include "peanut_gb.h"
}

// ==================== PIN DEFINITIONS ====================

// Display (ILI9341)
#define TFT_CS      5
#define TFT_DC      2
#define TFT_RST     4
#define TFT_MOSI    23
#define TFT_MISO    19
#define TFT_SCLK    18

// I2S Audio (MAX98357A)
#define I2S_BCLK    26
#define I2S_LRC     25
#define I2S_DOUT    22

// Buttons (all use INPUT_PULLUP, connect to GND when pressed)
#define BTN_UP      13
#define BTN_DOWN    12
#define BTN_LEFT    14
#define BTN_RIGHT   27
#define BTN_A       26
#define BTN_B       25
#define BTN_START   33
#define BTN_SELECT  32
#define BTN_MENU    15

// Volume Control (Potentiometer)
#define VOLUME_POT  34

// ==================== GLOBALS ====================

// Emulator
struct gb_s gb;
const unsigned char* game_rom = nullptr;
uint32_t game_rom_size = 0;
uint16_t scaledRow[320];

// Game selection
int current_game = 0;
int menuSelection = 0;
bool game_selected = false;
bool return_to_menu = false;

// Button states
volatile uint8_t wifi_keys = 0;
bool lastButtonStates[9] = {false};
unsigned long lastDebounce[9] = {0};
const unsigned long debounceDelay = 50;

// Volume
int currentVolume = 70;
float volumeScale = 0.7;
unsigned long lastVolumeRead = 0;

// Frame timing
#define FRAME_TIME_MS 16

// ==================== AUDIO BUFFER ====================

#define SAMPLE_RATE 22050
#define AUDIO_BUF_SAMPLES 1024
static int16_t audioBuf[AUDIO_BUF_SAMPLES * 2];
static volatile int audioBufHead = 0;
static volatile int audioBufTail = 0;

// ==================== GB APU ====================
// (Keep all your existing APU code here - aCh1, aCh2, aCh3, aCh4, apuReg, etc.)
// ... (I'm not repeating it for brevity, but keep all your APU functions) ...

// ==================== DISPLAY ====================

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

void lcd_draw(struct gb_s* gb, const uint8_t* pixels, const uint_fast8_t line) {
  for (int x = 0; x < LCD_WIDTH; x++) {
    uint8_t p = pixels[x] & 3;
    uint16_t c;
    switch (p) {
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

// ==================== ROM ACCESS ====================

uint8_t rom_read(struct gb_s* gb, const uint_fast32_t addr) {
  if (addr < game_rom_size) {
    return game_rom[addr];
  }
  return 0xFF;
}

uint8_t cart_ram_read(struct gb_s* gb, const uint_fast32_t addr) { return 0; }
void cart_ram_write(struct gb_s* gb, const uint_fast32_t addr, const uint8_t val) {}

void gb_error(struct gb_s* gb, const enum gb_error_e err, const uint16_t addr) {
  Serial.print("GB ERROR: "); Serial.println(err);
}

// ==================== GAME MENU ====================

void drawGameMenu() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(2);
  tft.setCursor(40, 10);
  tft.print("ESP32 GameBoy");
  
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(30, 45);
  tft.print("SELECT GAME:");
  
  tft.setTextSize(2);
  
  int totalGames = getGameCount();
  
  if (totalGames == 0) {
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(30, 100);
    tft.print("NO GAMES FOUND!");
    tft.setTextSize(1);
    tft.setCursor(30, 140);
    tft.print("Place .gb files in /games");
    tft.setCursor(30, 160);
    tft.print("folder on SD card");
    return;
  }
  
  // Show up to 5 games on screen
  int startIndex = 0;
  if (menuSelection >= 5) {
    startIndex = menuSelection - 4;
  }
  
  for (int i = startIndex; i < totalGames && i < startIndex + 5; i++) {
    int y = 80 + (i - startIndex) * 40;
    
    if (i == menuSelection) {
      tft.fillRect(20, y - 12, 280, 35, ILI9341_BLUE);
      tft.setTextColor(ILI9341_YELLOW);
    } else {
      tft.setTextColor(ILI9341_WHITE);
    }
    
    tft.setCursor(30, y);
    tft.print(i + 1);
    tft.print(". ");
    tft.print(getGameName(i));
  }
  
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(1);
  tft.setCursor(30, 210);
  tft.print("UP/DOWN: Select  START: Play");
  tft.setCursor(30, 225);
  tft.print("MENU: Exit game (in-game)");
  
  // Show volume
  tft.setCursor(250, 225);
  tft.print("Vol:");
  tft.print(currentVolume);
  tft.print("%");
}

void load_game(int game_index) {
  if (!isSDCardAvailable() || game_index >= getGameCount()) return;
  
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(0, 0);
  tft.print("Loading: ");
  tft.println(getGameName(game_index));
  
  // Load ROM from SD card
  uint8_t* rom = loadROMFromSD(getGameFilename(game_index), &game_rom_size);
  if (rom == nullptr) {
    tft.setTextColor(ILI9341_RED);
    tft.print("FAILED TO LOAD ROM!");
    delay(2000);
    drawGameMenu();
    return;
  }
  
  game_rom = rom;
  current_game = game_index;
  
  Serial.print("Loading: ");
  Serial.println(getGameName(game_index));
  
  enum gb_init_error_e ret = gb_init(&gb, rom_read, cart_ram_read, cart_ram_write, gb_error, NULL);
  if (ret != GB_INIT_NO_ERROR) {
    Serial.print("ROM FAILED: ");
    Serial.println((int)ret);
    tft.setTextColor(ILI9341_RED);
    tft.print("ROM INIT FAILED: ");
    tft.println((int)ret);
    delay(2000);
    drawGameMenu();
    return;
  }
  
  gb_init_lcd(&gb, lcd_draw);
  game_selected = true;
  
  // Clear screen for game
  tft.fillScreen(ILI9341_BLACK);
}

// ==================== VOLUME CONTROL ====================

void updateVolume() {
  int potValue = analogRead(VOLUME_POT);
  static int lastVolume = -1;
  
  int newVolume = map(potValue, 0, 4095, 0, 100);
  newVolume = constrain(newVolume, 0, 100);
  
  if (abs(newVolume - lastVolume) > 2) {
    currentVolume = newVolume;
    volumeScale = currentVolume / 100.0;
    lastVolume = currentVolume;
    
    // Show volume on TFT if in menu
    if (!game_selected) {
      tft.fillRect(250, 215, 70, 20, ILI9341_BLACK);
      tft.setTextColor(ILI9341_GREEN);
      tft.setCursor(250, 225);
      tft.print("Vol:");
      tft.print(currentVolume);
      tft.print("%");
    }
  }
}

// ==================== BUTTON HANDLING ====================

void initButtons() {
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_MENU, INPUT_PULLUP);
}

void checkButtons() {
  bool currentState;
  unsigned long now = millis();
  
  struct ButtonMap {
    int pin;
    uint8_t joypadBit;
    int index;
  };
  
  ButtonMap buttons[] = {
    {BTN_UP,     JOYPAD_UP,     0},
    {BTN_DOWN,   JOYPAD_DOWN,   1},
    {BTN_LEFT,   JOYPAD_LEFT,   2},
    {BTN_RIGHT,  JOYPAD_RIGHT,  3},
    {BTN_A,      JOYPAD_A,      4},
    {BTN_B,      JOYPAD_B,      5},
    {BTN_START,  JOYPAD_START,  6},
    {BTN_SELECT, JOYPAD_SELECT, 7},
  };
  
  // Game buttons
  for (int i = 0; i < 8; i++) {
    currentState = digitalRead(buttons[i].pin) == LOW;
    
    if (currentState != lastButtonStates[i]) {
      lastDebounce[i] = now;
    }
    
    if ((now - lastDebounce[i]) > debounceDelay) {
      if (currentState && !lastButtonStates[i]) {
        wifi_keys |= buttons[i].joypadBit;
        lastButtonStates[i] = true;
      } else if (!currentState && lastButtonStates[i]) {
        wifi_keys &= ~buttons[i].joypadBit;
        lastButtonStates[i] = false;
      }
    }
  }
  
  // MENU button
  static bool lastMenuState = false;
  static unsigned long lastMenuDebounce = 0;
  bool menuState = digitalRead(BTN_MENU) == LOW;
  
  if (menuState != lastMenuState) {
    lastMenuDebounce = now;
  }
  
  if ((now - lastMenuDebounce) > debounceDelay) {
    if (menuState && !lastMenuState) {
      if (game_selected) {
        return_to_menu = true;
      }
      lastMenuState = true;
    } else if (!menuState && lastMenuState) {
      lastMenuState = false;
    }
  }
  
  // Menu navigation (only in menu)
  if (!game_selected) {
    static bool upPressed = false, downPressed = false;
    static unsigned long lastNavTime = 0;
    
    bool upCurrent = digitalRead(BTN_UP) == LOW;
    bool downCurrent = digitalRead(BTN_DOWN) == LOW;
    
    if (upCurrent && !upPressed && (now - lastNavTime) > 200) {
      menuSelection--;
      if (menuSelection < 0) menuSelection = getGameCount() - 1;
      drawGameMenu();
      upPressed = true;
      lastNavTime = now;
    } else if (!upCurrent && upPressed) {
      upPressed = false;
    }
    
    if (downCurrent && !downPressed && (now - lastNavTime) > 200) {
      menuSelection++;
      if (menuSelection >= getGameCount()) menuSelection = 0;
      drawGameMenu();
      downPressed = true;
      lastNavTime = now;
    } else if (!downCurrent && downPressed) {
      downPressed = false;
    }
    
    // START button to select game
    static bool startPressed = false;
    bool startCurrent = digitalRead(BTN_START) == LOW;
    
    if (startCurrent && !startPressed && (now - lastNavTime) > 200) {
      load_game(menuSelection);
      startPressed = true;
    } else if (!startCurrent && startPressed) {
      startPressed = false;
    }
  }
}

// ==================== I2S AUDIO ====================

void initI2S() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
  };
  
  i2s_pin_config_t pins = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE,
  };
  
  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

void audioTask(void* param) {
  const int CHUNK = 32;
  int16_t out[CHUNK * 2];
  
  for (;;) {
    int count = 0;
    while (audioBufTail != audioBufHead && count < CHUNK) {
      out[count*2] = audioBuf[audioBufTail*2];
      out[count*2+1] = audioBuf[audioBufTail*2+1];
      audioBufTail = (audioBufTail + 1) % AUDIO_BUF_SAMPLES;
      count++;
    }
    
    if (count < CHUNK) {
      while (count < CHUNK) {
        apuNextSample(&out[count*2], &out[count*2+1]);
        count++;
      }
    }
    
    size_t written = 0;
    i2s_write(I2S_NUM_0, out, CHUNK * 2 * sizeof(int16_t), &written, portMAX_DELAY);
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  
  // Initialize APU
  memset(apuReg, 0, sizeof(apuReg));
  memset(&aCh1, 0, sizeof(aCh1));
  memset(&aCh2, 0, sizeof(aCh2));
  memset(&aCh3, 0, sizeof(aCh3));
  memset(&aCh4, 0, sizeof(aCh4));
  aCh4.lfsr = 0x7FFF;
  
  // Initialize display
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  
  // Initialize buttons
  initButtons();
  
  // Initialize volume potentiometer
  pinMode(VOLUME_POT, INPUT);
  analogReadResolution(12);
  
  // Initialize SD card (this will scan for ROMs automatically)
  initSDCard();
  
  // Initialize I2S
  initI2S();
  
  // Start audio task
  xTaskCreatePinnedToCore(audioTask, "audio", 4096, NULL, 1, NULL, 0);
  
  // Show game menu
  drawGameMenu();
  
  Serial.println("ESP32 GameBoy Ready!");
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
}

// ==================== LOOP ====================

void loop() {
  // Update volume from potentiometer
  static unsigned long lastVolumeCheck = 0;
  if (millis() - lastVolumeCheck > 50) {
    updateVolume();
    lastVolumeCheck = millis();
  }
  
  // Check buttons
  checkButtons();
  
  // Handle menu exit
  if (return_to_menu) {
    return_to_menu = false;
    game_selected = false;
    
    // Reset APU
    memset(apuReg, 0, sizeof(apuReg));
    memset(&aCh1, 0, sizeof(aCh1));
    memset(&aCh2, 0, sizeof(aCh2));
    memset(&aCh3, 0, sizeof(aCh3));
    memset(&aCh4, 0, sizeof(aCh4));
    aCh4.lfsr = 0x7FFF;
    apuOn = true;
    fsCounter = 0;
    fsStep = 0;
    
    menuSelection = current_game;
    drawGameMenu();
    return;
  }
  
  // Wait for game selection
  if (!game_selected) {
    delay(10);
    return;
  }
  
  // Run emulator
  uint32_t frame_start = millis();
  gb.direct.joypad = ~wifi_keys;
  gb_run_frame(&gb);
  
  uint32_t elapsed = millis() - frame_start;
  if (elapsed < FRAME_TIME_MS)
    delay(FRAME_TIME_MS - elapsed);
  else
    delay(1);
}
