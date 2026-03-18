#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

extern "C" {
#define PEANUT_GB_IMPLEMENTATION
#include "peanut_gb.h"
}

#include "rom_list.h"
#include "webpage.h"

//////////////////// PINS ////////////////////

#define TFT_CS  5
#define TFT_DC  2
#define TFT_RST 4

//////////////////// WIFI ////////////////////

const char* ssid     = "ESP32-GameBoy";
const char* password = "12345678";

WebServer server(80);
volatile uint8_t wifi_keys      = 0;
volatile uint8_t wifi_keys_prev = 0;

//////////////////// DISPLAY ////////////////////

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

//////////////////// EMULATOR ////////////////////

struct gb_s  gb;
uint16_t     scaledRow[320];
int          activeRomIdx = 0;

//////////////////// APP STATE ////////////////////

enum AppState { STATE_MENU, STATE_GAME };
AppState appState = STATE_MENU;
int menuSelected  = 0;

//////////////////// ROM ACCESS ////////////////////

// Reads from PROGMEM — no RAM used for ROM data
uint8_t rom_read(struct gb_s* gb, const uint_fast32_t addr)
{
  if (addr < ROM_LIST[activeRomIdx].size)
    return pgm_read_byte(&ROM_LIST[activeRomIdx].data[addr]);
  return 0xFF;
}

uint8_t cart_ram_read(struct gb_s* gb, const uint_fast32_t addr)
{
  return 0;
}

void cart_ram_write(struct gb_s* gb, const uint_fast32_t addr, const uint8_t val)
{
}

//////////////////// GB ERROR ////////////////////

void gb_error(struct gb_s* gb, const enum gb_error_e err, const uint16_t addr)
{
  Serial.print("GB ERROR: ");
  Serial.println(err);
}

//////////////////// LCD DRAW — fullscreen stretch ////////////////////

void lcd_draw(struct gb_s* gb, const uint8_t* pixels, const uint_fast8_t line)
{
  for (int x = 0; x < LCD_WIDTH; x++)
  {
    uint8_t p = pixels[x] & 3;
    uint16_t color;
    switch (p)
    {
      case 0: color = 0xFFFF; break;
      case 1: color = 0xAD55; break;
      case 2: color = 0x52AA; break;
      default: color = 0x0000; break;
    }
    scaledRow[x * 2]     = color;
    scaledRow[x * 2 + 1] = color;
  }

  int y0 = ((int)line * 240) / 144;
  int y1 = ((int)(line + 1) * 240) / 144;
  for (int sy = y0; sy < y1; sy++)
    tft.drawRGBBitmap(0, sy, scaledRow, 320, 1);
}

//////////////////// HELPERS ////////////////////

void formatSize(uint32_t size, char* out)
{
  if (size >= 1024 * 1024)
    snprintf(out, 12, "%.1fMB", size / (1024.0f * 1024.0f));
  else
    snprintf(out, 12, "%uKB", (unsigned)(size / 1024));
}

//////////////////// MENU ////////////////////

#define MENU_ITEM_H  36
#define MENU_VISIBLE  5

