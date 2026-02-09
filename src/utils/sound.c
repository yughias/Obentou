#include "utils/sound.h"

#include "types.h"
#include "SDL_MAINLOOP.h"
#include <stdio.h>
#include <string.h>

#define AUDIO_BUFFER_SIZE (1 << 16)
#define AUDIO_BUFFER_MASK (AUDIO_BUFFER_SIZE - 1)

static SDL_AudioStream* audio_stream;
static audio_callback_ptr audio_callback;
static u8 audio_buffer[AUDIO_BUFFER_SIZE];
static u8 silence_buffer[AUDIO_BUFFER_SIZE];
static float push_rate_counter;
static float push_rate_reload = -1;
static float push_rate_scaled;
static bool is_paused = true;
static SDL_AtomicInt rb_read;
static SDL_AtomicInt rb_write;
bool sound_channel_muted[MAX_AUDIO_CHANNELS];

#define DISPLAY_BUFFER_SAMPLES 1024

typedef struct wave_info_t {
    int buffer[DISPLAY_BUFFER_SAMPLES];
    int idx;
    int min;
    int max;
} wave_info_t;

wave_info_t wave_info[MAX_AUDIO_CHANNELS];

int sound_set_channel_sample(int sample, int channel){
    int out = sample;
    if(sound_channel_muted[channel])
        out = 0;

    if(wave_info[channel].idx != DISPLAY_BUFFER_SAMPLES)
        wave_info[channel].buffer[wave_info[channel].idx++] = out;
    return out;
}

static bool draw_wave(void* arg){
    wave_info_t* info = (wave_info_t*)arg;
    const int white = -1;

    if(info->idx < DISPLAY_BUFFER_SAMPLES) return false;
    info->idx = 0;

    int range = info->max - info->min;

    float avg = 0;
    for(int i = 0; i < DISPLAY_BUFFER_SAMPLES; i++)
        avg += info->buffer[i];
    avg /= DISPLAY_BUFFER_SAMPLES;

    int local_min = info->max;
    int local_max = info->min;
    for(int i = 0; i < DISPLAY_BUFFER_SAMPLES; i++){
        local_min = SDL_min(local_min, info->buffer[i]);
        local_max = SDL_max(local_max, info->buffer[i]);
    }

    int trigger_idx = 0;
    int candidate_idx = 0;
    int state = 0;

    for(int i = width / 2; i < DISPLAY_BUFFER_SAMPLES; i++){
        int val = info->buffer[i];
        int prev_val = info->buffer[i-1];

        if (state == 0) {
            // STATE 0: ARMING
            if (prev_val == local_min && val != local_min) {
                candidate_idx = i;
                state = 1;
            }
        } 
        else if (state == 1) {
            // STATE 1: MAX
            if (val == local_max) {
                state = 2;
            }
        } 
        else if (state == 2) {
            // STATE 2: OFF
            if (prev_val != local_min && val == local_min) {
                trigger_idx = candidate_idx;
                break;
            }
        }
    }

    int idx = trigger_idx - (width / 2);
    if(idx < 0) idx = 0;

    int prev_y;
    int center_y = height - 32;

    for(int i = 0; i < width; i++){
        int sample_idx = idx++;
        if(sample_idx >= DISPLAY_BUFFER_SAMPLES) 
            sample_idx = DISPLAY_BUFFER_SAMPLES - 1;

        int val = info->buffer[sample_idx];

        int offset = ((val - info->min) * (center_y - 32)) / range;
        
        int sample_y = center_y - offset; 

        if(i == 0) prev_y = sample_y;

        int y_min = (sample_y < prev_y) ? sample_y : prev_y;
        int y_max = (sample_y < prev_y) ? prev_y : sample_y;

        if (y_min < 0) y_min = 0;
        if (y_max >= height) y_max = height - 1;

        if (y_min < height && y_max >= 0) {
            for(int j = y_min; j <= y_max; j++)
                pixels[i + j * width] = white;
        }

        prev_y = sample_y;
    }

    return true;
}

void sound_open_wave_viewer(const char* name, int min, int max, int idx){
    wave_info[idx].idx = 0;
    wave_info[idx].min = min;
    wave_info[idx].max = max;
    createWidget(name, 512, 512, draw_wave, wave_info + idx);
}

