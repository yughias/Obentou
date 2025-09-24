#ifndef __INES_H__
#define __INES_H__

#include "types.h"

typedef enum VRAM_ALIGN {VRAM_UND, VRAM_1_LO, VRAM_1_HI, VRAM_H, VRAM_V, VRAM_4} VRAM_ALIGN;

typedef struct ines_t {
    u8 mapper;
    VRAM_ALIGN vram_align;

    size_t trainer_size;
    u8* trainer;
    
    size_t prg_size;
    u8* prg;

    size_t chr_size;
    u8* chr;
    bool is_chr_ram;

    size_t prg_ram_size;
    u8* prg_ram;
} ines_t;

void nes_ines_load(ines_t* ines, u8* file, size_t size);

#endif