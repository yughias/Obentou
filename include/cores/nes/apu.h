#ifndef __APU_H__
#define __APU_H__

#include "SDL2/SDL.h"

#include "types.h"

#define APU_CYCLES_PER_SECOND (1.789773e6 / 2)
#define SAMPLE_RATE 44100
#define SAMPLE_BUFFER_SIZE 4096
#define PUSH_RATE_RELOAD (APU_CYCLES_PER_SECOND / SAMPLE_RATE)
#define DISPLAY_BUFFER_SIZE 1024

typedef struct envelope_t {
    bool start_flag;
    bool mode;
    u8 decay;
    u8 divider;
    u8 volume;
} envelope_t;

typedef struct length_counter_t {
    u8 counter;
    bool halt;
} length_counter_t;

typedef struct pulse_t {
    bool enabled;
    u16 timer_reload;
    u16 timer;
    u8 duty_table;
    u8 duty_idx;
    bool sweep_enabled;
    bool sweep_negate;
    bool sweep_reload_flag;
    u8 sweep_p;
    u8 sweep_divider;
    u8 sweep_shift;
    length_counter_t length;
    envelope_t envelope;
} pulse_t;

typedef struct triangle_t {
    bool enabled;
    u16 timer_reload;
    u16 timer;
    u8 sample_idx;
    bool linear_reload_flag;
    u8 linear_counter;
    u8 linear_counter_reload;
    length_counter_t length;
} triangle_t;

typedef struct noise_t {
    bool enabled;
    bool mode;
    u16 timer_reload;
    u16 timer;
    u16 lfsr;
    length_counter_t length;
    envelope_t envelope;
} noise_t;

typedef struct dmc_t {
    bool irq;
    bool irq_enabled;
    bool loop;
    bool empty;
    u8 output;
    u8 bit_counter;
    u8 buffer;
    u8 shifter;
    u16 divider_reload;
    u16 divider;
    u16 address_reload;
    u16 address;
    u16 length_reload;
    u16 length;
    bool silence_flag;
} dmc_t;

typedef struct apu_t {
    SDL_AudioDeviceID audioDev;

    float buffer[SAMPLE_BUFFER_SIZE];
    int buffer_idx;

    float push_rate_counter;
    float push_rate_reload;

    pulse_t pulses[2];
    triangle_t triangle;
    noise_t noise;
    dmc_t dmc;

    u32 frame_counter;
    u8 sequencer_mode;

    bool mute[5];
    u8 display_buffers[5][DISPLAY_BUFFER_SIZE];
    int display_idx;

    bool inhibit_irq;
    bool frame_irq;

    void* ctx;
} apu_t;

void nes_apu_sync(apu_t* apu);
void nes_apu_push_sample(apu_t* apu);

float nes_apu_get_sample(apu_t* apu);
u8 nes_apu_get_pulse_sample(pulse_t* pulse, bool idx);
u8 nes_apu_get_triangle_sample(triangle_t* triangle);
u8 nes_apu_get_noise_sample(noise_t* noise);
u8 nes_apu_get_dmc_sample(dmc_t* dmc);

u8 nes_apu_get_status(apu_t* apu);

void nes_apu_clock_quarter_frame(apu_t* apu);
void nes_apu_clock_half_frame(apu_t* apu);
void nes_apu_clock_sweep_counters(apu_t* apu);
void nes_apu_clock_length_counters(apu_t* apu);
void nes_apu_clock_envelope_counters(apu_t* apu);
void nes_apu_clock_linear_counter(apu_t* apu);

void nes_apu_update_pulse(pulse_t* pulse);
void nes_apu_update_triangle(triangle_t* triangle);
void nes_apu_update_noise(noise_t* noise);
void nes_apu_update_dmc(apu_t* apu);

void nes_apu_write_pulse_0(pulse_t* pulse, u8 byte);
void nes_apu_write_pulse_1(pulse_t* pulse, u8 byte);
void nes_apu_write_pulse_2(pulse_t* pulse, u8 byte);
void nes_apu_write_pulse_3(pulse_t* pulse, u8 byte);

void nes_apu_write_triangle_0(triangle_t* triangle, u8 byte);
void nes_apu_write_triangle_1(triangle_t* triangle, u8 byte);
void nes_apu_write_triangle_2(triangle_t* triangle, u8 byte);

void nes_apu_write_noise_0(noise_t* noise, u8 byte);
void nes_apu_write_noise_1(noise_t* noise, u8 byte);
void nes_apu_write_noise_2(noise_t* noise, u8 byte);

void nes_apu_write_dmc_0(dmc_t* dmc, u8 byte);
void nes_apu_write_dmc_1(dmc_t* dmc, u8 byte);
void nes_apu_write_dmc_2(dmc_t* dmc, u8 byte);
void nes_apu_write_dmc_3(dmc_t* dmc, u8 byte);

void nes_apu_update_length_counter(length_counter_t* length);
void nes_apu_update_envelope(envelope_t* envelope, bool halt);

void nes_apu_write_control(apu_t* apu, u8 byte);
void nes_apu_write_mi_reg(apu_t* apu, u8 byte);

u16 nes_apu_get_sweep_target(pulse_t* pulse, bool idx);

void nes_apu_draw_waves(apu_t* apu, SDL_Window** win);
void nes_apu_draw_wave(int x0, int y0, u8* buffer, int scale, SDL_Surface* s);

#endif