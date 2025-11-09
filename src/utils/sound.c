#include "utils/sound.h"

#include "types.h"

#include "SDL_MAINLOOP.h"

#include <stdio.h>

static SDL_AudioStream* audio_stream;
static SDL_AudioStreamCallback audio_callback;
static float push_rate_counter;
static float push_rate_reload = -1;
static float push_rate_scaled;

void sound_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    if(isGrabbed())
        return;
    audio_callback(userdata, stream, additional_amount, total_amount);
}

void sound_open(SDL_AudioSpec *audio_spec, SDL_AudioStreamCallback callback, void* userdata) {
    if(audio_stream)
        sound_close();
    audio_callback = callback;
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, audio_spec, callback ? sound_callback : NULL, userdata);
}

void sound_pause(bool pause) {
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
}

void sound_set_push_rate(float push_rate) {
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
        SDL_PutAudioStreamData(audio_stream, sample, sample_size);
    }
}

void sound_queue_samples(const void* samples, size_t size){ 
    SDL_PutAudioStreamData(audio_stream, samples, size);
}
