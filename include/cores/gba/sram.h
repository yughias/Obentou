#ifndef __SRAM_H__
#define __SRAM_H__

#include "gamepak.h"
#include "types.h"

#define SRAM_SIZE (1 << 15)

void gba_sram_setup_memory(gba_gamepak_t* gamepak, size_t size);
u8 gba_sram_read(gba_gamepak_t* gamepak, u16 addr);
void gba_sram_write(gba_gamepak_t* gamepak, u16 addr, u8 byte);

#endif