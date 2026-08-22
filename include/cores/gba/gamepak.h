#ifndef __GAMEPAK_H__
#define __GAMEPAK_H__

#include "utils/archive.h"

#include "cores/gba/db.h"
#include "types.h"

#define GAMECODE_OFFSET 0xAC

#define N_WAIT_IDX 0
#define S_WAIT_IDX 1

typedef enum {NO_GAMEPAK = 0, GAMEPAK_ROM_ONLY, GAMEPAK_SRAM, GAMEPAK_FLASH, GAMEPAK_EEPROM} GAMEPAK_TYPE;

typedef struct gba_gamepak_t gba_gamepak_t;
typedef struct arm7tdmi_t arm7tdmi_t;

typedef struct gba_gamepak_t {
    GAMEPAK_TYPE type;
    size_t ROM_SIZE;
    u8* ROM;
    u8* savMemory;
    size_t savMemorySize;
    u32 savSizeMask;

    void* internalData;

    u8 waitstates[3][2];
    u8 sram_wait;
} gba_gamepak_t;

void gba_load_gamepak(gba_gamepak_t* gamepak, const archive_t* rom_archive);
void gba_setup_gamepak_type(gba_gamepak_t* gamepak);
void gba_setup_rom_only_memory(gba_gamepak_t* gamepak);
void gba_update_waitstates(gba_gamepak_t* gamepak, arm7tdmi_t* cpu, u16 waitcnt_reg);

void gba_setup_save_memory_with_db(gba_gamepak_t* gamepak, db_hash hash);
bool gba_rom_contains(u8* rom, const char* string, size_t rom_size);
void gba_free_gamepak(gba_gamepak_t* gamepak);

typedef struct byte_vec_t byte_vec_t;
void serialize_gba_gamepak_t(gba_gamepak_t*, byte_vec_t*);
u8* deserialize_gba_gamepak_t(gba_gamepak_t*, u8*, u8*);

#endif