#include "peripherals/sound.h"

#include "types.h"

#include <stdio.h>

#define AUDIO_BUFFER_SIZE 4096

static SDL_AudioStream* audio_stream;
static u8 audio_buffer[AUDIO_BUFFER_SIZE];
static size_t audio_buffer_idx;
static float push_rate_counter;
static float push_rate_reload = -1;
static int push_rate_scaled;

void sound_open(SDL_AudioSpec *audio_spec, SDL_AudioStreamCallback callback, void* userdata) {
    if(audio_stream)
        sound_close();
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, audio_spec, callback, userdata);
    SDL_ResumeAudioStreamDevice(audio_stream);
}

void sound_close() {
    SDL_DestroyAudioStream(audio_stream);
    sound_set_push_rate(-1);
}

void sound_set_push_rate(float push_rate) {
    audio_buffer_idx = 0;
    push_rate_counter = 0;
    push_rate_reload = push_rate;
    push_rate_scaled = push_rate_reload;
}

void sound_set_push_rate_multiplier(int multiplier) {
    push_rate_scaled = push_rate_reload * multiplier;
}

bool sound_is_push_rate_set(){
    return push_rate_reload > 0.0f;
}

void sound_push_sample(int cycles, int sample_size, void* ctx, void* sample, sound_get_sample_ptr func) {
    push_rate_counter -= cycles;
    if(push_rate_counter <= 0) {
        push_rate_counter += push_rate_scaled;
        func(ctx, sample);
        memcpy(audio_buffer + audio_buffer_idx, sample, sample_size);
        audio_buffer_idx += sample_size;
        if(audio_buffer_idx) {
            if(audio_buffer_idx > AUDIO_BUFFER_SIZE){
                printf("sound_push_sample: audio buffer overflow\n");
                audio_buffer_idx = AUDIO_BUFFER_SIZE;
            }
            SDL_PutAudioStreamData(audio_stream, audio_buffer, audio_buffer_idx);
            audio_buffer_idx = 0;
        }
    }
}

void sound_queue_samples(const void* samples, size_t size){ 
    SDL_PutAudioStreamData(audio_stream, samples, size);
}