void drawMenu()
{
  tft.fillScreen(0x0000);

  // Title bar
  tft.fillRect(0, 0, 320, 40, 0x0319);
  tft.setTextColor(0x07FF);
  tft.setTextSize(2);
  tft.setCursor(70, 12);
  tft.print("SELECT GAME");

  // ROM count
  tft.setTextSize(1);
  tft.setTextColor(0x8410);
  tft.setCursor(252, 4);
  tft.print(ROM_COUNT);
  tft.print(" ROMs");

  // Scanline background
  for (int y = 40; y < 228; y += 4)
    tft.drawFastHLine(0, y, 320, 0x0821);

  // Scroll window
  int scrollStart = max(0, min(menuSelected - 2,
                               ROM_COUNT - MENU_VISIBLE));

  for (int i = 0; i < MENU_VISIBLE; i++)
  {
    int idx = scrollStart + i;
    if (idx >= ROM_COUNT) break;

    int  y     = 46 + i * MENU_ITEM_H;
    bool isSel = (idx == menuSelected);

    if (isSel)
    {
      tft.fillRoundRect(6, y, 306, MENU_ITEM_H - 4, 6, 0x07FF);
      tft.setTextColor(0x0000);
    }
    else
    {
      tft.drawRoundRect(6, y, 306, MENU_ITEM_H - 4, 6, 0x2945);
      tft.setTextColor(0xFFFF);
    }

    // Arrow + name
    tft.setTextSize(2);
    tft.setCursor(16, y + 8);
    tft.print(isSel ? "> " : "  ");

    // Truncate long names
    char truncated[18];
    strncpy(truncated, ROM_LIST[idx].name, 17);
    truncated[17] = '\0';
    tft.print(truncated);

    // Size on right
    char sizeBuf[12];
    formatSize(ROM_LIST[idx].size, sizeBuf);
    tft.setTextSize(1);
    tft.setTextColor(isSel ? 0x0000 : 0x8410);
    tft.setCursor(268, y + 12);
    tft.print(sizeBuf);
  }

  // Scrollbar
  if (ROM_COUNT > MENU_VISIBLE)
  {
    int trackH = MENU_VISIBLE * MENU_ITEM_H;
    int barH   = max(8, (MENU_VISIBLE * trackH) / ROM_COUNT);
    int barY   = 46 + (scrollStart * (trackH - barH)) /
                       max(1, ROM_COUNT - MENU_VISIBLE);
    tft.fillRect(314, 46, 4, trackH, 0x2945);
    tft.fillRect(314, barY, 4, barH, 0x07FF);
  }

  // Footer
  tft.fillRect(0, 228, 320, 12, 0x0319);
  tft.setTextColor(0x8410);
  tft.setTextSize(1);
  tft.setCursor(55, 231);
  tft.print("[UP/DOWN] Navigate    [A] Launch");
}

//////////////////// LAUNCH ROM ////////////////////

void launchRom(int idx)
{
  activeRomIdx = idx;

  // Loading screen
  tft.fillScreen(0x0000);
  tft.fillRect(0, 0, 320, 40, 0x0319);
  tft.setTextColor(0x07FF);
  tft.setTextSize(2);
  tft.setCursor(90, 12);
  tft.print("LOADING");

  tft.setTextColor(0xFFFF);
  tft.setTextSize(2);
  tft.setCursor(20, 80);
  tft.print(ROM_LIST[idx].name);

  char sizeBuf[12];
  formatSize(ROM_LIST[idx].size, sizeBuf);
  tft.setTextColor(0x8410);
  tft.setTextSize(1);
  tft.setCursor(20, 108);
  tft.print(sizeBuf);
  tft.print(" in PROGMEM");

  tft.setCursor(20, 130);
  tft.print("Initializing emulator...");

  enum gb_init_error_e ret = gb_init(
    &gb,
    rom_read,
    cart_ram_read,
    cart_ram_write,
    gb_error,
    NULL
  );

  if (ret != GB_INIT_NO_ERROR)
  {
    tft.setTextColor(0xF800);
    tft.setCursor(20, 155);
    tft.print("gb_init FAILED: ");
    tft.print((int)ret);
    Serial.print("gb_init failed: ");
    Serial.println((int)ret);
    delay(2000);
    drawMenu();
    appState = STATE_MENU;
    return;
  }

  gb_init_lcd(&gb, lcd_draw);

  tft.setTextColor(0x07E0);
  tft.setCursor(20, 155);
  tft.print("OK — starting!");
  delay(500);

  wifi_keys      = 0;
  wifi_keys_prev = 0;
  appState       = STATE_GAME;
}

//////////////////// WEB HANDLERS ////////////////////

void handleRoot()    { server.send_P(200, "text/html", WEBPAGE); }

