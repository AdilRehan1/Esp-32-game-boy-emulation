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

#define TFT_CS 5
#define TFT_DC 2
#define TFT_RST 4

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// ---------------- BUTTONS ----------------

#define BTN_UP 32
#define BTN_DOWN 33
#define BTN_LEFT 34
#define BTN_RIGHT 35
#define BTN_A 25
#define BTN_B 26
#define BTN_START 27
#define BTN_SELECT 14

// ---------------- EMULATOR ----------------

struct gb_s gb;

uint16_t framebuffer[160 * 144];

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

void error(struct gb_s* gb, const enum gb_error_e err, const uint16_t addr)
{
  Serial.print("GB ERROR ");
  Serial.println(err);
}

// ---------------- LCD DRAW ----------------

void lcd_draw(struct gb_s* gb, const uint_fast32_t y, const uint8_t* pixels)
{
  for (int x = 0; x < 160; x++)
  {
    uint8_t p = pixels[x];

    uint16_t color;

    switch (p)
    {
      case 0: color = ILI9341_WHITE; break;
      case 1: color = ILI9341_LIGHTGREY; break;
      case 2: color = ILI9341_DARKGREY; break;
      default: color = ILI9341_BLACK; break;
    }

    framebuffer[y * 160 + x] = color;
  }

  if (y == 143)
  {
    tft.drawRGBBitmap(40, 20, framebuffer, 160, 144);
  }
}

// ---------------- INPUT ----------------

uint8_t read_input()
{
  uint8_t keys = 0;

  if (!digitalRead(BTN_RIGHT)) keys |= GB_PAD_RIGHT;
  if (!digitalRead(BTN_LEFT))  keys |= GB_PAD_LEFT;
  if (!digitalRead(BTN_UP))    keys |= GB_PAD_UP;
  if (!digitalRead(BTN_DOWN))  keys |= GB_PAD_DOWN;

  if (!digitalRead(BTN_A)) keys |= GB_PAD_A;
  if (!digitalRead(BTN_B)) keys |= GB_PAD_B;

  if (!digitalRead(BTN_START))  keys |= GB_PAD_START;
  if (!digitalRead(BTN_SELECT)) keys |= GB_PAD_SELECT;

  return keys;
}

// ---------------- SETUP ----------------

void setup()
{
  Serial.begin(115200);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  Serial.println("Starting GameBoy");

  gb_init(
    &gb,
    rom_read,
    cart_ram_read,
    cart_ram_write,
    error,
    NULL
  );

  gb_set_lcd_draw_callback(&gb, lcd_draw);
}

// ---------------- LOOP ----------------

void loop()
{
  gb_set_joypad(&gb, read_input());

  gb_run_frame(&gb);
}
