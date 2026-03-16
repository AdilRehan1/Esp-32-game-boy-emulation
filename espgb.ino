#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

extern "C" {
#define PEANUT_GB_IMPLEMENTATION
#include "peanut_gb.h"
}

#include "game_rom.h"

// ---------------- DISPLAY PINS ----------------

#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_MISO 19

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ---------------- BUTTON PINS ----------------

#define BTN_UP     32
#define BTN_DOWN   33
#define BTN_LEFT   34
#define BTN_RIGHT  35
#define BTN_A      25
#define BTN_B      26
#define BTN_START  27
#define BTN_SELECT 14

// ---------------- EMULATOR ----------------

gb_t gb;

uint16_t framebuffer[160 * 144];

// ---------------- INPUT ----------------

uint8_t read_input()
{
  uint8_t keys = 0;

  if (!digitalRead(BTN_RIGHT)) keys |= GB_BUTTON_RIGHT;
  if (!digitalRead(BTN_LEFT))  keys |= GB_BUTTON_LEFT;
  if (!digitalRead(BTN_UP))    keys |= GB_BUTTON_UP;
  if (!digitalRead(BTN_DOWN))  keys |= GB_BUTTON_DOWN;

  if (!digitalRead(BTN_A))     keys |= GB_BUTTON_A;
  if (!digitalRead(BTN_B))     keys |= GB_BUTTON_B;

  if (!digitalRead(BTN_START))  keys |= GB_BUTTON_START;
  if (!digitalRead(BTN_SELECT)) keys |= GB_BUTTON_SELECT;

  return keys;
}

// ---------------- VIDEO CALLBACK ----------------

void lcd_draw_line(struct gb_s* gb, const uint8_t* pixels, const uint_fast8_t line)
{
  for (int x = 0; x < 160; x++)
  {
    uint8_t c = pixels[x];

    uint16_t color;

    switch (c)
    {
      case 0: color = ILI9341_WHITE; break;
      case 1: color = ILI9341_LIGHTGREY; break;
      case 2: color = ILI9341_DARKGREY; break;
      default: color = ILI9341_BLACK; break;
    }

    framebuffer[line * 160 + x] = color;
  }

  if (line == 143)
  {
    tft.drawRGBBitmap(40, 20, framebuffer, 160, 144);
  }
}

// ---------------- AUDIO CALLBACK (optional) ----------------

void audio_callback(struct gb_s* gb, const int16_t* samples, uint_fast8_t count)
{
  // not implemented yet
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

  Serial.println("Starting GameBoy Emulator");

  gb_init(&gb, lcd_draw_line, audio_callback);

  gb_load_rom(&gb, game_rom);

}

// ---------------- LOOP ----------------

void loop()
{
  gb.direct.joypad = read_input();

  gb_run_frame(&gb);
}
