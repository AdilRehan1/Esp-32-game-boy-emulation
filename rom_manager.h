#pragma once
#include <pgmspace.h>

// ROM structure
struct GameROM {
  const char* name;
  const unsigned char* data;
  uint32_t size;
};

// Include your ROM files
#include "Super_Mario_Land.h"
#include "Tetris.h"

// List of available games
const GameROM GAMES[] PROGMEM = {
  { "Super Mario Land", Super_Mario_Land, Super_Mario_Land_len },
  { "Tetris", Tetris, Tetris_len },
};

const int GAME_COUNT = sizeof(GAMES) / sizeof(GAMES[0]);
