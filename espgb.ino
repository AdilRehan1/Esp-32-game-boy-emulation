
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

extern "C" {
  #include "peanut_gb.h"
}

#include "game_rom.h"

/* ================= TFT ================= */
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

/* ============== BUTTONS (FIXED) ============== */
#define BTN_UP      32
#define BTN_DOWN    33
#define BTN_LEFT    12
#define BTN_RIGHT   13
#define BTN_A       25
#define BTN_B       26
#define BTN_START   27
#define BTN_SELECT  14

gb_t gb;

/* ============== FRAMEBUFFER ============== */
uint16_t framebuffer[160 * 144];

/* ============== DISPLAY CALLBACK ============== */
void lcd_draw_line(uint8_t y, uint8_t *pixels) {
  for (int x = 0; x < 160; x++) {
    uint8_t p = pixels[x];

    // Simple 4-shade grayscale palette
    uint16_t color;
    switch (p) {
      case 0: color = ILI9341_WHITE; break;
      case 1: color = ILI9341_LIGHTGREY; break;
      case 2: color = ILI9341_DARKGREY; break;
      default: color = ILI9341_BLACK; break;
    }

    framebuffer[y * 160 + x] = color;
  }
}

/* ============== SETUP BUTTONS ============== */
void setupButtons() {
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  setupButtons();

  // Initialize emulator
  gb_init(&gb, game_gb, game_gb_len);

  // Set LCD callback
  gb.display.callback = lcd_draw_line;
}

/* ================= LOOP ================= */
void loop() {

  // Button mapping
  gb.direct.joypad_bits.up     = (digitalRead(BTN_UP)     == LOW);
  gb.direct.joypad_bits.down   = (digitalRead(BTN_DOWN)   == LOW);
  gb.direct.joypad_bits.left   = (digitalRead(BTN_LEFT)   == LOW);
  gb.direct.joypad_bits.right  = (digitalRead(BTN_RIGHT)  == LOW);
  gb.direct.joypad_bits.a      = (digitalRead(BTN_A)      == LOW);
  gb.direct.joypad_bits.b      = (digitalRead(BTN_B)      == LOW);
  gb.direct.joypad_bits.start  = (digitalRead(BTN_START)  == LOW);
  gb.direct.joypad_bits.select = (digitalRead(BTN_SELECT) == LOW);

  // Run one frame
  gb_run_frame(&gb);

  // Push framebuffer to TFT
  tft.startWrite();
  tft.setAddrWindow(40, 48, 160, 144);  // Centered on 320x240
  tft.pushColors(framebuffer, 160 * 144, true);
  tft.endWrite();
}
