#include "cores/watara/apu.h"
#include "SDL_MAINLOOP.h"

#include <stdio.h>

static const bool duty_lut[4][8] = {
    {0, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 1, 1},
};

static const int ch3_divider_lut[4] = {
    256, 512, 1024, 2048
};

static const int noise_divider_lut[16] = {
        8,    16,    32,    64, 
      128,   256,   512,  1024,
     2048,  4096,  8192, 16384,
    32768, 65536, 32768, 65536,
};

static void apu_wave_reload(wave_t* w){
    w->duty_phase = (w->duty_phase + 1) % 8;
    int freq = w->flow | ((w->fhi & 0b111) << 8);
    w->freq_counter += freq << 2;
}

static void apu_ch3_reload(apu_t* apu){
    ch3_t* ch3 = &apu->ch3;
    ch3->freq_counter += ch3_divider_lut[ch3->ctrl & 0b11];
    if(ch3->trigger){   
        if(!ch3->nibble_counter){
            u16 addr = ch3->addr_lo | (ch3->addr_hi << 8);
            ch3->dma_active = true;
            ch3->sample = apu->read(apu->ctx, addr);
            ch3->dma_active = false;
            addr += 1;
            ch3->addr_lo = addr & 0xFF;
            ch3->addr_hi = addr >> 8;
            if(!ch3->byte_counter){
                ch3->length -= 1;
                if(!ch3->length){
                    ch3->trigger = false;
                    ch3->irq = true;
                    return;
                }
                ch3->byte_counter = 16;
            }
            ch3->byte_counter -= 1;
            ch3->nibble_counter = 2;
        }
        ch3->nibble_counter -= 1;
    }
}

static void apu_noise_reload(noise_t* n){
    n->freq_counter += noise_divider_lut[n->freq_vol >> 4];
    bool lfsr_len = n->ctrl & 1;
    bool b0;
    bool b1;
    if(lfsr_len){
       b0 = n->lfsr & (1 << 0xD);
       b1 = n->lfsr & (1 << 0xE);
    } else {
        b0 = n->lfsr & (1 << 0x5);
        b1 = n->lfsr & (1 << 0x6);
    }
    bool xored = b0 ^ b1;
    n->lfsr = (n->lfsr << 1) | (xored);
}

static u8 apu_wave_get_sample(wave_t* w){
    bool mode = w->vol_duty & 0x40;
    if(!mode && !w->length)
        return 0; 
    int freq = w->flow | ((w->fhi & 0b111) << 8);
    u8 duty_idx = (w->vol_duty >> 4) & 0b11;
    bool level = duty_lut[duty_idx][w->duty_phase];
    u8 volume = w->vol_duty & 0xF;
    u8 out = level * volume;
    if(freq <= 2)
        return volume;
    return out;
}

static u8 apu_ch3_get_sample(ch3_t* c){
    if(!c->trigger)
        return 0;
    if(c->nibble_counter == 1)
        return c->sample >> 4;
    return c->sample & 0xF;
}

static u8 apu_noise_get_sample(noise_t* n){
    bool enabled = n->ctrl & 0x10;
    if(!enabled)
        return 0;
    bool mode = n->ctrl & 0b10;
    if(!mode && !n->length)
        return 0;
    bool level = n->lfsr & 1;
    u8 volume = n->freq_vol & 0xF;
    return level * volume;
}

void watara_apu_step(apu_t* apu, int cycles){
    for(int i = 0; i < 2; i++){
        wave_t* w = &apu->waves[i];
        w->freq_counter -= cycles;
        if(w->freq_counter <= 0)
            apu_wave_reload(w);
    }

    ch3_t* ch3 = &apu->ch3;
    ch3->freq_counter -= cycles;
    if(ch3->freq_counter <= 0)
        apu_ch3_reload(apu);

    noise_t* n = &apu->noise;
    n->freq_counter -= cycles;
    if(n->freq_counter <= 0)
        apu_noise_reload(n);

    apu->len_counter -= cycles;
    if(apu->len_counter <= 0){
        apu->len_counter += 65536;
        for(int i = 0; i < 2; i++){
            wave_t* w = &apu->waves[i];
            if(w->length)
                w->length -= 1;
        }
        noise_t* n = &apu->noise;
        if(n->length)
            n->length -= 1;
    }
}

static sample_t apu_get_sample(apu_t* apu){
    sample_t sample;
    u8 waves[2] = { apu_wave_get_sample(&apu->waves[0]), apu_wave_get_sample(&apu->waves[1]) };
    sample.r = waves[0];
    sample.l = waves[1];

    u8 noise_sample = apu_noise_get_sample(&apu->noise);
    bool noise_l = apu->noise.ctrl & 0x08;
    bool noise_r = apu->noise.ctrl & 0x04;
    sample.l += noise_sample * noise_l;
    sample.r += noise_sample * noise_r;

    u8 ch3_sample = apu_ch3_get_sample(&apu->ch3);
    bool ch3_l = apu->ch3.ctrl & 0x08;
    bool ch3_r = apu->ch3.ctrl & 0x04;
    sample.l += ch3_sample * ch3_l;
    sample.r += ch3_sample * ch3_r;

    // clipping
    if(sample.l > 0xF)
        sample.l = 0xF;
    if(sample.r > 0xF)
        sample.r = 0xF;
    sample.l <<= 3;
    sample.r <<= 3;

    if(apu->display_idx != DISPLAY_BUFFER_SIZE){
        apu->display_buffers[0][apu->display_idx] = waves[0];
        apu->display_buffers[1][apu->display_idx] = waves[1];
        apu->display_buffers[2][apu->display_idx] = noise_sample;
        apu->display_buffers[3][apu->display_idx] = ch3_sample;
        apu->display_idx += 1;
    }

    return sample;
}