void sound_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    if(isGrabbed()) {
        SDL_PutAudioStreamData(stream, silence_buffer, additional_amount);
        return;
    }

    if(audio_callback){       
        if(additional_amount > 0){
            Uint8* data = SDL_stack_alloc(Uint8, additional_amount);
            audio_callback(userdata, data, additional_amount);
            SDL_PutAudioStreamData(stream, data, additional_amount);
            SDL_stack_free(data);
        }
    } else {
        int read_pos = SDL_GetAtomicInt(&rb_read);
        int write_pos = SDL_GetAtomicInt(&rb_write);
        int available = write_pos - read_pos;

        if (available > additional_amount) {
            int to_write = (available < total_amount) ? available : total_amount;
            
            int read_idx = read_pos & AUDIO_BUFFER_MASK;
            int chunk1 = AUDIO_BUFFER_SIZE - read_idx;
            
            if (to_write <= chunk1) {
                SDL_PutAudioStreamData(stream, &audio_buffer[read_idx], to_write);
            } else {
                SDL_PutAudioStreamData(stream, &audio_buffer[read_idx], chunk1);
                SDL_PutAudioStreamData(stream, &audio_buffer[0], to_write - chunk1);
            }
            
            SDL_AddAtomicInt(&rb_read, to_write);
        } else {
            SDL_PutAudioStreamData(stream, silence_buffer, additional_amount);
        }
    }
}

void sound_open(SDL_AudioSpec *audio_spec, audio_callback_ptr callback, void* userdata) {
    if(audio_stream)
        sound_close();
    audio_callback = callback;
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, audio_spec, sound_callback, userdata);
    SDL_SetAtomicInt(&rb_read, 0);
    SDL_SetAtomicInt(&rb_write, 0);
}

void sound_pause(bool pause) {
    is_paused = pause;
    if(pause == SDL_AudioStreamDevicePaused(audio_stream))
        return;
    if(pause)
        SDL_PauseAudioStreamDevice(audio_stream);
    else
        SDL_ResumeAudioStreamDevice(audio_stream);
}

void sound_close() {
    SDL_DestroyAudioStream(audio_stream);
    sound_set_push_rate(-1);
    audio_callback = NULL;
    audio_stream = NULL;
    is_paused = true;
    SDL_SetAtomicInt(&rb_read, 0);
    SDL_SetAtomicInt(&rb_write, 0);
    memset(sound_channel_muted, 0, sizeof(sound_channel_muted));
}

void sound_set_push_rate(float push_rate) {
    push_rate_counter = 0;
    push_rate_reload = push_rate;
    push_rate_scaled = push_rate_reload;
    SDL_SetAtomicInt(&rb_read, 0);
    SDL_SetAtomicInt(&rb_write, 0);
}

void sound_set_push_rate_multiplier(int multiplier) {
    push_rate_scaled = push_rate_reload * multiplier;
    // set frequency ratio only if callback method is used
    if(audio_callback)
        SDL_SetAudioStreamFrequencyRatio(audio_stream, multiplier);
}

float sound_get_push_rate(){
    return push_rate_reload;
}

void sound_push_sample(int cycles, int sample_size, void* ctx, void* sample, sound_get_sample_ptr func) {
    if(is_paused)
        return;
    push_rate_counter -= cycles;
    while(push_rate_counter <= 0) {
        push_rate_counter += push_rate_scaled;
        
        int read_pos = SDL_GetAtomicInt(&rb_read);
        int write_pos = SDL_GetAtomicInt(&rb_write);
        int free_space = AUDIO_BUFFER_SIZE - (write_pos - read_pos);

        if(free_space >= sample_size){
            func(ctx, sample);
            
            int write_idx = write_pos & AUDIO_BUFFER_MASK;
            int chunk1 = AUDIO_BUFFER_SIZE - write_idx;
            
            if(sample_size <= chunk1) {
                memcpy(&audio_buffer[write_idx], sample, sample_size);
            } else {
                memcpy(&audio_buffer[write_idx], sample, chunk1);
                memcpy(&audio_buffer[0], (u8*)sample + chunk1, sample_size - chunk1);
            }
            
            SDL_AddAtomicInt(&rb_write, sample_size);
        }
    }
}

void sound_queue_samples(const void* samples, size_t size){ 
    int speed = push_rate_scaled / push_rate_reload + 0.5f;
    SDL_SetAudioStreamFrequencyRatio(audio_stream, speed);
    SDL_PutAudioStreamData(audio_stream, samples, size);
}