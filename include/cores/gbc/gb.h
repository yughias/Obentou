#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include "cpus/sm83.h"
#include "cores/gbc/ppu.h"
#include "cores/gbc/memory.h"
#include "cores/gbc/joypad.h"
#include "cores/gbc/dma.h"
#include "cores/gbc/mbc.h"
#include "cores/gbc/apu.h"
#include "cores/gbc/gb_timer.h"
#include "cores/gbc/serial.h"

#include "utils/serializer.h"

#include "types.h"

#define CYCLES_PER_FRAME 70224
#define DIV_INCREMENT_RATE 256
#define REFRESH_RATE 59.727500569606 

typedef enum {DMG_TYPE, CGB_TYPE, MEGADUCK_TYPE, DMG_ON_CGB_TYPE} CONSOLE_TYPE;

#define GB_STRUCT(X) \
    X(CONSOLE_TYPE, console_type, 1, 0) \
    X(sm83_t, cpu, 1, 1) \
    X(uint64_t, startFrame_clock, 1, 0) \
    X(gb_timer_t, timer, 1, 0) \
    X(dma_t, dma, 1, 0) \
    X(joypad_t, joypad, 1, 0) \
    X(serial_t, serial, 1, 0) \
    X(ppu_t, ppu, 1, 0) \
    X(apu_t, apu, 1, 0) \
    X(mbc_t, mbc, 1, 1) \
    X(readGbFunc, readTable[0x100], 0, 0) \
    X(writeGbFunc, writeTable[0x100], 0, 0) \
    X(u8*, BOOTROM, 0, 0) \
    X(size_t, BOOTROM_SIZE, 0, 0) \
    X(bool, BOOTROM_ENABLED, 1, 0) \
    X(u8*, ROM, 0, 0) \
    X(size_t, ROM_SIZE, 0, 0) \
    X(bool, noCart, 0, 0) \
    X(size_t, ERAM_SIZE, 0, 0) \
    X(u8, ERAM, MAX_ERAM_SIZE, 1, 0) \
    X(u8, VRAM, VRAM_SIZE, 1, 0) \
    X(u8, WRAM, WRAM_SIZE, 1, 0) \
    X(u8, OAM, OAM_SIZE, 1, 0) \
    X(u8, HRAM, HRAM_SIZE, 1, 0) \
    X(u8, BGP_CRAM, CRAM_SIZE, 1, 0) \
    X(u8, OBP_CRAM, CRAM_SIZE, 1, 0) \
    X(u8, SVBK_REG, 1, 0) \
    X(u8, VBK_REG, 1, 0) \
    X(u8, KEY0_REG, 1, 0) \
    X(u8, KEY1_REG, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(gb, GB_STRUCT)

#endif