void watara_apu_push_sample(apu_t* apu, int cycles){
    apu->push_counter -= cycles;
    if(apu->push_counter <= 0){
        apu->push_counter += apu->push_reload;
        sample_t sample = apu_get_sample(apu);

        #ifdef __EMSCRIPTEN__
        if(apu->buffer_size < AUDIO_BUFFER_SIZE)
            apu->buffer[apu->buffer_size++] = sample;
        if(SDL_GetQueuedAudioSize(apu->audio_dev) <= AUDIO_BUFFER_SIZE * sizeof(sample_t)){
            SDL_QueueAudio(apu->audio_dev, apu->buffer, apu->buffer_size * sizeof(sample_t));
            apu->buffer_size = 0;
        }
        #else
        apu->buffer[apu->buffer_size++] = sample;
        if(apu->buffer_size == AUDIO_BUFFER_SIZE){
            SDL_QueueAudio(apu->audio_dev, apu->buffer, apu->buffer_size*sizeof(sample_t));
            apu->buffer_size = 0;
            while(SDL_GetQueuedAudioSize(apu->audio_dev) > AUDIO_BUFFER_SIZE*sizeof(sample_t));
        }
        #endif

    }
}

static void apu_draw_wave(int x0, int y0, u8* buffer, SDL_Surface* s, int scale){
    int* pixels = (int*)s->pixels;
    const int white = color(255, 255, 255);

    float avg = 0;
    for(int i = 0; i < DISPLAY_BUFFER_SIZE; i++)
        avg += buffer[i];
    avg /= DISPLAY_BUFFER_SIZE;

    int idx = 0;
    int start = -1;
    int end = -1;
    int max_val = -(1 << 16);
    int min_val = (1 << 16);
    for(int i = s->w/4; i < DISPLAY_BUFFER_SIZE; i++){
        u8 s0 = buffer[i-1];
        u8 s1 = buffer[i];
        if(s0 < min_val)
            min_val = s0;
        if(s0 > max_val)
            max_val = s1;
        if(s0 < avg && s1 >= avg){
            start = i;
        }
        if(start != -1 && s1 < avg && s0 >= avg){
            end = i;
            break;
        } 
    }

    y0 += (max_val - min_val) * scale / 2;

    if(start == -1)
        start = 0;
    if(end == -1)
        end = start;
    idx = (start + end) / 2;
    idx -= s->w/4;
    if(idx < 0)
        idx = 0;
    
    int prev;
    for(int i = 0; i < s->w/2; i++){
        int sample_idx = idx;
        if(sample_idx >= DISPLAY_BUFFER_SIZE)
            sample_idx = DISPLAY_BUFFER_SIZE ? DISPLAY_BUFFER_SIZE-1 : 0;
        int sample = y0 - buffer[sample_idx] * scale;
        if(!i)
            prev = sample;
        if(sample >= prev){
            for(int j = prev; j <= sample; j++)
                pixels[x0 + i + j * s->w] = white;
        } else {
            for(int j = sample; j <= prev; j++)
                pixels[x0 + i + j * s->w] = white;
        }
        idx += 1;
        prev = sample;
    }
}

void watara_apu_draw_waves(apu_t* apu, SDL_Window** win){
    Uint32 id = SDL_GetWindowID(*win);
    if(!id){
        *win = NULL;
        return;
    }
    if(apu->display_idx != DISPLAY_BUFFER_SIZE)
        return;
    SDL_Surface* s = SDL_GetWindowSurface(*win);
    int* pixels = (int*)s->pixels;
    SDL_FillRect(s, NULL, 0);
    for(int y = 0; y < 2; y++){
        for(int x = 0; x < 2; x++){
            int idx =  x + y*2;
            int x0 = x*s->w/2;
            int y0 = y*s->h/2 + s->h/4;
            u8* buf = apu->display_buffers[idx];
            apu_draw_wave(x0, y0, buf, s, 8);
        }   
    }

    const int grey = color(100, 100, 100);
    for(int i = 0; i < s->w; i++){
        pixels[i + s->h / 2 * s->w] = grey;
    }
    for(int i = 0; i < s->h; i++)
        pixels[s->w / 2 + i * s->w] = grey;
    
    memset(apu->display_buffers, 0, sizeof(apu->display_buffers));
    apu->display_idx = 0;

    SDL_UpdateWindowSurface(*win);
}