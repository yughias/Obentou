#include "chips/ay.h"

#include "utils/sound.h"

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

static u16 ay_get_freq_count(const ay_t*, int);
static u16 ay_get_noise_count(const ay_t*);
static u32 ay_get_env_count(const ay_t*);
static void ay_env_volume(ay_t*);

u16 ay_get_sample(ay_t* ay, int volume_multiplier){
    u16 sample = 0;
    for(int i = 0; i < 3; i++){
        bool tone_enabled = ay->reg[AY_MIXER] & (1 << i);
        bool noise_enabled = ay->reg[AY_MIXER] & (1 << (i+3));
        bool pulse = ay->pulse[i];
        bool noise = ay->lfsr & 1;
        bool bool_sample = (pulse || tone_enabled) && (noise || noise_enabled);
        int int_sample = bool_sample * ay->volume[i];
        float voltage = dac_voltage[int_sample];
        sample += sound_set_channel_sample(voltage * volume_multiplier, i);
    }
    return sample;
}

void ay_step(ay_t* ay){
    // 3 channels tone emulation
    for(int i = 0; i < 3; i++){
        if(!(ay->reg[AY_AMP_A + i] & 0x10))
            ay->volume[i] = ay->reg[AY_AMP_A + i] & 0x0F;

        if(ay->reg[AY_AMP_A + i] & 0x1F){
            u16 freq_count = ay_get_freq_count(ay, i);
            // simulate a low-pass filter 
            if (!freq_count)
                continue;
    
            if(!ay->pulse_counter[i]){
                ay->pulse[i] ^= 1;
                ay->pulse_counter[i] = freq_count;
            }

            ay->pulse_counter[i] -= 1;
        }
    }

    if(ay->reg[AY_NOISE_PERIOD]){
        if(!ay->noise_counter){
            ay->lfsr = (((ay->lfsr & 1) ^ ((ay->lfsr >> 3) & 1)) << 16) | (ay->lfsr >> 1);
            ay->noise_counter = ay_get_noise_count(ay);
        }
        ay->noise_counter -= 1;
    }
    
    if(!ay->env_counter){
        ay_env_volume(ay);
        for(int i = 0; i < 3; i++)
            if(ay->reg[AY_AMP_A + i] & 0x10)
                ay->volume[i] = ay->env_step;
        ay->env_counter = ay_get_env_count(ay);
    }
    ay->env_counter -= 1;
}

static u16 ay_get_freq_count(const ay_t* ay, int i){
    u16 counter = ((ay->reg[i*2 + 1] & 0x0F) << 8) | ay->reg[i*2];
    counter <<= 3;
    return counter;    
}

static u32 ay_get_env_count(const ay_t* ay){
    u32 counter = ((ay->reg[AY_ENV_COARSE]) << 8) | ay->reg[AY_ENV_FINE];
    if(!counter)
        counter = 1;
    counter <<= 4;
    return counter;
}

static u16 ay_get_noise_count(const ay_t* ay){
    u16 counter = (ay->reg[AY_NOISE_PERIOD] & 0b11111);
    if(!counter)
        counter = 1;
    counter <<= 4;
    return counter;
}

static void ay_env_volume(ay_t* ay){
    u8 env_reg = ay->reg[AY_ENV_SHAPE];
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

void ay_reset(ay_t* ay){
    memset(ay->reg, 0, sizeof(ay->reg));
    memset(ay, 0, sizeof(ay_t));
    ay->lfsr = 1;
}

static void ay_update_envelope(ay_t* ay){
    if(ay->reg[AY_ENV_SHAPE] & 0b100){
        ay->env = ASC_ENV;
        ay->env_step = 0;
    } else {
        ay->env = DESC_ENV;
        ay->env_step = 0x0F;
    }
}

void ay_write_selected_port(ay_t* ay, u8 data){
    ay->reg[ay->selected & 0x0F] = data;
    if(ay->selected == AY_ENV_SHAPE)
        ay_update_envelope(ay);
}