#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include "cpus/arm7tdmi/arm7tdmi.h"
#include "cores/gba/memory.h"
#include "cores/gba/ppu.h"
#include "cores/gba/keypad.h"
#include "cores/gba/dma.h"
#include "cores/gba/timer.h"
#include "cores/gba/gamepak.h"
#include "cores/gba/prefetcher.h"
#include "cores/gba/apu.h"
#include "cores/gba/scheduler.h"

#include "utils/serializer.h"

// 1 PPU
// 4 TIMERS
// 1 SWEEP FOR CHANNEL 1
// 2*3 TONE CHANNEL: LENGTH + UPDATE TONE + ENVELOPE
// 2 WAVE CHANNEL: LENGTH + UPDATE WAVE
// 3 NOISE CHANNEL: LENGTH + UPDATE LFSR + ENVELOPE
// 1 push sample to audio device 
#define GBA_SCHEDULER_POOL_SIZE 18

#define GBA_STRUCT(X) \
    X(arm7tdmi_t, cpu, 1, 1) \
    X(ppu_t, ppu, 1, 0) \
    X(apu_t, apu, 1, 1) \
    X(prefetcher_t, prefetcher, 1, 0) \
    X(u8, EWRAM, EWRAM_SIZE, 1, 0) \
    X(u8, IWRAM, IWRAM_SIZE, 1, 0) \
    X(bios_t, bios, 1, 1) \
    X(gba_gamepak_t, gamepak, 1, 1) \
    X(gba_tmr_t, timers, 4, 1, 1) \
    X(DMA_IDX, active_dma, 1, 0) \
    X(dma_t, dmas, 4, 1, 0) \
    X(u16, IE, 1, 0) \
    X(u16, IF, 1, 0) \
    X(u16, IME, 1, 0) \
    X(bool, HALTCNT, 1, 0) \
    X(u16, KEYCNT, 1, 0) \
    X(u16, RCNT, 1, 0) \
    X(u8, POSTFLG, 1, 0) \
    X(u16, WAITCNT, 1, 0) \
    X(gba_scheduler_t, scheduler_pool, GBA_SCHEDULER_POOL_SIZE, 1, 1) \
    X(gba_scheduler_ptr_t, scheduler_head, 1, 1) \
    X(u64, frame_clock, 1, 0) \
    X(u32, clock_before_scheduling, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(gba, GBA_STRUCT);

void gba_check_interrupts(gba_t* gba);
void gba_reset(gba_t* gba);

#endif