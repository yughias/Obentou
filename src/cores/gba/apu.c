#include "cores/gba/apu.h"
#include "cores/gba/gba.h"

#include "utils/sound.h"

#include <stdio.h>

#define IS_SOUND_DMA(x) (((x >> 28) & 0b11) == 0b11)
#define LENGTH_UNIT (1 << 16)
#define NOISE_FREQ_UNIT (1 << 5)
#define ENV_UNIT (1 << 18)
#define TONE_FREQ_UNIT (1 << 4)
#define WAVE_FREQ_UNIT (1 << 3)
#define SWEEP_UNIT 131072
#define CALCULATE_LENGTH_TIME(reg) (64 - (reg)) * LENGTH_UNIT
#define CALCULATE_SWEEP_TIME(reg) ((reg >> 4) & 0b111) * SWEEP_UNIT
#define IS_PLAYBACK(reg) ((reg >> 11) & 0x1F)
#define HOST_GAIN 32

static const i8 waveforms[4][8] = {
    { -1, -1, -1, -1, -1, -1, -1, +1 },
    { +1, -1, -1, -1, -1, -1, -1, +1 },
    { +1, -1, -1, -1, -1, +1, +1, +1 },
    { -1, +1, +1, +1, +1, +1, +1, -1 }
};

static void fifo_reset(fifo_t* fifo);
static u8 fifo_pop(fifo_t* fifo);
static void fifo_refill(gba_t* gba, u32 fifo_addr);

static void event_envelope_start(gba_t* gba, apu_channel_t* ch, int idx){
    if(ch->envelope_time){
        ch->envUpdate = gba_scheduler_create_add_event_1_args(&gba->scheduler_head, gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE, gba_event_update_envelope, idx, ch->envelope_time);
    } else {
        ch->envUpdate = NULL;
    }
}

static void channel_trigger(gba_t* gba, apu_channel_t* ch, u8 idx, gba_event_t lengthFunc, gba_event_t envFunc, gba_event_t freqFunc){
    gba_scheduler_t** scheduler = &gba->scheduler_head;

    gba_scheduler_remove_if_present(scheduler, &ch->lengthExpired);
    gba_scheduler_remove_if_present(scheduler, &ch->envUpdate);
    gba_scheduler_remove_if_present(scheduler, &ch->freqUpdate);

    ch->freqUpdate = gba_scheduler_create_add_event_1_args(scheduler, gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE, freqFunc, idx, ch->freq);

    if(ch->length_enabled){
        ch->lengthExpired = gba_scheduler_create_add_event_1_args(scheduler, gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE, lengthFunc, idx, ch->length);
    } else {
        ch->lengthExpired = NULL;
    }

    event_envelope_start(gba, ch, idx);
}

void gba_update_SOUND12CNT_duty(gba_t* gba, u16 reg, int idx){
    apu_channel_t* ch = &gba->apu.sound_channels[idx];
    ch->length = CALCULATE_LENGTH_TIME(reg & 0x3F);

    u16 playback_reg = idx ? gba->apu.SOUND2CNT_L : gba->apu.SOUND1CNT_H;
    bool playback = IS_PLAYBACK(playback_reg); 

    if(!playback)
        gba_turn_off_dac(gba, idx);
}

