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

#include "types.h"

#define CYCLES_PER_FRAME 70224
#define DIV_INCREMENT_RATE 256
#define REFRESH_RATE 59.727500569606 

typedef enum {DMG_TYPE, CGB_TYPE, MEGADUCK_TYPE, DMG_ON_CGB_TYPE} CONSOLE_TYPE;

typedef struct gb_t {
    CONSOLE_TYPE console_type;
    sm83_t cpu;
    uint64_t startFrame_clock;
    gb_timer_t timer;   
    dma_t dma;
    joypad_t joypad;
    serial_t serial;
    ppu_t ppu;
    apu_t apu;
    mbc_t mbc;

    readGbFunc readTable[0x100];
    writeGbFunc writeTable[0x100];

    u8 BOOTROM_DISABLE_REG;

    u8* BOOTROM;
    size_t BOOTROM_SIZE;
    
    u8* ROM;
    size_t ROM_SIZE;
    bool noCart;
    
    size_t ERAM_SIZE;
    u8 ERAM[MAX_ERAM_SIZE];
    
    u8 VRAM[VRAM_SIZE];
    u8 WRAM[WRAM_SIZE];
    u8 OAM[OAM_SIZE];
    u8 HRAM[HRAM_SIZE];
    
    u8 BGP_CRAM[CRAM_SIZE];
    u8 OBP_CRAM[CRAM_SIZE];
    
    u8 SVBK_REG;
    u8 VBK_REG;
    u8 KEY0_REG;
    u8 KEY1_REG;
} gb_t;

#endif