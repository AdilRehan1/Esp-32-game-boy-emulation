#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

extern "C" {
#define PEANUT_GB_IMPLEMENTATION
#include "peanut_gb.h"
}

#include "game_rom.h"

// ---------------- DISPLAY ----------------
#define TFT_CS  5
#define TFT_DC  2
#define TFT_RST 4

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ---------------- BUTTONS ----------------
#define BTN_UP     32   
#define BTN_DOWN   33   
#define BTN_LEFT   34   
#define BTN_RIGHT  35  
#define BTN_A      25 
#define BTN_B      26 
#define BTN_START  27 
#define BTN_SELECT 22 

// ---------------- EMULATOR ----------------
struct gb_s gb;
uint16_t framebuffer[LCD_WIDTH * LCD_HEIGHT];

#define FRAME_TIME_MS 16

// ---------------- ROM ACCESS ----------------
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

// ---------------- ERROR HANDLER ----------------
void gb_error(struct gb_s* gb, const enum gb_error_e err, const uint16_t addr)
{
  Serial.print("GB ERROR: ");
  Serial.println(err);
}

// ---------------- LCD DRAW ----------------
void lcd_draw(struct gb_s* gb, const uint8_t* pixels, const uint_fast8_t line)
{
  memset(&framebuffer[line * LCD_WIDTH], 0, LCD_WIDTH * sizeof(uint16_t));

  for (int x = 0; x < LCD_WIDTH; x++)
  {
    uint8_t p = pixels[x] & 3;
    uint16_t color;
    switch (p)
    {
      case 0: color = 0xFFFF; break; // White
      case 1: color = 0xAD55; break; // Light grey
      case 2: color = 0x52AA; break; // Dark grey
      default: color = 0x0000; break; // Black
    }
    framebuffer[line * LCD_WIDTH + x] = color;
  }


  if (line == LCD_HEIGHT - 1)
  {
    tft.drawRGBBitmap(80, 48, framebuffer, LCD_WIDTH, LCD_HEIGHT);
    Serial.println("frame drawn"); 
  }
}

// ---------------- INPUT ----------------
uint8_t read_input()
{
  uint8_t keys = 0;

  if (!digitalRead(BTN_RIGHT))  keys |= JOYPAD_RIGHT;
  if (!digitalRead(BTN_LEFT))   keys |= JOYPAD_LEFT;
  if (!digitalRead(BTN_UP))     keys |= JOYPAD_UP;
  if (!digitalRead(BTN_DOWN))   keys |= JOYPAD_DOWN;

  if (!digitalRead(BTN_A))      keys |= JOYPAD_A;
  if (!digitalRead(BTN_B))      keys |= JOYPAD_B;

  if (!digitalRead(BTN_SELECT)) keys |= JOYPAD_SELECT;
  if (!digitalRead(BTN_START))  keys |= JOYPAD_START;

  return keys;
}

// ---------------- SETUP ----------------
void setup()
{
  Serial.begin(115200);

  pinMode(BTN_UP,     INPUT_PULLUP);
  pinMode(BTN_DOWN,   INPUT_PULLUP);
  pinMode(BTN_LEFT,   INPUT_PULLUP);
  pinMode(BTN_RIGHT,  INPUT_PULLUP);
  pinMode(BTN_A,      INPUT_PULLUP);
  pinMode(BTN_B,      INPUT_PULLUP);
  pinMode(BTN_START,  INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

 
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(0x0000);
  Serial.println("Starting GameBoy");

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
    while (1);
  }
  Serial.println("gb_init OK");

  gb_init_lcd(&gb, lcd_draw);
  Serial.println("lcd init OK");

  memset(framebuffer, 0, sizeof(framebuffer));

  Serial.println("Entering loop...");
}

// ---------------- LOOP ----------------
void loop()
{
  uint32_t frame_start = millis();

  uint8_t raw = read_input();
  if (raw != 0)
  {
    Serial.print("raw input: 0b");
    Serial.print(raw, BIN);
    Serial.print("  joypad sent: 0b");
    Serial.println(~raw, BIN);
  }

  gb.direct.joypad = ~raw;
  gb_run_frame(&gb);

  uint32_t elapsed = millis() - frame_start;
  if (elapsed < FRAME_TIME_MS)
    delay(FRAME_TIME_MS - elapsed);
  else
    delay(1);
}
