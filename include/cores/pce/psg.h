#ifndef __PSG_H__
#define __PSG_H__

#define PSG_CHANNELS 6

#include "types.h"
#include "cores/pce/timings.h"

#include <SDL2/SDL.h>

typedef enum PSG_REGS_IDX {
    PSG_SELECTOR = 0,
    PSG_GLOBAL_VOL,
    PSG_FINE_FREQ,
    PSG_COARSE_FREQ,
    PSG_CTRL,
    PSG_BALANCE,
    PSG_SOUND_DATA,
    PSG_NOISE,
    PSG_LFO_FREQ,
    PSG_LFO_CTRL
} PSG_REGS_IDX;

typedef struct ch_t {
    u8 dac;
    u16 freq;
    u8 ctrl;
    u8 balance;
    u8 data[32];
    u8 idx;
    u8 left_vol;
    u8 right_vol;
    int freq_counter;
} ch_t;

typedef struct lfsr_t {
    bool on;
    u16 freq;
    u32 seed;
} lfsr_t;

typedef struct sample_t {
    float left;
    float right;
} sample_t;

typedef struct psg_t {
    u8 selector;
    u8 left_vol;
    u8 right_vol;

    ch_t ch[PSG_CHANNELS];
    lfsr_t lfsr[2];
} psg_t;

void pce_psg_write(psg_t* psg, u8 offset, u8 value);
void pce_psg_step(psg_t* psg, u32 cycles);

void pce_psg_draw_waveforms(SDL_Window** win, psg_t* psg);

#endif