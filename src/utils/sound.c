#include "utils/sound.h"
#include "types.h"
#include "SDL_MAINLOOP.h"
#include <stdio.h>
#include <string.h>

#define AUDIO_BUFFER_SIZE (1 << 16)
#define AUDIO_BUFFER_MASK (AUDIO_BUFFER_SIZE - 1)

static SDL_AudioStream* audio_stream;
static SDL_AudioStreamCallback audio_callback;
static u8 audio_buffer[AUDIO_BUFFER_SIZE];
static u8 silence_buffer[AUDIO_BUFFER_SIZE];
static float push_rate_counter;
static float push_rate_reload = -1;
static float push_rate_scaled;
static bool is_paused = true;
static SDL_AtomicInt rb_read;
static SDL_AtomicInt rb_write;


void sound_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    if(isGrabbed()) {
        SDL_PutAudioStreamData(stream, silence_buffer, additional_amount);
        return;
    }

    if(audio_callback){
        audio_callback(userdata, stream, additional_amount, total_amount);
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

void sound_open(SDL_AudioSpec *audio_spec, SDL_AudioStreamCallback callback, void* userdata) {
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
    SDL_PutAudioStreamData(audio_stream, samples, size);
}