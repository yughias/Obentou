#ifndef __WATARA_H__
#define __WATARA_H__

#include "cpus/w65c02.h"
#include "cores/watara/lcd.h"
#include "cores/watara/tmr.h"
#include "cores/watara/apu.h"
#include "cores/watara/dma.h"

#define CYCLES_BEFORE_NMI 65536
#define NMI_RATE 61.04
#define RAM_SIZE 0x2000

typedef struct watara_t {
    w65c02_t cpu;
    lcd_t lcd;
    tmr_t tmr;
    apu_t apu;
    dma_t dma;

    u8* rom;
    size_t rom_size;

    u8 ram[RAM_SIZE];

    // regs 
    u8 ctrl;
    u8 ddr;
    u8 link;
} watara_t;

#endif