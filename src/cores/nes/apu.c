#include "cores/nes/nes.h"
#include "peripherals/sound.h"

static const bool duty_lut[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1}
};

static const u8 length_counter_lut[32] = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static const u8 dmc_divider_lut[16] = {
    214, 190, 170, 160, 143, 127, 113, 107, 95, 80, 71, 64, 53, 42, 36, 27
};

static const u8 triangle_samples_lut[32] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0
};

static const u16 noise_timer_lut[16] = {
    2, 4, 8, 16, 32, 48, 64, 80, 101, 127, 190, 254, 381, 508, 1017, 2034
};

static float pulse_table[31] = {
    0.000000, 0.011609, 0.022939, 0.034001, 0.044803, 0.055355, 0.065665, 0.075741,
    0.085591, 0.095224, 0.104645, 0.113862, 0.122882, 0.131710, 0.140353, 0.148816,
    0.157105, 0.165226, 0.173183, 0.180981, 0.188626, 0.196120, 0.203470, 0.210679,
    0.217751, 0.224689, 0.231499, 0.238182, 0.244744, 0.251186, 0.257513
};

static float tnd_table[203] = {
    0.000000, 0.006700, 0.013345, 0.019936, 0.026474, 0.032959, 0.039393, 0.045775, 0.052106, 0.058386, 0.064618, 0.070800, 0.076934, 0.083020, 0.089058, 0.095050,
    0.100996, 0.106896, 0.112751, 0.118561, 0.124327, 0.130049, 0.135728, 0.141365, 0.146959, 0.152512, 0.158024, 0.163494, 0.168925, 0.174315, 0.179666, 0.184978,
    0.190252, 0.195487, 0.200684, 0.205845, 0.210968, 0.216054, 0.221105, 0.226120, 0.231099, 0.236043, 0.240953, 0.245828, 0.250669, 0.255477, 0.260252, 0.264993,
    0.269702, 0.274379, 0.279024, 0.283638, 0.288220, 0.292771, 0.297292, 0.301782, 0.306242, 0.310673, 0.315074, 0.319446, 0.323789, 0.328104, 0.332390, 0.336649,
    0.340879, 0.345083, 0.349259, 0.353408, 0.357530, 0.361626, 0.365696, 0.369740, 0.373759, 0.377752, 0.381720, 0.385662, 0.389581, 0.393474, 0.397344, 0.401189,
    0.405011, 0.408809, 0.412584, 0.416335, 0.420064, 0.423770, 0.427454, 0.431115, 0.434754, 0.438371, 0.441966, 0.445540, 0.449093, 0.452625, 0.456135, 0.459625,
    0.463094, 0.466543, 0.469972, 0.473380, 0.476769, 0.480138, 0.483488, 0.486818, 0.490129, 0.493421, 0.496694, 0.499948, 0.503184, 0.506402, 0.509601, 0.512782,
    0.515946, 0.519091, 0.522219, 0.525330, 0.528423, 0.531499, 0.534558, 0.537601, 0.540626, 0.543635, 0.546627, 0.549603, 0.552563, 0.555507, 0.558434, 0.561346,
    0.564242, 0.567123, 0.569988, 0.572838, 0.575673, 0.578493, 0.581298, 0.584088, 0.586863, 0.589623, 0.592370, 0.595101, 0.597819, 0.600522, 0.603212, 0.605887,
    0.608549, 0.611197, 0.613831, 0.616452, 0.619059, 0.621653, 0.624234, 0.626802, 0.629357, 0.631899, 0.634428, 0.636944, 0.639448, 0.641939, 0.644418, 0.646885,
    0.649339, 0.651781, 0.654212, 0.656630, 0.659036, 0.661431, 0.663813, 0.666185, 0.668544, 0.670893, 0.673229, 0.675555, 0.677869, 0.680173, 0.682465, 0.684746,
    0.687017, 0.689276, 0.691525, 0.693763, 0.695991, 0.698208, 0.700415, 0.702611, 0.704797, 0.706973, 0.709139, 0.711294, 0.713440, 0.715576, 0.717702, 0.719818,
    0.721924, 0.724021, 0.726108, 0.728186, 0.730254, 0.732313, 0.734362, 0.736402, 0.738433, 0.740455, 0.742468
};