void gba_update_SOUND12CNT_freq(gba_t* gba, u16 l_reg, u16* h_reg, int idx){
    apu_t* apu = &gba->apu;
    apu_channel_t* ch = &apu->sound_channels[idx];
    u16 playback_reg = idx ? gba->apu.SOUND2CNT_L : gba->apu.SOUND1CNT_H;
    bool playback = IS_PLAYBACK(playback_reg); 
    ch->freq = (2048 - ((*h_reg) & 0x7FF)) * TONE_FREQ_UNIT;
    ch->length_enabled = (*h_reg) & (1 << 14);
    
    if((*h_reg) & (1 << 15) && playback){
        waveform_t* waveform = &gba->apu.waveforms[idx];
        ch->enabled = true;
        waveform->selected = (l_reg >> 6) & 0x3;
        ch->envelope_time = ((l_reg >> 8) & 0b111) * ENV_UNIT;
        ch->envelope_dir = l_reg & (1 << 11);
        ch->volume = (l_reg >> 12) & 0xF;
        channel_trigger(gba, ch, idx, gba_event_length_expired, gba_event_update_envelope, gba_event_update_tone);
    
        if(!idx){
            u32 sweep_time = CALCULATE_SWEEP_TIME((*h_reg));
            gba_scheduler_remove_if_present(&gba->scheduler_head, &apu->updateSweep);

            if(sweep_time){
                apu->updateSweep = gba_scheduler_create_add_event_0_args(&gba->scheduler_head, gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE, gba_event_update_sweep, sweep_time);
            }
        }

        (*h_reg) &= ~(1 << 15);
    }
}

void gba_update_SOUND3CNT_L(gba_t* gba){
    bool playback = gba->apu.SOUND3CNT_L & (1 << 7);

    if(!playback)
        gba_turn_off_dac(gba, 2);
}

void gba_update_SOUND3CNT_H(gba_t* gba){
    u16 reg = gba->apu.SOUND3CNT_H;
    apu_channel_t* ch = &gba->apu.sound_channels[2];
    ch->length = (256 - (reg & 0xFF)) * LENGTH_UNIT;
}

void gba_update_SOUND3CNT_X(gba_t* gba){
    apu_t* apu = &gba->apu;
    apu_channel_t* ch = &apu->sound_channels[2];
    bool playback = apu->SOUND3CNT_L & (1 << 7);
    ch->freq = (2048 - (apu->SOUND3CNT_X & 0x7FF)) * WAVE_FREQ_UNIT;

    if(apu->SOUND3CNT_X & (1 << 15) && playback){
        ch->enabled = true;
        ch->length_enabled = apu->SOUND3CNT_X & (1 << 14);
        ch->envelope_time = 0;
        apu->wave_idx = 0;
        ch->sample = true;
        channel_trigger(gba, ch, 2, gba_event_length_expired, NULL, gba_event_update_wave);

        apu->SOUND3CNT_X &= ~(1 << 15);
    }
}

void gba_update_SOUND4CNT_L(gba_t* gba){
    u16 reg = gba->apu.SOUND4CNT_L;
    apu_channel_t* noise_channel = &gba->apu.sound_channels[3];
    noise_channel->length = CALCULATE_LENGTH_TIME(reg & 0x3F);
    bool playback = IS_PLAYBACK(gba->apu.SOUND4CNT_L); 

    if(!playback)
        gba_turn_off_dac(gba, 3);
}

void gba_update_SOUND4CNT_H(gba_t* gba){
    apu_channel_t* noise_channel = &gba->apu.sound_channels[3];
    u16 l_reg = gba->apu.SOUND4CNT_L;
    u16 h_reg = gba->apu.SOUND4CNT_H;
    u8 r = h_reg & 0x7;
    u8 s = (h_reg >> 4) & 0xF;
    bool playback = IS_PLAYBACK(gba->apu.SOUND4CNT_L); 
    noise_channel->freq = NOISE_FREQ_UNIT << (s+1);
    
    if(!r)
        noise_channel->freq >>= 1;
    else
        noise_channel->freq *= r; 

    noise_channel->length_enabled = h_reg & (1 << 14);

    if(h_reg & (1 << 15) && playback){
        noise_channel->enabled = true;
        noise_channel->envelope_time = ((l_reg >> 8) & 0b111) * ENV_UNIT;
        noise_channel->envelope_dir = l_reg & (1 << 11);
        noise_channel->volume = (l_reg >> 12) & 0xF;

        if(h_reg & (1 << 3))
            gba->apu.lfsr = 0x40;
        else
            gba->apu.lfsr = 0x4000;

        channel_trigger(gba, noise_channel, 3, gba_event_length_expired, gba_event_update_envelope, gba_event_update_lfsr);

        gba->apu.SOUND4CNT_H &= ~(1 << 15);
    }
}

