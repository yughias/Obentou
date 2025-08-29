#ifndef __PCE_INTERFACE_H__
#define __PCE_INTERFACE_H__

#include "interface.h"

void pce_run_frame(void* ctx);
void* pce_init(const char* filename, SDL_AudioDeviceID device_id);
bool pce_detect(const char* filename);


#define pce_width 256
#define pce_height 242
#define pce_fps 59.8337024193

#define pce_audio_spec \
{ \
    .callback = NULL, \
    .channels = 2, \
    .format = AUDIO_F32, \
    .freq = 44100, \
    .samples = 1024, \
} \

#endif