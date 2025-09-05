#ifndef __GBC_INTERFACE_H__
#define __GBC_INTERFACE_H__

#include "interface.h"

void gbc_run_frame(void* ctx);
void* gbc_init(const char* filename, SDL_AudioDeviceID device_id);
bool gbc_detect(const char* filename);

#define gbc_width 160
#define gbc_height 144
#define gbc_fps 59.727500569606

#define gbc_audio_spec \
{ \
    .callback = NULL, \
    .channels = 2, \
    .format = AUDIO_S16, \
    .freq = 44100, \
    .samples = 4096, \
} \


#endif