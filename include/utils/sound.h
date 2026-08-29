#ifndef __SOUND_PERIPHERAL_H__
#define __SOUND_PERIPHERAL_H__

#include "SDL3/SDL.h"

#include "types.h"

#define MAX_AUDIO_CHANNELS 8

typedef void (*sound_get_sample_ptr)(void* ctx, void* sample);
typedef void (*audio_callback_ptr)(void* userdata, Uint8* stream, int len);

void sound_open(SDL_AudioSpec *audio_spec, audio_callback_ptr callback, void* userdata);
void sound_close();
void sound_push_sample(int cycles, int sample_size, void* ctx, sound_get_sample_ptr func);
void sound_set_push_rate(float push_rate);
void sound_queue_samples(const void* samples, size_t size);
void sound_pause(bool pause);
void sound_set_push_rate_multiplier(int multiplier);
float sound_get_push_rate();
int sound_get_cycles_until_sample();

int sound_set_channel_sample(int sample, int channel);
void sound_open_wave_viewer(const char* name, int min, int max, int idx);

extern bool sound_channel_muted[MAX_AUDIO_CHANNELS];

#endif