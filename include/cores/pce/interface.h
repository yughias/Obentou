#ifndef __PCE_INTERFACE_H__
#define __PCE_INTERFACE_H__

#include "interface.h"

void PCE_run_frame(void* ctx);
void* PCE_init(const char* filename);
bool PCE_detect(const char* filename);


#define PCE_WIDTH 256
#define PCE_HEIGHT 242
#define PCE_FPS 59.8337024193
#define PCE_SOUND_PUSH_RATE (7.16e6/44100.0f)

#define PCE_AUDIO_SPEC \
{ \
    .callback = NULL, \
    .channels = 2, \
    .format = AUDIO_F32, \
    .freq = 44100, \
    .samples = 1024, \
} \

#endif