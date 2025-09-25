#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "types.h"
#include "peripherals/archive.h"

#define BOOTROM_DISABLE_ADDR 0xFF50

#define VRAM_SIZE 0x4000
#define WRAM_SIZE 0x8000
#define OAM_SIZE  0xA0
#define HRAM_SIZE 0x7F
#define MAX_ERAM_SIZE 0x80000
#define CRAM_SIZE 64

#define BOOTROM_START_ADDR 0x0000
#define ROM_START_ADDR 0x0000
#define VRAM_START_ADDR 0x8000
#define WRAM_START_ADDR 0xC000
#define OAM_START_ADDR 0xFE00
#define HRAM_START_ADDR 0xFF80

typedef struct gb_t gb_t;

void gb_initMemory(gb_t*, const archive_t* rom_archive, const archive_t* bios_archive);
void gb_loadRom(gb_t*, const archive_t* rom_archive);
void gb_loadBootRom(gb_t*, file_t* file);
void gb_loadSav(gb_t*, const char*);
void gb_saveSav(gb_t*, const char*);
void gb_freeMemory(gb_t* gb);

u8 gb_readByte(void*, u16);
void gb_writeByte(void*, u16, u8);

#endif