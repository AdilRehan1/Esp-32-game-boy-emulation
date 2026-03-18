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

#include "game_rom.h"
#include "webpage.h"       // HTML lives here — loaded from PROGMEM

//////////////////// WIFI ////////////////////

const char* ssid     = "ESP32-GameBoy";
const char* password = "12345678";

WebServer server(80);
volatile uint8_t wifi_keys = 0;   // set by web handlers, read in loop

//////////////////// DISPLAY ////////////////////

#define TFT_CS  5
#define TFT_DC  2
#define TFT_RST 4

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

//////////////////// EMULATOR ////////////////////

struct gb_s gb;
uint16_t framebuffer[LCD_WIDTH * LCD_HEIGHT];

#define FRAME_TIME_MS 16

//////////////////// ROM ACCESS ////////////////////

uint8_t rom_read(struct gb_s* gb, const uint_fast32_t addr)
{
  return game_rom[addr];
}

uint8_t cart_ram_read(struct gb_s* gb, const uint_fast32_t addr)
{
  return 0;
}

void cart_ram_write(struct gb_s* gb, const uint_fast32_t addr, const uint8_t val)
{
}

//////////////////// ERROR ////////////////////

void gb_error(struct gb_s* gb, const enum gb_error_e err, const uint16_t addr)
{
  Serial.print("GB ERROR: ");
  Serial.println(err);
}

//////////////////// LCD DRAW ////////////////////

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
    framebuffer[line * LCD_WIDTH + x] = color;
  }

  if (line == LCD_HEIGHT - 1)
  {
    tft.drawRGBBitmap(80, 48, framebuffer, LCD_WIDTH, LCD_HEIGHT);
  }
}

//////////////////// WEB HANDLERS ////////////////////

void handleRoot()
{
  // Serve the HTML from PROGMEM — zero RAM cost for the page string
  server.send_P(200, "text/html", WEBPAGE);
}

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

  // Display
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(0x0000);

  // Splash
  tft.setTextColor(0x07FF);
  tft.setTextSize(2);
  tft.setCursor(60, 100);
  tft.print("ESP32 GameBoy");
  tft.setTextSize(1);
  tft.setTextColor(0x8410);
  tft.setCursor(85, 126);
  tft.print("Starting WiFi...");

  // WiFi AP
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Show IP on screen
  tft.fillRect(0, 140, 320, 14, 0x0000);
  tft.setTextColor(0xFFFF);
  tft.setCursor(70, 141);
  tft.print("Connect to: ");
  tft.print(ssid);
  tft.setCursor(85, 155);
  tft.print("IP: ");
  tft.print(WiFi.softAPIP());

  // Web server routes
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

  // Emulator
  tft.setCursor(85, 170);
  tft.setTextColor(0x8410);
  tft.print("Loading ROM...");

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
    Serial.print("gb_init failed: ");
    Serial.println((int)ret);
    tft.setTextColor(0xF800);
    tft.setCursor(85, 185);
    tft.print("ROM FAILED: ");
    tft.print((int)ret);
    while (1);
  }

  gb_init_lcd(&gb, lcd_draw);
  memset(framebuffer, 0, sizeof(framebuffer));

  Serial.println("Ready — open browser to " + WiFi.softAPIP().toString());
}

//////////////////// LOOP ////////////////////

void loop()
{
  server.handleClient();

  uint32_t frame_start = millis();

  // WiFi-only input — physical buttons removed
  // peanut_gb expects joypad bits INVERTED (0 = pressed)
  gb.direct.joypad = ~wifi_keys;

  gb_run_frame(&gb);

  uint32_t elapsed = millis() - frame_start;
  if (elapsed < FRAME_TIME_MS)
    delay(FRAME_TIME_MS - elapsed);
  else
    delay(1);
}