void nes_apu_sync(apu_t* apu){
    nes_t* nes = (nes_t*)apu->ctx;

    nes_apu_update_triangle(&apu->triangle);

    if(nes->cpu.cycles & 1){
        nes_apu_update_pulse(&apu->pulses[0]);
        nes_apu_update_pulse(&apu->pulses[1]);
        nes_apu_update_noise(&apu->noise);
        nes_apu_update_dmc(apu);
        nes_apu_push_sample(apu);

        switch(apu->frame_counter){
            case 3728:
            nes_apu_clock_quarter_frame(apu);
            break;

            case 7456:
            nes_apu_clock_quarter_frame(apu);
            nes_apu_clock_half_frame(apu);
            break;

            case 11185:
            nes_apu_clock_quarter_frame(apu);
            break;

            case 14914:
            if(apu->sequencer_mode == 4){
                nes_apu_clock_quarter_frame(apu);
                nes_apu_clock_half_frame(apu);
                if(!apu->inhibit_irq)
                    apu->frame_irq = true;
                apu->frame_counter = 0;
            }
            break;

            case 18640:
            if(apu->sequencer_mode == 5){
                nes_apu_clock_quarter_frame(apu);
                nes_apu_clock_half_frame(apu);
                apu->frame_counter = 0;
            }
            break;
        }

        apu->frame_counter += 1;
    }
}

void nes_apu_push_sample(apu_t* apu){
    float sample;
    sound_push_sample(1, sizeof(float), apu, &sample, nes_apu_get_sample);
}

void nes_apu_write_pulse_0(pulse_t* pulse, u8 byte){
    pulse->duty_table = byte >> 6;
    pulse->envelope.volume = byte & 0x0F;
    pulse->envelope.mode = byte & 0x10;
    pulse->length.halt = byte & 0x20;
}

void nes_apu_write_pulse_1(pulse_t* pulse, u8 byte){
    pulse->sweep_shift = byte & 0b111;
    pulse->sweep_enabled = byte & 0x80;
    pulse->sweep_p = (byte >> 4) & 0b111;
    pulse->sweep_negate = byte & 0x08;
    pulse->sweep_reload_flag = true;
}

void nes_apu_write_pulse_2(pulse_t* pulse, u8 byte){
    pulse->timer_reload &= 0xFF00;
    pulse->timer_reload |= byte;
}

void nes_apu_write_pulse_3(pulse_t* pulse, u8 byte){
    pulse->timer_reload &= 0xFF;
    pulse->timer_reload |= (byte & 0b111) << 8;
    pulse->envelope.start_flag = true;
    pulse->duty_idx = 0;
    if(pulse->enabled)
        pulse->length.counter = length_counter_lut[byte >> 3];
}

void nes_apu_write_triangle_0(triangle_t* triangle, u8 byte){
    triangle->linear_counter_reload = byte & 0x7F;
    triangle->length.halt = byte & 0x80;
}

void nes_apu_write_triangle_1(triangle_t* triangle, u8 byte){
    triangle->timer_reload &= 0xFF00;
    triangle->timer_reload |= byte;
}

void nes_apu_write_triangle_2(triangle_t* triangle, u8 byte){
    triangle->timer_reload &= 0xFF;
    triangle->timer_reload |= (byte & 0b111) << 8;
    triangle->linear_reload_flag = true;
    if(triangle->enabled)
        triangle->length.counter = length_counter_lut[byte >> 3];
}

void nes_apu_write_noise_0(noise_t* noise, u8 byte){
    noise->envelope.volume = byte & 0x0F;
    noise->envelope.mode = byte & 0x10;
    noise->length.halt = byte & 0x20;
}

void nes_apu_write_noise_1(noise_t* noise, u8 byte){
    noise->mode = byte & 0x80;
    noise->timer_reload = noise_timer_lut[byte & 0xF];
}

void nes_apu_write_noise_2(noise_t* noise, u8 byte){
    noise->envelope.start_flag = true;
    if(noise->enabled)
        noise->length.counter = length_counter_lut[byte >> 3];
}

void nes_apu_write_dmc_0(dmc_t* dmc, u8 byte){
    dmc->irq_enabled = byte & 0x80;
    dmc->irq = dmc->irq && dmc->irq_enabled;
    dmc->loop = byte & 0x40;
    dmc->divider_reload = dmc_divider_lut[byte & 0xF];
    dmc->divider = dmc->divider_reload;
}

