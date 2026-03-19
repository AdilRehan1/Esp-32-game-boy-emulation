#  ESP32 GameBoy Emulator (WiFi Controlled)

A **GameBoy emulator running on ESP32** with a TFT display, powered by the lightweight **Peanut-GB emulator**, and controlled wirelessly using your phone over WiFi.

---

##  Features

*  GameBoy emulation on ESP32
*  **Wireless control via phone (WiFi web interface)**
*  ILI9341 TFT display support
*  ~60 FPS emulation loop
*  4-color GameBoy palette rendering
*  No physical buttons required

---

##  How It Works

* The ESP32 runs a GameBoy emulator using `peanut_gb.h`
* A ROM is stored as a `.h` file (`game_rom.h`)
* The ESP32 creates its own WiFi hotspot
* Your phone connects to it and opens a web controller
* Button presses from the webpage are sent to the ESP32 in real-time

---

##  Hardware Required

* ESP32
* ILI9341 TFT Display (SPI)
* Jumper wires

---

##  Wiring (ILI9341 → ESP32)

| TFT Pin | ESP32 Pin |
| ------- | --------- |
| VCC     | 3.3V      |
| GND     | GND       |
| CS      | GPIO 5    |
| DC      | GPIO 2    |
| RST     | GPIO 4    |
| MOSI    | GPIO 23   |
| SCK     | GPIO 18   |
| LED     | 3.3V      |

---

##  Project Structure

```
ESP32-GameBoy/
│── gbESP.ino        # Main emulator code
│── peanut_gb.h      # Emulator core
│── game_rom.h       # Converted ROM file
│── README.md
```

---

##  Setup Instructions

### 1. Install Arduino Libraries

Install:

* `Adafruit GFX`
* `Adafruit ILI9341`
* `WiFi` (comes with ESP32 core)

---

### 2. Convert Game ROM

Convert your `.gb` file into a header file:

```bash
xxd -i game.gb > game_rom.h
```

Then rename the array inside to:

```c
const unsigned char game_rom[] = { ... };
```

---

### 3. Upload Code

* Open `gbESP.ino` in Arduino IDE
* Select **ESP32 board**
* Upload the code

---

### 4. Connect to ESP32

* Connect your phone to:

```
SSID: ESP32-GameBoy
Password: 12345678
```

* Open browser and go to:

```
http://192.168.4.1
```

---

##  Controls

| Button  | Function  |
| ------- | --------- |
| ↑ ↓ ← → | Movement  |
| A       | Action    |
| B       | Secondary |
| START   | Start     |
| SELECT  | Select    |

---

## Web Controller

* Runs directly from ESP32
* No app needed
* Works on any phone or browser

---

##  Limitations

* Only original GameBoy games (no GBC yet)
* Limited by ESP32 performance
* Audio not implemented
* Uses 4-shade grayscale palette

---

##  Future Improvements

*  Add sound support
*  GameBoy Color (GBC) support
*  Bluetooth controller support
*  Save game support
*  Performance optimization

---

##  Credits

* Peanut GB Emulator
* Adafruit GFX Library
* ESP32 Arduino Core

---

##  License
This project is open-source. Use it for learning and personal projects.

---

---