void gba_update_SOUNDCNT_L(apu_t* apu){
    u16 reg = apu->SOUNDCNT_L;
    apu->sound_channels_amplifier_right = (reg & 0b111) + 1;
    apu->sound_channels_amplifier_left = ((reg >> 4) & 0b111) + 1;

    for(int i = 0; i < 4; i++){
        apu->sound_channels_enabled_right[i] = reg & (1 << (8+i));
        apu->sound_channels_enabled_left[i] = reg & (1 << (12+i));
    }
}

void gba_update_SOUNDCNT_H(apu_t* apu){
    for(int i = 0; i < 2; i++){
        apu->timer_idx[i] = (bool)(apu->SOUNDCNT_H & (1 << (10 + i*4)));
        if(apu->SOUNDCNT_H & (1 << (11 + i*4)))
            fifo_reset(&apu->fifo[i]);
        apu->dma_sound_volume[i] = 1 - (apu->SOUNDCNT_H & (1 << (2 + i)));
        apu->dma_sound_enabled_left[i] = apu->SOUNDCNT_H & (1 << (9 + i*4));
        apu->dma_sound_enabled_right[i] = apu->SOUNDCNT_H & (1 << (8 + i*4));
    }

    apu->sound_channels_volume = 2 - (apu->SOUNDCNT_H & 0b11);

    apu->SOUNDCNT_H &= ~( (1 << 11) | (1 << 15) | (0xF << 4));
}

void gba_apu_check_timer(gba_t* gba, u8 tmr_idx){
    apu_t* apu = &gba->apu;
    for(int i = 0; i < 2; i++){
        if(apu->timer_idx[i] == tmr_idx){
            fifo_t* fifo = &apu->fifo[i];
            if(fifo->size <= APU_FIFO_REQUEST)
                fifo_refill(gba, BASE_FIFO_ADDR + i*4);
            apu->dma_sound_sample[i] = fifo_pop(fifo);
        }
    }
}

void gba_push_into_fifo(fifo_t* fifo, u8 byte){
    if(fifo->size == APU_FIFO_LENGTH)
        return;

    fifo->size += 1;
    fifo->data[fifo->w_idx] = byte;
    fifo->w_idx = (fifo->w_idx + 1) % APU_FIFO_LENGTH;
}

static void fifo_reset(fifo_t* fifo){
    fifo->size = 0;
    fifo->r_idx = 0;
    fifo->w_idx = 0;
}

static u8 fifo_pop(fifo_t* fifo){
    if(!fifo->size)
        return 0;
    u8 out = fifo->data[fifo->r_idx];
    fifo->size -= 1;
    fifo->r_idx = (fifo->r_idx + 1) % APU_FIFO_LENGTH;
    return out;
}

static void fifo_refill(gba_t* gba, u32 fifo_addr){
    arm7tdmi_t* cpu = &gba->cpu;

    for(int i = 1; i <= 2; i++){
        dma_t* dma = &gba->dmas[i];
        bool enabled = dma->DMACNT >> 31;
        if(IS_SOUND_DMA(dma->DMACNT) && dma->DMADAD == fifo_addr && enabled){
            int step = 0;
            switch((dma->DMACNT >> 17) & 0b11){
                case 0b00:
                step = 4;
                break;

                case 0b01:
                step = -4;
                break;
            }

            for(int j = 0; j < 4; j++){
                u32 word = cpu->readWord(cpu, dma->internal_source, true);
                dma->internal_source += step;
                cpu->writeWord(cpu, fifo_addr, word, true);
            }

            if((dma->DMACNT >> 0x1E) & 1){
                gba->IF |= 1 << (0x8 + i);
                gba_check_interrupts(gba);
            }
        }
    }
}

