#ifndef __NES_INTERFACE_H__
#define __NES_INTERFACE_H__

#include "interface.h"

void nes_run_frame(void* ctx);
void* nes_init(const char* filename, SDL_AudioDeviceID device_id);
bool nes_detect(const char* filename);

#define nes_width 256
#define nes_height 240
#define nes_fps 60.0988

#define nes_audio_spec \
{ \
    .callback = NULL, \
    .channels = 1, \
    .format = AUDIO_F32, \
    .freq = 44100, \
    .samples = 4096, \
} \

#endif