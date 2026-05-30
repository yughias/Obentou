#include "cores/speccy/speccy.h"

#include "utils/sound.h"

#include <SDL_MAINLOOP.h>

#include <math.h>

static float dac_voltage[16] = {
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

static uint16_t ay_get_freq_count(const ay_t*, int);
static uint16_t ay_get_noise_count(const ay_t*);
static uint32_t ay_get_env_count(const ay_t*);
static void ay_env_volume(ay_t*);

static void get_sample(void* ctx, void* sample_void){
    speccy_t* speccy = (speccy_t*)ctx;
    ay_t* ay = &speccy->ay;
    u16* sample = (u16*)sample_void;
    *sample = 0; 
    const uint16_t volume_multiplier = SDL_MAX_SINT16 / 4;
    for(int i = 0; i < 3; i++){
        bool tone_enabled = ay->reg[AY_MIXER] & (1 << i);
        bool noise_enabled = ay->reg[AY_MIXER] & (1 << (i+3));
        bool pulse = ay->pulse[i];
        bool noise = ay->lfsr & 1;
        bool bool_sample = (pulse || tone_enabled) && (noise || noise_enabled);
        int int_sample = bool_sample * ay->volume[i];
        float voltage = dac_voltage[int_sample];
        *sample += sound_set_channel_sample(voltage * volume_multiplier, i+1);
    }

    // add buzzer
    bool buzzer = (speccy->ula & 0b10000); 
    *sample += sound_set_channel_sample(buzzer, 0) ? volume_multiplier : 0;
    // sample += audioSpec.silence;
}


void speccy_send_audio(void* ctx){
    u16 sample;
    sound_push_sample(1, sizeof(sample), ctx, &sample, get_sample);
}

void speccy_ay_step(ay_t* ay){
    ay->halfClock ^= 1;
    if(ay->halfClock)
        return;

    // 3 channels tone emulation
    for(int i = 0; i < 3; i++){
        if(!(ay->reg[AY_AMP_A + i] & 0x10))
            ay->volume[i] = ay->reg[AY_AMP_A + i] & 0x0F;

        if(ay->reg[AY_AMP_A + i] & 0x1F){
            if(ay->pulse_counter[i] >= ay_get_freq_count(ay, i)){
                ay->pulse[i] ^= 1;
                ay->pulse_counter[i] = 0;
            }

            ay->pulse_counter[i]++;
        }
    }

    if(ay->reg[AY_NOISE_PERIOD]){
        if(ay->noise_counter >= ay_get_noise_count(ay)){
            ay->lfsr = (((ay->lfsr & 1) ^ ((ay->lfsr >> 3) & 1)) << 16) | (ay->lfsr >> 1);
            ay->noise_counter = 0;
        }
        ay->noise_counter++;
    }
    
    if(ay->env_counter >= ay_get_env_count(ay)){
        ay_env_volume(ay);
        for(int i = 0; i < 3; i++)
            if(ay->reg[AY_AMP_A + i] & 0x10)
                ay->volume[i] = ay->env_step;
        ay->env_counter = 0;
    }
    ay->env_counter++;
}

static uint16_t ay_get_freq_count(const ay_t* ay, int i){
    uint16_t counter = ((ay->reg[i*2 + 1] & 0x0F) << 8) | ay->reg[i*2];
    if(!counter)
        counter = 1;
    counter <<= 3;
    return counter;    
}

static uint32_t ay_get_env_count(const ay_t* ay){
    uint32_t counter = ((ay->reg[AY_ENV_COARSE]) << 8) | ay->reg[AY_ENV_FINE];
    if(!counter)
        counter = 1;
    counter <<= 4;
    return counter;
}

static uint16_t ay_get_noise_count(const ay_t* ay){
    uint16_t counter = (ay->reg[AY_NOISE_PERIOD] & 0b11111);
    if(!counter)
        counter = 1;
    counter <<= 4;
    return counter;
}

static void ay_env_volume(ay_t* ay){
    uint8_t env_reg = ay->reg[AY_ENV_SHAPE];
    bool hold = env_reg & 0b1;
    bool alternate = env_reg & 0b10;
    bool cont = env_reg & 0b1000;

    if(
        (ay->env == DESC_ENV && ay->env_step == 0x00) ||
        (ay->env == ASC_ENV  && ay->env_step == 0x0F)
    ) {
        if(!cont){
            ay->env_step = 0;
            ay->env = NO_ENV;
            return;
        }

        if(alternate){
            switch(ay->env){
                case ASC_ENV:
                ay->env = DESC_ENV;
                break;

                case DESC_ENV:
                ay->env = ASC_ENV;
                break;

                default:
                break;
            }
        } else {
            switch(ay->env){
                case ASC_ENV:
                ay->env_step = 0x00;
                break;

                case DESC_ENV:
                ay->env_step = 0x0F;
                break;

                default:
                break;
            }
        }

        if(hold){
            switch(ay->env){
                case ASC_ENV:
                ay->env_step = 0x0F;
                break;

                case DESC_ENV:
                ay->env_step = 0x00;
                break;

                default:
                break;
            }
            ay->env = NO_ENV;
        }
    } else {
        if(ay->env == ASC_ENV && ay->env_step != 0x0F)
            ay->env_step += 1;

        if(ay->env == DESC_ENV && ay->env_step != 0x00)
            ay->env_step -= 1;
    }
}

void speccy_ay_reset(ay_t* ay){
    memset(ay->reg, 0, sizeof(ay->reg));
    memset(ay, 0, sizeof(ay_t));
    ay->lfsr = 1;
}

void speccy_ay_update_envelope(ay_t* ay){
    if(ay->reg[AY_ENV_SHAPE] & 0b100){
        ay->env = ASC_ENV;
        ay->env_step = 0;
    } else {
        ay->env = DESC_ENV;
        ay->env_step = 0x0F;
    }
}