static void apu_mixer(void* ctx, void* s) {
    sample_t* sample = s;
    gba_t* gba = ctx;
    apu_t* apu = &gba->apu;

    sample->left = sample->right = 0;
    gba_mix_dma_sound(apu, sample);
    gba_mix_dac_sound(apu, sample);

    u16 bias = gba->apu.SOUNDBIAS & (0x1FF << 1);
    sample->left += bias;
    sample->right += bias;

    if(sample->left < 0)
        sample->left = 0;
    if(sample->left > 0x3FF)
        sample->left = 0x3FF;
    
    if(sample->right < 0)
        sample->right = 0;
    if(sample->right > 0x3FF)
        sample->right = 0x3FF;

    sample->left -= 0x200;
    sample->right -= 0x200;

    sample->left = sample->left * HOST_GAIN;
    sample->right = sample->right * HOST_GAIN;
}

void gba_event_push_sample(gba_t* gba, u32 dummy){
    sample_t sample;
    sound_push_sample(sound_get_cycles_until_sample(), sizeof(sample), gba, &sample, apu_mixer);

    gba_scheduler_create_add_event_0_args(&gba->scheduler_head, gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE, gba_event_push_sample, sound_get_cycles_until_sample());
}

void gba_mix_dma_sound(apu_t* apu, sample_t* sample){
    for(int i = 0; i < 2; i++){
        int dma_sample = sound_set_channel_sample(4 * (((i16)(i8)apu->dma_sound_sample[i])) >> apu->dma_sound_volume[i], 4 + i);
        if(apu->dma_sound_enabled_right[i])
            sample->right += dma_sample;
        if(apu->dma_sound_enabled_left[i])
            sample->left += dma_sample;
    }
}

void gba_mix_dac_sound(apu_t* apu, sample_t* sample){
    sample_t dac_sample = {0, 0};
    
    for(int i = 0; i < 4; i++){
        u16 sample_value = sound_set_channel_sample(apu->sound_channels[i].sample * apu->sound_channels[i].volume, i);
        dac_sample.left += sample_value * apu->sound_channels_enabled_left[i];
        dac_sample.right += sample_value * apu->sound_channels_enabled_right[i];    
    }

    dac_sample.left *= apu->sound_channels_amplifier_left;
    dac_sample.right *= apu->sound_channels_amplifier_right;

    dac_sample.left >>= apu->sound_channels_volume;
    dac_sample.right >>= apu->sound_channels_volume;

    sample->left += dac_sample.left;
    sample->right += dac_sample.right;
}

void gba_event_length_expired(gba_t* gba, u32 idx){
    apu_channel_t* ch = &gba->apu.sound_channels[idx];

    ch->enabled = false;
    gba_scheduler_remove_if_present(&gba->scheduler_head, &ch->freqUpdate);
    gba_scheduler_remove_if_present(&gba->scheduler_head, &ch->envUpdate);
    ch->lengthExpired = NULL;

    if(!idx)
        gba_scheduler_remove_if_present(&gba->scheduler_head, &gba->apu.updateSweep);
}

void gba_event_update_envelope(gba_t* gba, u32 idx){
    apu_channel_t* ch = &gba->apu.sound_channels[idx];
    if(ch->envelope_dir){
        if(ch->volume != 15)
            ch->volume += 1;
    } else {
        if(ch->volume != 0)
            ch->volume -= 1;
    }
    event_envelope_start(gba, ch, idx);
}

void gba_event_update_lfsr(gba_t* gba, u32 dummy){
    apu_channel_t* noise_channel = &gba->apu.sound_channels[3];

    u16* lfsr = &gba->apu.lfsr;
    bool carry = *lfsr & 1;
    noise_channel->sample = carry;
    *lfsr >>= 1;

    if(carry){
        if(gba->apu.SOUND4CNT_H & (1 << 3))
            *lfsr ^= 0x60;
        else
            *lfsr ^= 0x6000;
    }

    noise_channel->freqUpdate = gba_scheduler_create_add_event_0_args(&gba->scheduler_head, gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE, gba_event_update_lfsr, noise_channel->freq);
}

void gba_event_update_tone(gba_t* gba, u32 idx){
    apu_channel_t* ch = &gba->apu.sound_channels[idx];
    waveform_t* waveform = &gba->apu.waveforms[idx];   

    ch->sample = waveforms[waveform->selected][waveform->idx];
    waveform->idx = (waveform->idx + 1) % 8; 

    ch->freqUpdate = gba_scheduler_create_add_event_1_args(&gba->scheduler_head, gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE, gba_event_update_tone, idx, ch->freq);
}

