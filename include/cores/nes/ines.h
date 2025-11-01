#ifndef __INES_H__
#define __INES_H__

#include "types.h"

#include "utils/serializer.h"

typedef enum VRAM_ALIGN {VRAM_UND, VRAM_1_LO, VRAM_1_HI, VRAM_H, VRAM_V, VRAM_4} VRAM_ALIGN;

#define INES_STRUCT(X) \
    X(u8, mapper, 0, 0) \
    X(VRAM_ALIGN, vram_align, 1, 0) \
    X(size_t, trainer_size, 0, 0) \
    X(u8*, trainer, 0, 0) \
    X(size_t, prg_size, 0, 0) \
    X(u8*, prg, 0, 0) \
    X(size_t, chr_size, 0, 0) \
    X(u8*, chr, 0, 0) \
    X(bool, is_chr_ram, 0, 0) \
    X(size_t, prg_ram_size, 0, 0) \
    X(u8*, prg_ram, 0, 0)

DECLARE_SERIALIZABLE_STRUCT(ines, INES_STRUCT)

void nes_ines_load(ines_t* ines, u8* file, size_t size);

#endif