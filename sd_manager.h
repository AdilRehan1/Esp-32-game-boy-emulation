#pragma once
#include <Arduino.h>
#include <SD.h>
#include <FS.h>

// SD Card Pin Definitions
#define SD_CS       17
#define SD_MOSI     23
#define SD_MISO     19
#define SD_SCLK     18

// Game structure for SD card ROMs
struct SDGame {
  char name[64];
  char filename[64];
  uint32_t size;
};

// External declarations
extern SDGame sdGames[50];
extern int sdGameCount;
extern bool useSDCard;

// Function prototypes
void initSDCard();
uint8_t* loadROMFromSD(const char* filename, uint32_t* size);
void scanSDRoms();
void freeROMMemory();
