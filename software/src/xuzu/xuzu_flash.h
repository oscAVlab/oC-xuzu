// xuzu preset drawer: 16 presets per app kept in the last sectors of the Teensy 3.2 program flash.
//
// Layout (each sector is 2 KB, 16 presets x 128 bytes):
//   0x3E000 sector 0  Compas
//   0x3E800 sector 1  Euclid x4
//   0x3F000 sector 2  Brains Seq
//   0x3F800 sector 3  reserved
// The first 6 KB (sectors 0-2) travel in the SysEx backup as packets 64-255 (32 bytes each).
//
// Flashing a new .hex with the Teensy Loader erases the whole program flash, presets included:
// backup -> flash -> restore.
#pragma once
#include <Arduino.h>
#include <string.h>

namespace xuzu {

static constexpr uint32_t FLASH_BASE = 0x3E000;
static constexpr uint32_t SECTOR = 2048;
static constexpr uint32_t PRESET_SIZE = 128;
static constexpr int NUM_PRESETS = 16;
static constexpr uint32_t BACKUP_BYTES = 3 * SECTOR;   // packets 64..255
enum Drawer : uint8_t { D_COMPAS = 0, D_EUCLID = 1, D_BRAINS = 2 };

// every preset starts with this header; an erased slot reads 0xFF
struct PresetHeader {
  uint16_t magic;    // 'X','Z'
  uint8_t version;
  uint8_t size;
};
static constexpr uint16_t PRESET_MAGIC = 0x5A58;

inline const uint8_t *PresetAddr(int drawer, int slot) {
  return (const uint8_t *)(FLASH_BASE + drawer * SECTOR + slot * PRESET_SIZE);
}
inline bool PresetValid(int drawer, int slot, uint8_t version) {
  const PresetHeader *h = (const PresetHeader *)PresetAddr(drawer, slot);
  return h->magic == PRESET_MAGIC && h->version == version;
}

// ---- low level: Kinetis FTFL commands, executed from RAM with interrupts off
FASTRUN static uint8_t FlashCommand() {
  __disable_irq();
  FTFL_FSTAT = FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_RDCOLERR;
  FTFL_FSTAT = FTFL_FSTAT_CCIF;
  while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF)) {}
  const uint8_t st = FTFL_FSTAT & (FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_MGSTAT0);
  __enable_irq();
  return st;
}
static bool FlashEraseSector(uint32_t addr) {
  while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF)) {}
  FTFL_FCCOB0 = 0x09;
  FTFL_FCCOB1 = (addr >> 16) & 0xFF; FTFL_FCCOB2 = (addr >> 8) & 0xFF; FTFL_FCCOB3 = addr & 0xFF;
  return FlashCommand() == 0;
}
static bool FlashProgramWord(uint32_t addr, uint32_t data) {
  while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF)) {}
  FTFL_FCCOB0 = 0x06;
  FTFL_FCCOB1 = (addr >> 16) & 0xFF; FTFL_FCCOB2 = (addr >> 8) & 0xFF; FTFL_FCCOB3 = addr & 0xFF;
  FTFL_FCCOB4 = (data >> 24) & 0xFF; FTFL_FCCOB5 = (data >> 16) & 0xFF;
  FTFL_FCCOB6 = (data >> 8) & 0xFF;  FTFL_FCCOB7 = data & 0xFF;
  return FlashCommand() == 0;
}

// one sector worth of RAM, shared by preset saving and SysEx restore
DMAMEM static uint32_t sector_buf[SECTOR / 4];

// writes the RAM sector buffer to the sector of `drawer`
static bool FlashWriteSector(int drawer) {
  const uint32_t addr = FLASH_BASE + drawer * SECTOR;
  if (!FlashEraseSector(addr)) return false;
  for (uint32_t i = 0; i < SECTOR / 4; ++i) {
    if (sector_buf[i] == 0xFFFFFFFFu) continue;           // erased already
    if (!FlashProgramWord(addr + i * 4, sector_buf[i])) return false;
  }
  // flush the flash read cache / prefetch buffer so new data is seen
  FMC_PFB0CR |= 0xF << 20;
  return memcmp((const void *)addr, sector_buf, SECTOR) == 0;
}

// saves `size` bytes (header included, filled in here) to a preset slot
static bool SavePreset(int drawer, int slot, uint8_t version, void *data, uint8_t size) {
  if (slot < 0 || slot >= NUM_PRESETS || size > PRESET_SIZE || size < sizeof(PresetHeader)) return false;
  PresetHeader *h = (PresetHeader *)data;
  h->magic = PRESET_MAGIC; h->version = version; h->size = size;
  memcpy(sector_buf, (const void *)(FLASH_BASE + drawer * SECTOR), SECTOR);
  uint8_t *dst = (uint8_t *)sector_buf + slot * PRESET_SIZE;
  memset(dst, 0xFF, PRESET_SIZE);
  memcpy(dst, data, size);
  return FlashWriteSector(drawer);
}
static bool LoadPreset(int drawer, int slot, uint8_t version, void *data, uint8_t size) {
  if (slot < 0 || slot >= NUM_PRESETS || !PresetValid(drawer, slot, version)) return false;
  const PresetHeader *h = (const PresetHeader *)PresetAddr(drawer, slot);
  if (h->size != size) return false;
  memcpy(data, PresetAddr(drawer, slot), size);
  return true;
}

// ---- SysEx backup helpers (byte offset 0..BACKUP_BYTES-1 inside the drawer area)
inline uint8_t BackupRead(uint32_t offset) { return *(const uint8_t *)(FLASH_BASE + offset); }
// restore: bytes land in the RAM buffer; when a sector is complete it is written
static uint8_t restore_sector = 0xFF;
static void RestoreBegin() { restore_sector = 0xFF; }
static bool RestoreWrite(uint32_t offset, const uint8_t *src, int n) {
  const int sec = offset / SECTOR;
  if (sec >= 3) return false;
  if (restore_sector != sec) { memset(sector_buf, 0xFF, SECTOR); restore_sector = sec; }
  memcpy((uint8_t *)sector_buf + (offset % SECTOR), src, n);
  if ((offset % SECTOR) + n >= SECTOR) { restore_sector = 0xFF; return FlashWriteSector(sec); }
  return true;
}

// CV preset select shared by the three apps: 0.25 V .. 5 V spread over the 16 presets;
// below 0.25 V nothing changes (so an unpatched jack never loads anything)
// returns -1 for "no change"
inline int CVPreset(int cv, int five_volts, int &last) {
  const int floor = five_volts / 20;
  if (cv < floor) { last = -1; return -1; }
  int v = (cv - floor) * NUM_PRESETS / (five_volts - floor + 1);
  if (v >= NUM_PRESETS) v = NUM_PRESETS - 1;
  // hysteresis: only move when clearly inside the next zone
  if (last >= 0 && v != last) {
    const int zone = (five_volts - floor) / NUM_PRESETS;
    const int center_last = floor + last * zone + zone / 2;
    const int d = cv > center_last ? cv - center_last : center_last - cv;
    if (d < zone * 6 / 10) v = last;
  }
  if (v == last) return -1;
  last = v;
  return v;
}

} // namespace xuzu
