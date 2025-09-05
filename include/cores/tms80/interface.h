#ifndef __TMS80_INTERFACE_H__
#define __TMS80_INTERFACE_H__

#include "interface.h"

void tms80_run_frame(void* ctx);
void* tms80_init(const char* filename, SDL_AudioDeviceID device_id);
bool tms80_detect(const char* filename);


#define tms80_width 256
#define tms80_height 192
#define tms80_fps 59.9227434033

#define tms80_audio_spec \
{ \
    .callback = NULL, \
    .channels = 1, \
    .format = AUDIO_S16, \
    .freq = 44100, \
    .samples = 4096, \
} \

#endif