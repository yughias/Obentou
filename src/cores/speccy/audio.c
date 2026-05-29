#include "cores/speccy/speccy.h"
#include <SDL_MAINLOOP.h>

#include <math.h>

SDL_AudioSpec audioSpec;
SDL_AudioDeviceID audioDev;

uint16_t buffer[AUDIO_BUFFER_SIZE];
size_t bufIdx = 0;

size_t audio_request_rate;
size_t audio_counter;

size_t queued_samples;
size_t queued_check_counter;
size_t queued_check_rate; 

ay_t ay;

float dac_voltage[16] = {
    0.0,
    0.00999465934234,
    0.0144502937362,
    0.0210574502174,
    0.0307011520562,
    0.0455481803616,
    0.0644998855573,
    0.107362478065,
    0.126588845655,
    0.20498970016,
    0.292210269322,
    0.372838941024,
    0.492530708782,
    0.635324635691,
    0.805584802014,
    1.0
};

uint16_t getSample();
uint16_t getFreqCountAy(int);
uint16_t getNoiseCountAy();
uint32_t getEnvCountAy();
void envelopeVolumeAy();

void initAudio(){
    SDL_memset(&audioSpec, 0, sizeof(audioSpec));
    audioSpec.freq = AUDIO_FREQUENCY;
    // audioSpec.format = AUDIO_S16;
    // audioSpec.channels = 1;
    // audioSpec.samples = AUDIO_SAMPLES;
    // audioSpec.callback = NULL;
    // TODO
    // audioDev = SDL_OpenAudioDevice(NULL, 0, &audioSpec, &audioSpec, 0);

    audio_request_rate = CLOCK_PER_FRAME * REFRESH_RATE / AUDIO_FREQUENCY;
    audio_counter = 0;

    queued_check_rate = audio_request_rate * AUDIO_SAMPLES;
    queued_check_counter = 0;

    resetAy();

    // SDL_PauseAudioDevice(audioDev, 0);
}

void sendAudioToDevice(){
    // if(!audio_counter && bufIdx < AUDIO_BUFFER_SIZE){
    //     buffer[bufIdx++] = getSample();
    //     audio_counter = audio_request_rate;
    // } 

    // if(bufIdx >= AUDIO_SAMPLES && queued_samples < AUDIO_BUFFER_SIZE){
    //     SDL_QueueAudio(audioDev, &buffer, bufIdx*2);
    //     bufIdx = 0;
    // }

    // if(!queued_check_counter){
    //     queued_samples = SDL_GetQueuedAudioSize(audioDev) / 2;
    //     queued_check_counter = queued_check_rate;
    // }

    // if(audio_counter)
    //     audio_counter--;

    // if(queued_check_counter)
    //     queued_check_counter--;
}

void freeAudio(){
    // SDL_CloseAudioDevice(audioDev);
}

void emulateAy(){
    ay.halfClock ^= 1;
    if(ay.halfClock)
        return;

    // 3 channels tone emulation
    for(int i = 0; i < 3; i++){
        if(!(AY_REG[AY_AMP_A + i] & 0x10))
            ay.volume[i] = AY_REG[AY_AMP_A + i] & 0x0F;

        if(AY_REG[AY_AMP_A + i] & 0x1F){
            if(ay.pulse_counter[i] >= getFreqCountAy(i)){
                ay.pulse[i] ^= 1;
                ay.pulse_counter[i] = 0;
            }

            ay.pulse_counter[i]++;
        }
    }

    if(AY_REG[AY_NOISE_PERIOD]){
        if(ay.noise_counter >= getNoiseCountAy()){
            ay.lfsr = (((ay.lfsr & 1) ^ ((ay.lfsr >> 3) & 1)) << 16) | (ay.lfsr >> 1);
            ay.noise_counter = 0;
        }
        ay.noise_counter++;
    }
    
    if(ay.env_counter >= getEnvCountAy()){
        envelopeVolumeAy();
        for(int i = 0; i < 3; i++)
            if(AY_REG[AY_AMP_A + i] & 0x10)
                ay.volume[i] = ay.env_step;
        ay.env_counter = 0;
    }
    ay.env_counter++;
}

uint16_t getFreqCountAy(int i){
    uint16_t counter = ((AY_REG[i*2 + 1] & 0x0F) << 8) | AY_REG[i*2];
    if(!counter)
        counter = 1;
    counter <<= 3;
    return counter;    
}

uint32_t getEnvCountAy(){
    uint32_t counter = ((AY_REG[AY_ENV_COARSE]) << 8) | AY_REG[AY_ENV_FINE];
    if(!counter)
        counter = 1;
    counter <<= 4;
    return counter;
}

uint16_t getNoiseCountAy(){
    uint16_t counter = (AY_REG[AY_NOISE_PERIOD] & 0b11111);
    if(!counter)
        counter = 1;
    counter <<= 4;
    return counter;
}

void envelopeVolumeAy(){
    uint8_t env_reg = AY_REG[AY_ENV_SHAPE];
    bool hold = env_reg & 0b1;
    bool alternate = env_reg & 0b10;
    // TODO
    //bool attack = env_reg & 0b100;
    bool cont = env_reg & 0b1000;

    if(
        (ay.env == DESC_ENV && ay.env_step == 0x00) ||
        (ay.env == ASC_ENV  && ay.env_step == 0x0F)
    ) {
        if(!cont){
            ay.env_step = 0;
            ay.env = NO_ENV;
            return;
        }

        if(alternate){
            switch(ay.env){
                case ASC_ENV:
                ay.env = DESC_ENV;
                break;

                case DESC_ENV:
                ay.env = ASC_ENV;
                break;

                default:
                break;
            }
        } else {
            switch(ay.env){
                case ASC_ENV:
                ay.env_step = 0x00;
                break;

                case DESC_ENV:
                ay.env_step = 0x0F;
                break;

                default:
                break;
            }
        }

        if(hold){
            switch(ay.env){
                case ASC_ENV:
                ay.env_step = 0x0F;
                break;

                case DESC_ENV:
                ay.env_step = 0x00;
                break;

                default:
                break;
            }
            ay.env = NO_ENV;
        }
    } else {
        if(ay.env == ASC_ENV && ay.env_step != 0x0F)
            ay.env_step += 1;

        if(ay.env == DESC_ENV && ay.env_step != 0x00)
            ay.env_step -= 1;
    }
}

uint16_t getSample(){
    uint16_t sample = 0; 
    const uint16_t volume_multiplier = SDL_MAX_SINT16 / 4;
    for(int i = 0; i < 3; i++){
        bool tone_enabled = AY_REG[AY_MIXER] & (1 << i);
        bool noise_enabled = AY_REG[AY_MIXER] & (1 << (i+3));
        bool pulse = ay.pulse[i];
        bool noise = ay.lfsr & 1;
        bool bool_sample = (pulse || tone_enabled) && (noise || noise_enabled);
        int int_sample = bool_sample * ay.volume[i];
        float voltage = dac_voltage[int_sample];
        sample += voltage * volume_multiplier;
    }

    // add buzzer
    sample += (ULA & 0b10000) ? volume_multiplier : 0;
    // sample += audioSpec.silence;
    return sample;
}

void resetAy(){
    memset(AY_REG, 0, sizeof(AY_REG));
    memset(&ay, 0, sizeof(ay));
    ay.lfsr = 1;
}

void updateEnvelopeAy(){
    if(AY_REG[AY_ENV_SHAPE] & 0b100){
        ay.env = ASC_ENV;
        ay.env_step = 0;
    } else {
        ay.env = DESC_ENV;
        ay.env_step = 0x0F;
    }
}