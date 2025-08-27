#ifndef __WATARA_INTERFACE_H__
#define __WATARA_INTERFACE_H__

#include "types.h"

void watara_run_frame(void* ctx);
bool watara_detect(const char* filename);
void* watara_init(const char* filename, SDL_AudioDeviceID device_id);


#define watara_width 160
#define watara_height 160
#define watara_fps 61.04

#define watara_audio_spec \
{ \
    .callback = NULL, \
    .channels = 2, \
    .format = AUDIO_S8, \
    .freq = 44100, \
    .samples = 4096, \
} \

#endif