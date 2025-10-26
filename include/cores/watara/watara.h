#ifndef __WATARA_H__
#define __WATARA_H__

#include "utils/serializer.h"

#include "cpus/w65c02.h"
#include "cores/watara/lcd.h"
#include "cores/watara/tmr.h"
#include "cores/watara/apu.h"
#include "cores/watara/dma.h"

#define CYCLES_BEFORE_NMI 65536
#define NMI_RATE 61.04
#define RAM_SIZE 0x2000

#define WATARA_STRUCT(X) \
    X(w65c02_t, cpu, 1, 1) \
    X(lcd_t, lcd, 1, 0) \
    X(tmr_t, tmr, 1, 0) \
    X(apu_t, apu, 1, 0) \
    X(dma_t, dma, 1, 0) \
    X(u8*, rom, 0, 0) \
    X(size_t, rom_size, 0, 0) \
    X(u8, ram, RAM_SIZE, 1, 0) \
    X(u8, ctrl, 1, 0) \
    X(u8, ddr, 1, 0) \
    X(u8, link, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(watara, WATARA_STRUCT);

#endif