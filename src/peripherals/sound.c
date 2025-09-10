#include "peripherals/sound.h"

#include "types.h"

#define AUDIO_BUFFER_SIZE 4096

static SDL_AudioDeviceID audio_dev;
static u8 audio_buffer[AUDIO_BUFFER_SIZE];
static size_t audio_buffer_idx;
static float push_rate_counter;
static float push_rate_reload = -1;

void sound_open(SDL_AudioSpec *audio_spec) {
    audio_dev = SDL_OpenAudioDevice(NULL, 0, audio_spec, NULL, 0);
    SDL_PauseAudioDevice(audio_dev, 0);  
}

void sound_close() {
    SDL_PauseAudioDevice(audio_dev, 1);
    SDL_CloseAudioDevice(audio_dev);  
    sound_set_push_rate(-1);
}

void sound_set_push_rate(float push_rate) {
    audio_buffer_idx = 0;
    push_rate_counter = 0;
    push_rate_reload = push_rate;
}

bool sound_is_push_rate_set(){
    return push_rate_reload > 0.0f;
}

void sound_push_sample(int cycles, int sample_size, void* ctx, void* sample, sound_get_sample_ptr func) {
    push_rate_counter -= cycles;
    if(push_rate_counter <= 0) {
        push_rate_counter += push_rate_reload;
        func(ctx, sample);
        memcpy(audio_buffer + audio_buffer_idx, sample, sample_size);
        audio_buffer_idx += sample_size;
        if(audio_buffer_idx == AUDIO_BUFFER_SIZE) {
            SDL_QueueAudio(audio_dev, audio_buffer, AUDIO_BUFFER_SIZE);
            audio_buffer_idx = 0;
        }
    }
}

void sound_queue_samples(const void* samples, size_t size){
    SDL_QueueAudio(audio_dev, samples, size);
}
