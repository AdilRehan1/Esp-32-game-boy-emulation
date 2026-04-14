#include "sd_manager.h"
#include <SPI.h>

// Global variables
SDGame sdGames[50];
int sdGameCount = 0;
bool useSDCard = true;

// Store current ROM pointer for freeing
static const unsigned char* currentROM = nullptr;

void initSDCard() {
  // Initialize SPI with proper pins
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  
  Serial.println("Initializing SD card...");
  
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
    Serial.println("Check wiring:");
    Serial.println("  - CS pin: " + String(SD_CS));
    Serial.println("  - MOSI pin: " + String(SD_MOSI));
    Serial.println("  - MISO pin: " + String(SD_MISO));
    Serial.println("  - SCK pin: " + String(SD_SCLK));
    useSDCard = false;
    return;
  }
  
  Serial.println("SD Card initialized successfully!");
  
  // Get SD card info
  uint32_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %d MB\n", cardSize);
  
  // Check if /games directory exists, create if not
  if (!SD.exists("/games")) {
    Serial.println("Creating /games directory...");
    if (!SD.mkdir("/games")) {
      Serial.println("Failed to create /games directory!");
      useSDCard = false;
      return;
    }
  }
  
  // Scan for ROMs
  scanSDRoms();
}

void scanSDRoms() {
  if (!useSDCard) return;
  
  sdGameCount = 0;
  
  File dir = SD.open("/games");
  if (!dir) {
    Serial.println("Failed to open /games directory");
    useSDCard = false;
    return;
  }
  
  Serial.println("Scanning for ROM files...");
  
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    
    if (!entry.isDirectory()) {
      String filename = entry.name();
      String ext = filename.substring(filename.lastIndexOf(".") + 1);
      ext.toLowerCase();
      
      // Check for Game Boy ROM extensions
      if (ext == "gb" || ext == "gbc") {
        // Remove extension for display name
        String displayName = filename.substring(0, filename.lastIndexOf("."));
        
        // Truncate if too long
        if (displayName.length() > 60) {
          displayName = displayName.substring(0, 57) + "...";
        }
        
        strcpy(sdGames[sdGameCount].filename, filename.c_str());
        strcpy(sdGames[sdGameCount].name, displayName.c_str());
        sdGames[sdGameCount].size = entry.size();
        sdGameCount++;
        
        Serial.printf("  [%d] %s (%d bytes)\n", sdGameCount, displayName.c_str(), entry.size());
        
        if (sdGameCount >= 50) {
          Serial.println("Maximum games reached (50)");
          break;
        }
      }
    }
    entry.close();
  }
  dir.close();
  
  if (sdGameCount == 0) {
    Serial.println("No Game Boy ROMs found in /games folder");
    Serial.println("Please add .gb or .gbc files to the /games folder on your SD card");
  } else {
    Serial.printf("Found %d game(s) on SD card\n", sdGameCount);
  }
}

uint8_t* loadROMFromSD(const char* filename, uint32_t* size) {
  if (!useSDCard) {
    Serial.println("SD Card not available");
    return nullptr;
  }
  
  char path[128];
  snprintf(path, sizeof(path), "/games/%s", filename);
  
  Serial.printf("Loading ROM: %s\n", path);
  
  // Check if file exists
  if (!SD.exists(path)) {
    Serial.printf("File not found: %s\n", path);
    return nullptr;
  }
  
  File file = SD.open(path, FILE_READ);
  if (!file) {
    Serial.printf("Failed to open file: %s\n", path);
    return nullptr;
  }
  
  *size = file.size();
  
  // Check if ROM is too large
  if (*size > 8 * 1024 * 1024) {  // 8MB max
    Serial.printf("ROM too large: %d bytes (max 8MB)\n", *size);
    file.close();
    return nullptr;
  }
  
  // Allocate memory for ROM
  uint8_t* buffer = (uint8_t*)malloc(*size);
  if (!buffer) {
    Serial.println("Failed to allocate memory for ROM");
    Serial.printf("Required: %d bytes, Free heap: %d bytes\n", *size, ESP.getFreeHeap());
    file.close();
    return nullptr;
  }
  
  // Read ROM data
  size_t bytesRead = file.read(buffer, *size);
  file.close();
  
  if (bytesRead != *size) {
    Serial.printf("Read error: expected %d bytes, got %d\n", *size, bytesRead);
    free(buffer);
    return nullptr;
  }
  
  // Free previous ROM if exists
  freeROMMemory();
  currentROM = buffer;
  
  Serial.printf("Successfully loaded: %s (%d bytes)\n", filename, *size);
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  
  return buffer;
}

void freeROMMemory() {
  if (currentROM != nullptr) {
    free((void*)currentROM);
    currentROM = nullptr;
    Serial.println("Freed previous ROM memory");
  }
}

// Helper function to get game count
int getGameCount() {
  return sdGameCount;
}

// Helper function to get game name by index
const char* getGameName(int index) {
  if (index >= 0 && index < sdGameCount) {
    return sdGames[index].name;
  }
  return nullptr;
}

// Helper function to get game filename by index
const char* getGameFilename(int index) {
  if (index >= 0 && index < sdGameCount) {
    return sdGames[index].filename;
  }
  return nullptr;
}

// Helper function to check if SD card is available
bool isSDCardAvailable() {
  return useSDCard && sdGameCount > 0;
}