void nes_apu_write_dmc_1(dmc_t* dmc, u8 byte){
    dmc->output = byte & 0x7F;
}

void nes_apu_write_dmc_2(dmc_t* dmc, u8 byte){
    dmc->address_reload = 0xC000 + (byte << 6);
    dmc->address = dmc->address_reload;
}

void nes_apu_write_dmc_3(dmc_t* dmc, u8 byte){
    dmc->length_reload = (byte << 4) + 1;
    dmc->length = dmc->length_reload;
}

void nes_apu_update_pulse(pulse_t* pulse){
    if(!pulse->timer) {
        pulse->timer = pulse->timer_reload;
        pulse->duty_idx = (pulse->duty_idx + 1) & 0b111;
    } else {
        pulse->timer -= 1;
    }
}

void nes_apu_update_triangle(triangle_t* triangle){
    if(!triangle->enabled || triangle->timer_reload < 8 || !triangle->length.counter || !triangle->linear_counter)
        return;

    if(!triangle->timer) {
        triangle->timer = triangle->timer_reload;
        triangle->sample_idx = (triangle->sample_idx + 1) & 0x1F;
    } else {
        triangle->timer -= 1;
    }
}

void nes_apu_update_noise(noise_t* noise){
    if(!noise->timer) {
        noise->timer = noise->timer_reload;
        bool bit = noise->lfsr & (1 << (noise->mode ? 6 : 1));
        bool xor_bit = (noise->lfsr & 1) ^ bit;
        noise->lfsr >>= 1;
        noise->lfsr |= xor_bit << 14;
    } else {
        noise->timer -= 1;
    }
}

void nes_apu_update_dmc(apu_t* apu){
    dmc_t* dmc = &apu->dmc;
    if(!dmc->divider){
        dmc->divider = dmc->divider_reload;
        if(dmc->empty && dmc->length){
            nes_t* n = (nes_t*)apu->ctx;
            m6502_t* m = &n->cpu;
            m->cycles += 1;
            dmc->empty = false;
            dmc->buffer = m->read(m->ctx, dmc->address);
            dmc->address += 1;
            if(!dmc->address)
                dmc->address = 0x8000;
            dmc->length -= 1;
            if(!dmc->length){
                if(dmc->loop){
                    dmc->length = dmc->length_reload;
                    dmc->address = dmc->address_reload;
                } else if(dmc->irq_enabled)
                    dmc->irq = true;
            }
        }
        if(!dmc->silence_flag){
            bool bit = dmc->shifter & 1;
            if(bit){
                if(dmc->output < 126)
                    dmc->output += 2;
            } else {
                if(dmc->output > 1)
                    dmc->output -= 2;
            }
            dmc->shifter >>= 1;
            dmc->bit_counter -= 1;
            if(!dmc->bit_counter){
                dmc->bit_counter = 8;
                if(!dmc->empty){
                    dmc->silence_flag = false;
                    dmc->shifter = dmc->buffer;
                    dmc->empty = true;
                } else {
                    dmc->silence_flag = true;
                }
            }
        }
    } else {
        dmc->divider -= 1;
    }
}

void nes_apu_get_sample(void* ctx, void* s){
    apu_t* apu = (apu_t*)ctx;
    float* sample = (float*)s;
    u8 ch[5]; 

    ch[0] = apu->mute[0] ? 0 : nes_apu_get_pulse_sample(&apu->pulses[0], 0);
    ch[1] = apu->mute[1] ? 0 : nes_apu_get_pulse_sample(&apu->pulses[1], 1);
    ch[2] = apu->mute[2] ? 0 : nes_apu_get_triangle_sample(&apu->triangle);
    ch[3] = apu->mute[3] ? 0 : nes_apu_get_noise_sample(&apu->noise);
    ch[4] = apu->mute[4] ? 0 : nes_apu_get_dmc_sample(&apu->dmc);

    if(apu->display_idx != DISPLAY_BUFFER_SIZE){
        for(int i = 0; i < 5; i++){
            apu->display_buffers[i][apu->display_idx] = ch[i];
        }
        apu->display_idx += 1;
    }

    float pulse_out = pulse_table[ch[0] + ch[1]];
    float tnd_out = tnd_table[3*ch[2] + 2*ch[3] + ch[4]];
    *sample = (tnd_out + pulse_out);
}

