#pragma once
#include "rom_tetris.h"
#include "rom_mario.h"
#include "rom_kirby.h"

struct RomEntry {
  const char*    name;
  const uint8_t* data;
  uint32_t       size;
};

// ── ADD YOUR ROMS HERE ──────────────────────────
static const RomEntry ROM_LIST[] = {
  { "Tetris",          rom_tetris, sizeof(rom_tetris) },
  { "Super Mario Land", rom_mario,  sizeof(rom_mario)  },
  { "Kirby",           rom_kirby,  sizeof(rom_kirby)  },
};
// ────────────────────────────────────────────────

static const int ROM_COUNT = sizeof(ROM_LIST) / sizeof(ROM_LIST[0]);
