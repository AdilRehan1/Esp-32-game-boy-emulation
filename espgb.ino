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

//////////////////// WIFI ////////////////////

const char* ssid = "ESP32-GameBoy";
const char* password = "12345678";

WebServer server(80);
uint8_t wifi_keys = 0;

//////////////////// DISPLAY ////////////////////

#define TFT_CS  5
#define TFT_DC  2
#define TFT_RST 4

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

//////////////////// BUTTONS ////////////////////

#define BTN_UP     32
#define BTN_DOWN   33
#define BTN_LEFT   15
#define BTN_RIGHT  16
#define BTN_A      25
#define BTN_B      26
#define BTN_START  27
#define BTN_SELECT 22

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

//////////////////// PHYSICAL INPUT ////////////////////

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

//////////////////// WIFI BUTTON HANDLERS ////////////////////

void pressUp()    { wifi_keys |= JOYPAD_UP; server.send(200,"text/plain","OK"); }
void pressDown()  { wifi_keys |= JOYPAD_DOWN; server.send(200,"text/plain","OK"); }
void pressLeft()  { wifi_keys |= JOYPAD_LEFT; server.send(200,"text/plain","OK"); }
void pressRight() { wifi_keys |= JOYPAD_RIGHT; server.send(200,"text/plain","OK"); }

void pressA()     { wifi_keys |= JOYPAD_A; server.send(200,"text/plain","OK"); }
void pressB()     { wifi_keys |= JOYPAD_B; server.send(200,"text/plain","OK"); }

void pressStart() { wifi_keys |= JOYPAD_START; server.send(200,"text/plain","OK"); }
void pressSelect(){ wifi_keys |= JOYPAD_SELECT; server.send(200,"text/plain","OK"); }

void releaseKeys(){ wifi_keys = 0; server.send(200,"text/plain","OK"); }

//////////////////// WEB PAGE ////////////////////

void handleRoot()
{

String page = R"rawliteral(

<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>

body{
text-align:center;
font-family:sans-serif;
}

button{
width:80px;
height:80px;
font-size:20px;
margin:5px;
}

</style>
</head>
<body>

<h2>ESP32 GameBoy Controller</h2>

<div>
<button ontouchstart="press('/up')" ontouchend="release()">UP</button><br>
<button ontouchstart="press('/left')" ontouchend="release()">LEFT</button>
<button ontouchstart="press('/right')" ontouchend="release()">RIGHT</button><br>
<button ontouchstart="press('/down')" ontouchend="release()">DOWN</button>
</div>

<br>

<div>
<button ontouchstart="press('/a')" ontouchend="release()">A</button>
<button ontouchstart="press('/b')" ontouchend="release()">B</button>
</div>

<br>

<div>
<button ontouchstart="press('/start')" ontouchend="release()">START</button>
<button ontouchstart="press('/select')" ontouchend="release()">SELECT</button>
</div>

<script>

function press(cmd)
{
  fetch(cmd);
}

function release()
{
  fetch('/release');
}

</script>

</body>
</html>

)rawliteral";

server.send(200,"text/html",page);

}

//////////////////// SETUP ////////////////////

void setup()
{

Serial.begin(115200);

pinMode(BTN_UP,INPUT_PULLUP);
pinMode(BTN_DOWN,INPUT_PULLUP);
pinMode(BTN_LEFT,INPUT_PULLUP);
pinMode(BTN_RIGHT,INPUT_PULLUP);

pinMode(BTN_A,INPUT_PULLUP);
pinMode(BTN_B,INPUT_PULLUP);
pinMode(BTN_START,INPUT_PULLUP);
pinMode(BTN_SELECT,INPUT_PULLUP);

tft.begin();
tft.setRotation(1);
tft.fillScreen(0x0000);

//////////////// WIFI //////////////////

WiFi.softAP(ssid,password);

Serial.println("WiFi started");
Serial.println(WiFi.softAPIP());

//////////////// SERVER //////////////////

server.on("/",handleRoot);

server.on("/up",pressUp);
server.on("/down",pressDown);
server.on("/left",pressLeft);
server.on("/right",pressRight);

server.on("/a",pressA);
server.on("/b",pressB);

server.on("/start",pressStart);
server.on("/select",pressSelect);

server.on("/release",releaseKeys);

server.begin();

//////////////// EMULATOR //////////////////

enum gb_init_error_e ret = gb_init(
&gb,
rom_read,
cart_ram_read,
cart_ram_write,
gb_error,
NULL
);

if(ret!=GB_INIT_NO_ERROR)
{
Serial.println("gb_init failed");
while(1);
}

gb_init_lcd(&gb,lcd_draw);

memset(framebuffer,0,sizeof(framebuffer));

}

//////////////////// LOOP ////////////////////

void loop()
{

server.handleClient();

uint32_t frame_start = millis();

uint8_t raw = read_input();

// 🔥 FIXED INPUT HANDLING
uint8_t combined = raw | wifi_keys;
combined &= 0xFF;
gb.direct.joypad = ~combined;

gb_run_frame(&gb);

uint32_t elapsed = millis() - frame_start;

if(elapsed < FRAME_TIME_MS)
delay(FRAME_TIME_MS - elapsed);
else
delay(1);

}