u8 nes_apu_get_pulse_sample(pulse_t* pulse, bool idx){
    if(!pulse->enabled || pulse->timer_reload < 8 || !pulse->length.counter)
        return 0;
    if(pulse->sweep_enabled && nes_apu_get_sweep_target(pulse, idx) >= 0x800)
        return 0;
    u8 volume = pulse->envelope.mode ? pulse->envelope.volume : pulse->envelope.decay;
    return duty_lut[pulse->duty_table][pulse->duty_idx] * volume;
}

u8 nes_apu_get_triangle_sample(triangle_t* triangle){
    return triangle_samples_lut[triangle->sample_idx];
}

u8 nes_apu_get_noise_sample(noise_t* noise){
    if(!noise->enabled || !noise->length.counter)
        return 0;
    if(noise->lfsr & 1)
        return 0;
    return noise->envelope.mode ? noise->envelope.volume : noise->envelope.decay;
}

u8 nes_apu_get_dmc_sample(dmc_t* dmc){
    return dmc->output;
}

void nes_apu_write_mi_reg(apu_t* apu, u8 byte){
    bool mode = byte & 0x80;
    apu->inhibit_irq = byte & 0x40;
    apu->frame_irq = apu->frame_irq && !apu->inhibit_irq;
    apu->sequencer_mode = mode ? 5 : 4;
    apu->frame_counter = 0;
    if(mode){
        nes_apu_clock_quarter_frame(apu);
        nes_apu_clock_half_frame(apu);
    }
}

void nes_apu_clock_length_counters(apu_t* apu){
    for(int i = 0; i < 2; i++){
        pulse_t* pulse = &apu->pulses[i];
        nes_apu_update_length_counter(&pulse->length);
    }

    triangle_t* triangle = &apu->triangle;
    nes_apu_update_length_counter(&triangle->length);

    noise_t* noise = &apu->noise;
    nes_apu_update_length_counter(&noise->length);
}

void nes_apu_clock_envelope_counters(apu_t* apu){
    for(int i = 0; i < 2; i++){
        pulse_t* pulse = &apu->pulses[i];
        nes_apu_update_envelope(&pulse->envelope, pulse->length.halt);
    }

    noise_t* noise = &apu->noise;
    nes_apu_update_envelope(&noise->envelope, noise->length.halt);
}

void nes_apu_clock_sweep_counters(apu_t* apu){
    for(int i = 0; i < 2; i++){
        pulse_t* pulse = &apu->pulses[i];
        if(pulse->sweep_enabled && !pulse->sweep_divider && pulse->sweep_shift){
            u16 target = nes_apu_get_sweep_target(pulse, i);
            if(target < 0x800)
                pulse->timer_reload = target;
        }
        if(!pulse->sweep_divider || pulse->sweep_reload_flag){
            pulse->sweep_reload_flag = false;
            pulse->sweep_divider = pulse->sweep_p;
        } else {
            pulse->sweep_divider -= 1;
        }
    }
}

void nes_apu_clock_linear_counter(apu_t* apu){
    triangle_t* triangle = &apu->triangle;
    
    if(triangle->linear_reload_flag){
        triangle->linear_counter = triangle->linear_counter_reload;
    } else if(triangle->linear_counter) {
        triangle->linear_counter -= 1;
    }

    if(!triangle->length.halt){
        triangle->linear_reload_flag = false;
    }

}

u16 nes_apu_get_sweep_target(pulse_t* pulse, bool idx){
    u16 timer = pulse->timer_reload;
    u16 shifted = timer >> pulse->sweep_shift;
    return pulse->sweep_negate ? timer + ~(shifted) + idx : timer + shifted;
}

