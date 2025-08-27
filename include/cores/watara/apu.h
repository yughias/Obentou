#ifndef __APU_H__
#define __APU_H__

#include "SDL2/SDL.h"

#include "types.h"

#define AUDIO_FREQ 44100
#define AUDIO_BUFFER_SIZE 4096
#define AUDIO_CYCLES_PER_SAMPLE (4e6 / AUDIO_FREQ)

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
    SDL_AudioDeviceID audio_dev;
    sample_t buffer[AUDIO_BUFFER_SIZE];
    int buffer_size;
    float push_reload;
    float push_counter;

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