void pressUp()     { wifi_keys |= JOYPAD_UP;     server.send(200,"text/plain","OK"); }
void pressDown()   { wifi_keys |= JOYPAD_DOWN;   server.send(200,"text/plain","OK"); }
void pressLeft()   { wifi_keys |= JOYPAD_LEFT;   server.send(200,"text/plain","OK"); }
void pressRight()  { wifi_keys |= JOYPAD_RIGHT;  server.send(200,"text/plain","OK"); }
void pressA()      { wifi_keys |= JOYPAD_A;      server.send(200,"text/plain","OK"); }
void pressB()      { wifi_keys |= JOYPAD_B;      server.send(200,"text/plain","OK"); }
void pressStart()  { wifi_keys |= JOYPAD_START;  server.send(200,"text/plain","OK"); }
void pressSelect() { wifi_keys |= JOYPAD_SELECT; server.send(200,"text/plain","OK"); }
void releaseKeys() { wifi_keys = 0;              server.send(200,"text/plain","OK"); }

//////////////////// SETUP ////////////////////

void setup()
{
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(0x0000);

  // Splash
  tft.setTextColor(0x07FF);
  tft.setTextSize(2);
  tft.setCursor(55, 80);
  tft.print("ESP32 GameBoy");

  tft.setTextSize(1);
  tft.setTextColor(0x8410);
  tft.setCursor(30, 115);
  tft.print(ROM_COUNT);
  tft.print(ROM_COUNT == 1 ? " ROM loaded" : " ROMs loaded");

  tft.setCursor(30, 133);
  tft.print("Starting WiFi...");

  WiFi.softAP(ssid, password);

  tft.setTextColor(0x07E0);
  tft.setCursor(30, 151);
  tft.print("SSID: ");
  tft.print(ssid);
  tft.setCursor(30, 169);
  tft.print("IP:   ");
  tft.print(WiFi.softAPIP());

  server.on("/",        handleRoot);
  server.on("/up",      pressUp);
  server.on("/down",    pressDown);
  server.on("/left",    pressLeft);
  server.on("/right",   pressRight);
  server.on("/a",       pressA);
  server.on("/b",       pressB);
  server.on("/start",   pressStart);
  server.on("/select",  pressSelect);
  server.on("/release", releaseKeys);
  server.begin();

  Serial.println("Ready");
  delay(1200);
  drawMenu();
}

//////////////////// LOOP ////////////////////

#define FRAME_TIME_MS 16

void loop()
{
  server.handleClient();

  uint8_t pressed = wifi_keys & ~wifi_keys_prev;
  wifi_keys_prev  = wifi_keys;

  // ── MENU ─────────────────────────────────────
  if (appState == STATE_MENU)
  {
    bool redraw = false;
    if (pressed & JOYPAD_UP)
    {
      menuSelected = (menuSelected - 1 + ROM_COUNT) % ROM_COUNT;
      redraw = true;
    }
    if (pressed & JOYPAD_DOWN)
    {
      menuSelected = (menuSelected + 1) % ROM_COUNT;
      redraw = true;
    }
    if (redraw) drawMenu();
    if (pressed & JOYPAD_A) launchRom(menuSelected);

    delay(16);
    return;
  }

  // ── GAME ─────────────────────────────────────
  uint32_t frame_start = millis();

  // Hold SELECT + START for 2 seconds to return to menu
  static uint32_t exit_hold_start = 0;
  if ((wifi_keys & JOYPAD_SELECT) && (wifi_keys & JOYPAD_START))
  {
    if (exit_hold_start == 0) exit_hold_start = millis();
    if (millis() - exit_hold_start > 2000)
    {
      exit_hold_start = 0;
      wifi_keys       = 0;
      wifi_keys_prev  = 0;
      appState        = STATE_MENU;
      drawMenu();
      return;
    }
  }
  else
  {
    exit_hold_start = 0;
  }

  gb.direct.joypad = ~wifi_keys;
  gb_run_frame(&gb);

  uint32_t elapsed = millis() - frame_start;
  if (elapsed < FRAME_TIME_MS)
    delay(FRAME_TIME_MS - elapsed);
  else
    delay(1);
}
