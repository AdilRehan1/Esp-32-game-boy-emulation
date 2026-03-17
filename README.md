# ESP32 Game Boy Emulator (ILI9341 + Peanut-GB)

A simple **Game Boy emulator handheld prototype** built using an **ESP32-WROVER**, a **ILI9341 TFT display**, and the **Peanut-GB emulator core**.
The project runs a Game Boy ROM directly from flash memory and renders it to the TFT display while reading input from physical buttons.

This project is intended as a **learning and experimentation platform** for embedded systems, emulator design, and DIY handheld consoles.

---

# Features

* Runs original **Nintendo Game Boy ROMs**
* Uses the lightweight **Peanut-GB emulator**
* Displays graphics on a **ILI9341 320×240 TFT**
* Button input mapped to Game Boy controls
* ROM compiled directly into firmware
* ~60 FPS frame pacing

---

# Hardware Used

| Component        | Description                     |
| ---------------- | ------------------------------- |
| ESP32-WROVER     | Main microcontroller with PSRAM |
| ILI9341 TFT      | 320×240 SPI display             |
| Push Buttons     | Game controls                   |
| Breadboard / PCB | Prototyping                     |

---

# Pinout

## TFT Display (ILI9341)

| TFT Pin | ESP32 Pin |
| ------- | --------- |
| VCC     | 3.3V      |
| GND     | GND       |
| CS      | GPIO 5    |
| DC      | GPIO 2    |
| RST     | GPIO 4    |
| MOSI    | GPIO 23   |
| MISO    | GPIO 19   |
| SCK     | GPIO 18   |

---

## Buttons

Buttons use **INPUT_PULLUP**, so each button connects:

```
GPIO ---- Button ---- GND
```

| Button | ESP32 Pin |
| ------ | --------- |
| UP     | GPIO 32   |
| DOWN   | GPIO 33   |
| LEFT   | GPIO 15   |
| RIGHT  | GPIO 13   |
| A      | GPIO 25   |
| B      | GPIO 26   |
| START  | GPIO 27   |
| SELECT | GPIO 14   |

---

# Project Structure

```
gbESP/
│
├── gbESP.ino        # Main firmware
├── peanut_gb.h      # Game Boy emulator core
├── game_rom.h       # Converted ROM file
└── README.md
```

---

# Converting a Game Boy ROM

The emulator loads ROMs as **C arrays**.
```
file = input("Enter the path to the game ROM file: ")
with open(file, "rb") as f:
    data = f.read()

with open("game_rom.h", "w") as out:
    out.write("const unsigned char game_rom[] = {\n")

    for i, b in enumerate(data):
        if i % 12 == 0:
            out.write("\n ")
        out.write(f"0x{b:02x}, ")

    out.write("\n};\n")
    out.write(f"const unsigned int game_rom_len = {len(data)};\n")
```
# Arduino IDE Setup

Install:

* ESP32 board support
* Adafruit GFX library
* Adafruit ILI9341 library

Board settings:

```
Board: ESP32 Dev Module
Flash Size: 4MB
Partition Scheme: Huge APP (3MB No OTA)
PSRAM: Enabled
```

---

# How It Works

1. **ROM Access**

The emulator reads ROM data through a callback:

```python
uint8_t rom_read(struct gb_s* gb, const uint_fast32_t addr)
{ return game_rom[addr]; }
```

---

2. **Rendering**

Each Game Boy scanline is converted into RGB565 pixels and stored in a framebuffer.
Once the frame finishes rendering, it is drawn to the TFT display.

Game Boy resolution:

```
160 × 144
```

TFT resolution:

```
320 × 240
```

The frame is centered on the screen.

---

3. **Input Handling**

Button presses are converted into Game Boy joypad bits:

```
RIGHT
LEFT
UP
DOWN
A
B
START
SELECT
```

These values are sent to the emulator each frame.

---

4. **Frame Timing**

Game Boy refresh rate:

```
~59.7 FPS
```

The loop maintains this rate using a **16 ms frame delay**.

---

# Current Limitations

* Audio not implemented
* Only one ROM compiled at a time
* No save states
* No SD card ROM loader
* No Game Boy Color support yet

---

# Possible Improvements

Future upgrades could include:

* I2S audio output
* SD card ROM browser
* Game Boy Color support
* Battery powered handheld build
* Full screen scaling
* Save states
* Menu system

---

# Credits

Game Boy emulator core:

**Peanut-GB**

Display libraries:

* Adafruit GFX
* Adafruit ILI9341

---
