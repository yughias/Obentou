#ifndef __SOUND_PERIPHERAL_H__
#define __SOUND_PERIPHERAL_H__

#include "SDL3/SDL.h"

#include "types.h"

typedef void (*sound_get_sample_ptr)(void* ctx, void* sample);

void sound_open(SDL_AudioSpec *audio_spec, SDL_AudioStreamCallback callback, void* userdata);
void sound_close();
void sound_push_sample(int cycles, int sample_size, void* ctx, void* sample, sound_get_sample_ptr func);
void sound_set_push_rate(float push_rate);
void sound_queue_samples(const void* samples, size_t size);
void sound_set_push_rate_multiplier(int multiplier);
bool sound_is_push_rate_set();

#endif