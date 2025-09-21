#include "cores/pce/psg.h"
#include "peripherals/sound.h"

#include "SDL_MAINLOOP.h"

#include <stdio.h>

void pce_psg_write(psg_t* psg, u8 offset, u8 value){
    if(offset > 9){
        printf("psg_write: Unhandled write to address 0x%02X\n", offset);
        return;
    }

    switch(offset){
        case PSG_SELECTOR:
        psg->selector = value % PSG_CHANNELS;
        break;

        case PSG_GLOBAL_VOL:
        psg->left_vol = value >> 4;
        psg->right_vol = value & 0x0F;
        break;

        case PSG_FINE_FREQ:
        psg->ch[psg->selector].freq = (psg->ch[psg->selector].freq & 0xFF00) | value;
        break;

        case PSG_COARSE_FREQ:
        psg->ch[psg->selector].freq = (psg->ch[psg->selector].freq & 0x00FF) | ((value & 0x0F) << 8);
        break;

        case PSG_CTRL:
        psg->ch[psg->selector].ctrl = value;
        u8 ch_dda = value >> 6;
        if(ch_dda == 0b01)
            psg->ch[psg->selector].idx = 0;
        break;

        case PSG_BALANCE:
        psg->ch[psg->selector].left_vol = value >> 4;
        psg->ch[psg->selector].right_vol = value & 0x0F;
        break;

        case PSG_SOUND_DATA:
        bool dda = (psg->ch[psg->selector].ctrl >> 6) & 0b01;
        if(dda)
            psg->ch[psg->selector].dac = value & 0x1F;
        else {
            psg->ch[psg->selector].data[psg->ch[psg->selector].idx] = value & 0x1F;
            psg->ch[psg->selector].idx = (psg->ch[psg->selector].idx + 1) & 0x1F;
        }
        break;

        case PSG_NOISE:
        if(psg->selector < 4)
            break;
        lfsr_t* lfsr = &psg->lfsr[psg->selector - 4];
        lfsr->on = value & 0x80;
        u8 freq = value & 0x1F;
        lfsr->freq = freq == 0x1F ? 32 : (~value & 0x1F) << 6;
        break;
    }
}

static void psg_draw_wave(int* pixels, const u8* wave, int start_x, int start_y, int w, int h) {   
    const int white = color(255, 255, 255);
    int last = wave[0] * 4;

    for (int x = 0; x < 32; x++) {
        int current = wave[x] * 4;

        for (int i = 0; i < 4; i++) {
            int lerp = last + (current - last) * i / 4;
            int step = (lerp > last) ? 1 : -1;
            for (int y = last; y != lerp; y += step) {
                pixels[(start_x + x * 4 + i) + (start_y - y) * w] = white;
            }
            // draw the final point
            pixels[(start_x + x * 4 + i) + (start_y - lerp) * w] = white;
            last = lerp;
        }
    }
}

static u8 psg_update_wave(ch_t* ch) {
    u8 data = ch->data[ch->idx];
    ch->idx = (ch->idx + 1) & 0x1F;
    return data;
}

static u8 psg_update_lfsr(lfsr_t* lfsr) {
    bool bit0 = lfsr->seed & (1 << 0);
    bool bit1 = lfsr->seed & (1 << 1);
    bool bit11 = lfsr->seed & (1 << 11);
    bool bit12 = lfsr->seed & (1 << 12);
    bool bit17 = lfsr->seed & (1 << 17);
    bool feedback = bit0 ^ bit1 ^ bit11 ^ bit12 ^ bit17;

    lfsr->seed = (lfsr->seed >> 1) | (feedback << 17);
    return lfsr->seed & 1 ? 0x1F : 0x00;
}

static void psg_get_sample(void* ctx, void* data){
    psg_t* psg = (psg_t*)ctx;
    static float volume_lut[32] = {
        0.005208f, 0.004382f, 0.003687f, 0.003102f, 0.002610f, 0.002196f, 0.001848f, 0.001555f,
        0.001308f, 0.001101f, 0.000926f, 0.000779f, 0.000656f, 0.000552f, 0.000464f, 0.000391f,
        0.000329f, 0.000277f, 0.000233f, 0.000196f, 0.000165f, 0.000139f, 0.000117f, 0.000098f,
        0.000083f, 0.000069f, 0.000058f, 0.000049f, 0.000041f, 0.000035f, 0.0f,      0.0f
    };

    sample_t sample = {0, 0};
    for(int i = 0; i < PSG_CHANNELS; i++){
        // audio mixing is tricky, refers to psg manual
        ch_t* ch = &psg->ch[i];
        u8 ch_vol = (ch->ctrl >> 1) & 0xF;
        bool extra_attenuation = !(ch->ctrl & 1);
        int left_idx = (15 - ch_vol) + (15 - ch->left_vol) + (15 - psg->left_vol);
        int right_idx = (15 - ch_vol) + (15 - ch->right_vol) + (15 - psg->right_vol);
        if(left_idx > 0xF) left_idx = 0xF;
        if(right_idx > 0xF) right_idx = 0xF;
        left_idx = (left_idx << 1) | extra_attenuation;
        right_idx = (right_idx << 1) | extra_attenuation;
        sample.left += ch->dac * volume_lut[left_idx];
        sample.right += ch->dac * volume_lut[right_idx];
    }
    
    memcpy(data, &sample, sizeof(sample_t));
}

static void psg_push_sample(psg_t* psg, u32 cycles) {
    sample_t sample;
    sound_push_sample(cycles, sizeof(sample_t), psg, &sample, psg_get_sample);
}

void pce_psg_step(psg_t* psg, u32 cycles) {
    for (int i = 0; i < PSG_CHANNELS; i++) {
        ch_t* ch = &psg->ch[i];
        bool on = ch->ctrl & 0x80;
        bool dda = (ch->ctrl >> 6) & 0b01;
        bool noise = false;
        if(i >= 4)
            noise = psg->lfsr[i - 4].on;
        if(!on || dda)
            continue;
        ch->freq_counter -= cycles;
        if(ch->freq_counter <= 0) {
            ch->freq_counter += noise ? psg->lfsr[i - 4].freq << 1 : ch->freq << 1;
            ch->dac = noise ? psg_update_lfsr(&psg->lfsr[i - 4]) : psg_update_wave(ch);
        }
    }

    psg_push_sample(psg, cycles);
}


void pce_psg_draw_waveforms(SDL_Window** win, psg_t* psg){
    Uint32 id = SDL_GetWindowID(*win);
    if(!id){
        *win = NULL;
        return;
    }
    SDL_Surface* s = SDL_GetWindowSurface(*win);
    SDL_FillSurfaceRect(s, NULL, color(0, 0, 0));

    int* pixels = (int*)s->pixels;

    // draw grid
    int grey = color(60, 60, 60);
    for(int grid_y = 1; grid_y < 3; grid_y++)
        for(int i = 0; i < s->h; i++)
            pixels[s->w*grid_y/3 + i * s->w] = grey;

    for(int i = 0; i < s->w; i++)
        pixels[i + s->h/2 * s->w] = grey;

    for(int ty = 0; ty < 2; ty++){
        for(int tx = 0; tx < 3; tx++){
            int wave_idx = tx + ty * 3;
            int x = tx * s->w / 3;
            int y = ((ty+1) * s->h / 2) - 1;
            psg_draw_wave(pixels, psg->ch[wave_idx].data, x, y, s->w, s->h);     
        }
    }

    SDL_UpdateWindowSurface(*win);
}