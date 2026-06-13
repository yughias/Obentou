#include "utils/wav.h"

#include <SDL3/SDL.h>

wav_t wav_load(const u8* data, size_t size) {
    wav_t wav = {0};
    SDL_AudioSpec wav_spec;
    u32 wav_size;
    SDL_LoadWAV_IO(SDL_IOFromConstMem(data, size), true, &wav_spec, &wav.data, &wav_size);
    wav.sample_rate = wav_spec.freq;
    wav.size = wav_size;
    wav.channels = wav_spec.channels;
    switch (wav_spec.format) {
        case SDL_AUDIO_U8:
        case SDL_AUDIO_S8:
        wav.bytes_per_sample = 1;
        break;

        case SDL_AUDIO_S16:
        wav.bytes_per_sample = 2;
        break;

        default:
        printf("unsupported wav format %d\n", wav_spec.format);
        break;
    }
    return wav;
}

void wav_free(wav_t* wav) {
    SDL_free(wav->data);
}