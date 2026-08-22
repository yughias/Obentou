#ifndef __APU_H__
#define __APU_H__

#include <stdatomic.h>

#include "types.h"

#include <SDL3/SDL.h>

#include "cores/gba/scheduler.h"

#include "utils/serializer.h"

#define APU_FIFO_LENGTH 32
#define APU_FIFO_REQUEST 16
#define BASE_FIFO_ADDR 0x040000A0
#define WAVE_RAM_SIZE 32

typedef struct sample_t {
    i16 left;
    i16 right;
} sample_t;

typedef struct fifo_t {
    u8 data[APU_FIFO_LENGTH];
    u8 size;
    u8 r_idx;
    u8 w_idx;
} fifo_t;

#define APU_CHANNEL_STRUCT(X) \
    X(bool, length_enabled, 1, 0) \
    X(u32, length, 1, 0) \
    X(bool, envelope_dir, 1, 0) \
    X(u32, envelope_time, 1, 0) \
    X(i8, sample, 1, 0) \
    X(u8, volume, 1, 0) \
    X(u32, freq, 1, 0) \
    X(bool, enabled, 1, 0) \
    X(gba_scheduler_ptr_t, lengthExpired, 1, 1) \
    X(gba_scheduler_ptr_t, freqUpdate, 1, 1) \
    X(gba_scheduler_ptr_t, envUpdate, 1, 1)

DECLARE_SERIALIZABLE_STRUCT(apu_channel, APU_CHANNEL_STRUCT);

typedef struct waveform_t {
    u8 selected;
    u8 idx;
} waveform_t;

#define APU_STRUCT(X) \
    X(u16, SOUNDBIAS, 1, 0) \
    X(u16, SOUNDCNT_L, 1, 0) \
    X(u16, SOUNDCNT_H, 1, 0) \
    X(u8,  SOUNDCNT_X, 1, 0) \
    X(u16, SOUND1CNT_L, 1, 0) \
    X(u16, SOUND1CNT_H, 1, 0) \
    X(u16, SOUND1CNT_X, 1, 0) \
    X(u16, SOUND2CNT_L, 1, 0) \
    X(u16, SOUND2CNT_H, 1, 0) \
    X(u16, SOUND3CNT_L, 1, 0) \
    X(u16, SOUND3CNT_H, 1, 0) \
    X(u16, SOUND3CNT_X, 1, 0) \
    X(u16, SOUND4CNT_L, 1, 0) \
    X(u16, SOUND4CNT_H, 1, 0) \
    X(apu_channel_t, sound_channels, 4, 1, 1) \
    X(waveform_t, waveforms, 2, 1, 0) \
    X(u16, lfsr, 1, 0) \
    X(u8, wave_ram, WAVE_RAM_SIZE, 1, 0) \
    X(u8, wave_idx, 1, 0) \
    X(u8, sound_channels_volume, 1, 0) \
    X(u8, sound_channels_amplifier_right, 1, 0) \
    X(u8, sound_channels_amplifier_left, 1, 0) \
    X(bool, sound_channels_enabled_right, 4, 1, 0) \
    X(bool, sound_channels_enabled_left, 4, 1, 0) \
    X(gba_scheduler_ptr_t, updateSweep, 1, 1) \
    X(fifo_t, fifo, 2, 1, 0) \
    X(u8, timer_idx, 2, 1, 0) \
    X(u8, dma_sound_sample, 2, 1, 0) \
    X(bool, dma_sound_enabled_left, 2, 1, 0) \
    X(bool, dma_sound_enabled_right, 2, 1, 0) \
    X(bool, dma_sound_volume, 2, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(apu, APU_STRUCT)

typedef struct gba_t gba_t;

void gba_push_into_fifo(fifo_t* apu, u8 byte);
void gba_apu_check_timer(gba_t* gba, u8 tmr_idx);

void gba_update_SOUND12CNT_duty(gba_t* gba, u16 reg, int idx);
void gba_update_SOUND12CNT_freq(gba_t* gba, u16 l_reg, u16* h_reg, int idx);

void gba_update_SOUND3CNT_L(gba_t* gba);
void gba_update_SOUND3CNT_H(gba_t* gba);
void gba_update_SOUND3CNT_X(gba_t* gba);

void gba_update_SOUND4CNT_L(gba_t* gba);
void gba_update_SOUND4CNT_H(gba_t* gba);

u8 gba_read_wave_ram(gba_t* gba, u8 addr);
void gba_write_wave_ram(gba_t* gba, u8 addr, u8 byte);

void gba_update_SOUNDCNT_L(apu_t* apu);
void gba_update_SOUNDCNT_H(apu_t* apu);
u8 gba_get_SOUNDCNT_X(apu_t* apu);

void gba_event_push_sample(gba_t* gba, u32 dummy);
void gba_mix_dma_sound(apu_t* apu, sample_t* sample);
void gba_mix_dac_sound(apu_t* apu, sample_t* sample);

void gba_event_length_expired(gba_t* gba, u32 idx);
void gba_event_update_envelope(gba_t* gba, u32 idx);
void gba_event_update_lfsr(gba_t* gba, u32 dummy);
void gba_event_update_tone(gba_t* gba, u32 idx);
void gba_event_update_wave(gba_t* gba, u32 dummy);
void gba_event_update_sweep(gba_t* gba, u32 dummy);

void gba_turn_off_dac(gba_t* gba, u32 idx);

#endif