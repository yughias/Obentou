#ifndef __APU_H__
#define __APU_H__

#include "SDL3/SDL.h"

#include "types.h"

#define DISPLAY_BUFFER_SIZE 1024

typedef u8 (*apu_read_func)(void* ctx, u16 addr);

typedef struct sample_t {
    union {
        i8 data[2];
        struct {
            i8 l;
            i8 r;
        };
    };
} sample_t;

typedef struct wave_t {
    u8 flow;
    u8 fhi;
    u8 vol_duty;
    u8 length;

    int duty_phase;
    int freq_counter;
} wave_t;

typedef struct ch3_t {
    u8 addr_lo;
    u8 addr_hi;
    u8 length;
    u8 ctrl;

    u8 byte_counter;
    u8 nibble_counter;
    u8 sample;
    
    bool trigger;
    bool dma_active;
    bool irq;

    int freq_counter;
} ch3_t;

typedef struct noise_t {
    u8 freq_vol;
    u8 length;
    u8 ctrl;

    u16 lfsr;
    int freq_counter;
} noise_t;

typedef struct apu_t {
    wave_t waves[2];
    ch3_t ch3;
    noise_t noise;

    int len_counter;

    apu_read_func read;

    void* ctx;

    u8 display_buffers[4][DISPLAY_BUFFER_SIZE];
    int display_idx;
} apu_t;

void watara_apu_step(apu_t* apu, int cycles);
void watara_apu_push_sample(apu_t* apu, int cycles);
void watara_apu_draw_waves(apu_t* apu, SDL_Window** win);

#endif