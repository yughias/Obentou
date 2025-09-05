#ifndef __APU_H__
#define __APU_H__

#include "types.h"

#include "SDL2/SDL.h"

#define AUDIO_FREQUENCY   44100
#define AUDIO_SAMPLES     4096
#define AUDIO_BUFFER_SIZE  (AUDIO_SAMPLES*2)
#define PUSH_RATE_RELOAD   ((float)APU_FREQUENCY/AUDIO_FREQUENCY)

#define APU_FREQUENCY 4194304

#define NR52_ADDR 0xFF26
#define NR51_ADDR 0xFF25
#define NR50_ADDR 0xFF24
#define NR10_ADDR 0xFF10
#define NR11_ADDR 0xFF11
#define NR12_ADDR 0xFF12
#define NR13_ADDR 0xFF13
#define NR14_ADDR 0xFF14
#define NR21_ADDR 0xFF16
#define NR22_ADDR 0xFF17
#define NR23_ADDR 0xFF18
#define NR24_ADDR 0xFF19
#define NR30_ADDR 0xFF1A
#define NR31_ADDR 0xFF1B
#define NR32_ADDR 0xFF1C
#define NR33_ADDR 0xFF1D
#define NR34_ADDR 0xFF1E
#define NR41_ADDR 0xFF20
#define NR42_ADDR 0xFF21
#define NR43_ADDR 0xFF22
#define NR44_ADDR 0xFF23

#define WAVE_RAM_START_ADDR 0xFF30
#define WAVE_RAM_SIZE 0x10

typedef struct apu_t {
    u8 NR52_REG;
    u8 NR51_REG;
    u8 NR50_REG;
    u8 NR10_REG;
    u8 NR11_REG;
    u8 NR12_REG;
    u8 NR13_REG;
    u8 NR14_REG;
    u8 NR21_REG;
    u8 NR22_REG;
    u8 NR23_REG;
    u8 NR24_REG;
    u8 NR30_REG;
    u8 NR31_REG;
    u8 NR32_REG;
    u8 NR33_REG;
    u8 NR34_REG;
    u8 NR41_REG;
    u8 NR42_REG;
    u8 NR43_REG;
    u8 NR44_REG;

    u8 WAVE_RAM[WAVE_RAM_SIZE];

    size_t pulse_wave_counter;
    size_t length_timer_counter;

    // channel 1: pulse with sweep pace
    bool ch1_on;
    u16 ch1_period_val;
    u16 ch1_length_timer;
    u8 ch1_volume;
    bool ch1_envelope_dir;
    u8 ch1_envelope_pace;
    u16 ch1_sweep_shadow;
    size_t ch1_envelope_counter;
    size_t ch1_waveduty_idx;
    size_t ch1_wavelevel_idx;
    size_t ch1_sweep_counter;
    u8 ch1_sweep_pace;
    bool ch1_sweep_dir;
    u8 ch1_sweep_slope;
    bool ch1_wave_level;
    int8_t ch1_sample;

    // channel 2: pulse
    bool ch2_on;
    u16 ch2_period_val;
    u16 ch2_length_timer;
    u8 ch2_volume;
    bool ch2_envelope_dir;
    u8 ch2_envelope_pace;
    size_t ch2_envelope_counter;
    size_t ch2_waveduty_idx;
    size_t ch2_wavelevel_idx;
    bool ch2_wave_level;
    int8_t ch2_sample;

    // channel 3: wave sound
    bool ch3_on;
    u16 ch3_period_val;
    u16 ch3_length_timer;
    int8_t ch3_sample;
    size_t ch3_wave_idx;

    // channel 4: noise
    bool ch4_on;
    u16 ch4_lfsr;
    size_t ch4_shift_counter;
    u16 ch4_length_timer;
    u8 ch4_volume;
    bool ch4_envelope_dir;
    u8 ch4_envelope_pace;
    size_t ch4_envelope_counter;
    int8_t ch4_sample;

    SDL_AudioDeviceID audioDev;
    int16_t buffer[AUDIO_BUFFER_SIZE];
    size_t bufIdx;
    float push_rate_counter;
    float push_rate_reload;
} apu_t;

void gb_initAudio(apu_t*);
void gb_freeAudio(apu_t*);
void gb_convertAudio(apu_t*);
void gb_emulateApu(apu_t*);

u8 gb_getNR52(apu_t* apu);
u8 gb_getNR10(apu_t* apu);
u8 gb_getNR11(apu_t* apu);
u8 gb_getNR14(apu_t* apu);
u8 gb_getNR21(apu_t* apu);
u8 gb_getNR24(apu_t* apu);
u8 gb_getNR30(apu_t* apu);
u8 gb_getNR32(apu_t* apu);
u8 gb_getNR34(apu_t* apu);
u8 gb_getNR44(apu_t* apu);

void gb_checkDAC1(apu_t* apu);
void gb_checkDAC2(apu_t* apu);
void gb_checkDAC3(apu_t* apu);
void gb_checkDAC4(apu_t* apu);

void gb_setChannel1Timer(apu_t* apu);
void gb_setChannel2Timer(apu_t* apu);
void gb_setChannel3Timer(apu_t* apu);
void gb_setChannel4Timer(apu_t* apu);

void gb_triggerChannel1(apu_t* apu);
void gb_triggerChannel2(apu_t* apu);
void gb_triggerChannel3(apu_t* apu);
void gb_triggerChannel4(apu_t* apu);

u8 gb_getWaveRamAddress(apu_t* apu, u8);
void gb_turnOffAudioChannels(apu_t* apu);

#endif