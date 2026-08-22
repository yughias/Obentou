#include "cores/gba/sram.h"

#include <stdio.h>
#include <stdlib.h>

void gba_sram_setup_memory(gba_gamepak_t* gamepak, size_t size){
    gamepak->type = GAMEPAK_SRAM;
    gamepak->savMemorySize = size;
    gamepak->savMemory = (u8*)calloc(1, size);
    gamepak->savSizeMask = size - 1;
    printf("SRAM DETECTED!\n");
}

u8 gba_sram_read(gba_gamepak_t* gamepak, u16 addr){
    return gamepak->savMemory[addr & gamepak->savSizeMask];
}

void gba_sram_write(gba_gamepak_t* gamepak, u16 addr, u8 byte){
    gamepak->savMemory[addr & gamepak->savSizeMask] = byte;
}
