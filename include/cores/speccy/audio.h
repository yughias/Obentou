#ifndef __AUDIO_H__
#define __AUDIO_H__

#define AUDIO_FREQUENCY   44100
#define AUDIO_SAMPLES      1024
#define AUDIO_BUFFER_SIZE  2048

#define AY_TONE_FINE_A     0x00
#define AY_TONE_COARSE_A   0x01
#define AY_TONE_FINE_B     0x02
#define AY_TONE_COARSE_B   0x03
#define AY_TONE_FINE_C     0x04
#define AY_TONE_COARSE_C   0x05
#define AY_NOISE_PERIOD    0x06
#define AY_MIXER           0x07
#define AY_AMP_A           0x08
#define AY_AMP_B           0x09
#define AY_AMP_C           0x0A
#define AY_ENV_FINE        0x0B
#define AY_ENV_COARSE      0x0C
#define AY_ENV_SHAPE       0x0D

void initAudio();
void freeAudio();
void emulateAy();
void resetAy();
void updateEnvelopeAy();
void sendAudioToDevice();

typedef struct {
    uint16_t pulse_counter[3];
    uint16_t noise_counter;
    uint32_t lfsr;
    uint32_t env_counter;
    uint8_t env_step;
    uint8_t volume[3];
    enum {NO_ENV, ASC_ENV, DESC_ENV} env;
    bool pulse[3];
    bool halfClock;
} ay_t; 

extern ay_t ay;

#endif