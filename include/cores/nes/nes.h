#ifndef __NES_H__
#define __NES_H__

#include "types.h"
#include "cores/nes/ines.h"
#include "cpus/m6502.h"
#include "cores/nes/ppu.h"
#include "cores/nes/memory.h"
#include "cores/nes/joypad.h"
#include "cores/nes/dma.h"
#include "cores/nes/apu.h"

#include "utils/serializer.h"

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 240
#define REFRESH_RATE 60.0988

#define RENDER_SCANLINES 240
#define SCANLINES 262
#define PPU_CYCLES_PER_SCANLINE 341
#define PPU_CYCLES_PER_FRAME (SCANLINES*PPU_CYCLES_PER_SCANLINE)

#define NES_STRUCT(X) \
    X(m6502_t, cpu, 1, 1) \
    X(ppu_t, ppu, 1, 1) \
    X(apu_t, apu, 1, 1) \
    X(ines_t, cart, 1, 1) \
    X(joypad_t, joypad, 1, 0) \
    X(dma_t, dma, 1, 0) \
    X(u8, ram, RAM_SIZE, 1, 0) \
    X(bool, cart_irq, 1, 0) \
    X(void*, mapper, 0, 0) \
    X(size_t, mapper_size, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(nes, NES_STRUCT);

void nes_reset(nes_t* nes);
void nes_sync(nes_t* nes);

#endif