void gba_event_update_wave(gba_t* gba, u32 dummy){
    apu_t* apu = &gba->apu;
    apu_channel_t* ch = &apu->sound_channels[2];
    u16 l_reg = apu->SOUND3CNT_L;

    u8 base = (bool)(l_reg & (1 << 6)) * 0x10;
    u8 buffer_size = (bool)(l_reg & (1 << 5)) * 0x20 + 0x20;
    u8 sample = apu->wave_ram[(base + (apu->wave_idx >> 1)) % WAVE_RAM_SIZE];
    ch->volume = apu->wave_idx & 1 ? sample & 0x0F : sample >> 4;
    apu->wave_idx = (apu->wave_idx + 1) % buffer_size;

    u16 h_reg = apu->SOUND3CNT_H;
    if(h_reg & (1 << 15)) {
        ch->volume -= ch->volume >> 2;
    } else {
        u8 sound_volume = (h_reg >> 13) & 0b11;
        if(!sound_volume)
            ch->volume = 0;
        else
            ch->volume >>= (sound_volume - 1);
    }

    ch->freqUpdate = gba_scheduler_create_add_event_0_args(&gba->scheduler_head, gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE, gba_event_update_wave, ch->freq);
}

void gba_event_update_sweep(gba_t* gba, u32 dummy){
    apu_t* apu = &gba->apu;
    apu_channel_t* ch = &apu->sound_channels[0];
    u32 sweep_time = CALCULATE_SWEEP_TIME(apu->SOUND1CNT_L);

    if(!sweep_time){
        apu->updateSweep = NULL;
        return;
    }

    u16 freq = apu->SOUND1CNT_X & 0x7FF;
    u8 n = apu->SOUND1CNT_L & 0b111;
    bool sweep_dir = gba->apu.SOUND1CNT_L & (1 << 3);
    freq += sweep_dir ? -(freq >> n) : (freq >> n);
    apu->SOUND1CNT_X &= ~0x7FF;
    apu->SOUND1CNT_X |= freq & 0x7FF;

    if(freq >= 2048 || !freq){
        ch->enabled = false;
        gba_scheduler_remove_if_present(&gba->scheduler_head, &ch->freqUpdate);
        gba_scheduler_remove_if_present(&gba->scheduler_head, &ch->lengthExpired);
        gba_scheduler_remove_if_present(&gba->scheduler_head, &ch->envUpdate);
        apu->updateSweep = NULL;
    } else {
        ch->freq = (2048 - freq) * TONE_FREQ_UNIT;
        apu->updateSweep = gba_scheduler_create_add_event_0_args(&gba->scheduler_head, gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE, gba_event_update_sweep, sweep_time);
    }
}

u8 gba_read_wave_ram(gba_t* gba, u8 addr){
    bool bank = gba->apu.SOUND3CNT_L & (1 << 6);
    return gba->apu.wave_ram[addr + (!bank) * 0x10];
}

void gba_write_wave_ram(gba_t* gba, u8 addr, u8 byte){
    bool bank = gba->apu.SOUND3CNT_L & (1 << 6);
    gba->apu.wave_ram[addr + (!bank) * 0x10] = byte;
}

void gba_turn_off_dac(gba_t* gba, u32 idx){
    apu_channel_t* ch = &gba->apu.sound_channels[idx];
    ch->sample = 0;
    if(!ch->enabled)
        return;
    gba_scheduler_remove_if_present(&gba->scheduler_head, &ch->lengthExpired);
    gba_event_length_expired(gba, idx);
}

u8 gba_get_SOUNDCNT_X(apu_t* apu){
    apu->SOUNDCNT_X &= (1 << 7);
    for(int i = 0; i < 4; i++)
        apu->SOUNDCNT_X |= apu->sound_channels[i].enabled << i;
    return apu->SOUNDCNT_X;
}