void nes_apu_write_control(apu_t* apu, u8 byte){
    apu->pulses[0].enabled = byte & 0b01;
    if(!apu->pulses[0].enabled)
        apu->pulses[0].length.counter = 0;
    apu->pulses[1].enabled = byte & 0b10;
    if(!apu->pulses[1].enabled)
        apu->pulses[1].length.counter = 0;
    apu->triangle.enabled = byte & 0b100;
    if(!apu->triangle.enabled)
        apu->triangle.length.counter = 0;
    apu->noise.enabled = byte & 0b1000;
    if(!apu->noise.enabled)
        apu->noise.length.counter = 0;
    if(byte & (1 << 4)){
        apu->dmc.silence_flag = false;
        apu->dmc.address = apu->dmc.address_reload;
        apu->dmc.length = apu->dmc.length_reload;
    } else {
        apu->dmc.silence_flag = true;
        apu->dmc.irq = false;
        apu->dmc.length = 0;
    }
}

u8 nes_apu_get_status(apu_t* apu){
    u8 byte = 0;
    byte |= apu->pulses[0].length.counter > 0;
    byte |= (apu->pulses[1].length.counter > 0) << 1;
    byte |= (apu->triangle.length.counter > 0) << 2;
    byte |= (apu->noise.length.counter > 0) << 3; 
    byte |= (apu->dmc.bit_counter != 0) << 4;
    byte |= apu->frame_irq << 6;
    byte |= apu->dmc.irq << 7;

    apu->frame_irq = false;
    return byte;
}


void nes_apu_draw_wave(int x0, int y0, u8* buffer, int scale, SDL_Surface* s){
    int* pixels = (int*)s->pixels;
    const int white = color(255, 255, 255);

    float avg = 0;
    for(int i = 0; i < DISPLAY_BUFFER_SIZE; i++)
        avg += buffer[i];
    avg /= DISPLAY_BUFFER_SIZE;

    y0 += avg * scale;

    int idx = 0;
    int start = -1;
    int end = -1;
    for(int i = s->w/4; i < DISPLAY_BUFFER_SIZE; i++){
        u8 s0 = buffer[i-1];
        u8 s1 = buffer[i];
        if(start == -1 && s0 < avg && s1 >= avg){
            start = i;
        } else if(start != -1 && s1 < avg && s0 >= avg){
            end = i;
            break;
        } 
    }

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

void nes_apu_draw_waves(apu_t* apu, SDL_Window** win){
    Uint32 id = SDL_GetWindowID(*win);
    if(!id){
        *win = NULL;
        return;
    }
    // display waveforms only if buffer is full
    if(apu->display_idx != DISPLAY_BUFFER_SIZE)
        return;
    SDL_Surface* s = SDL_GetWindowSurface(*win);
    int* pixels = (int*)s->pixels;
    SDL_FillRect(s, NULL, 0);

    for(int y = 0; y < 2; y++){
        for(int x = 0; x < 2; x++){
            int idx =  x + y*2;
            int x0 = x*s->w/2;
            int y0 = s->h/6 + y*s->h/3;
            u8* buf = apu->display_buffers[idx];
            nes_apu_draw_wave(x0, y0, buf, 4, s);
        }   
    }

    // DMC
    nes_apu_draw_wave(0, s->h - s->h/6, apu->display_buffers[4], 1, s);

    const int grey = color(100, 100, 100);
    for(int i = 0; i < s->w; i++){
        pixels[i + s->h / 3 * s->w] = grey;
        pixels[i + 2 * s->h / 3 * s->w] = grey;
    }
    for(int i = 0; i < s->h; i++)
        pixels[s->w / 2 + i * s->w] = grey;
    
    apu->display_idx = 0;

    SDL_UpdateWindowSurface(*win);
}

void nes_apu_clock_quarter_frame(apu_t* apu){
    nes_apu_clock_envelope_counters(apu);
    nes_apu_clock_linear_counter(apu);
}

void nes_apu_clock_half_frame(apu_t* apu){
    nes_apu_clock_length_counters(apu);
    nes_apu_clock_sweep_counters(apu);
}

void nes_apu_update_length_counter(length_counter_t* length){
    if(length->counter && !(length->halt)){
        length->counter -= 1;
    }
}


void nes_apu_update_envelope(envelope_t* envelope, bool halt){
    if(envelope->start_flag){
        envelope->start_flag = false;
        envelope->decay = 15;
        envelope->divider = envelope->volume;
    } else {
        if(envelope->divider) {
            envelope->divider -= 1;
        } else {
            envelope->divider = envelope->volume;
            if(envelope->decay)
                envelope->decay -= 1;
            else if(halt)
                envelope->decay = 15;
        }
    }
}