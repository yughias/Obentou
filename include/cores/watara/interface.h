#ifndef __WATARA_INTERFACE_H__
#define __WATARA_INTERFACE_H__

#include "types.h"

void WATARA_run_frame(void* ctx);
bool WATARA_detect(const char* filename);
void* WATARA_init(const char* filename);


#define WATARA_WIDTH 160
#define WATARA_HEIGHT 160
#define WATARA_FPS 61.04
#define WATARA_SOUND_PUSH_RATE (4e6/44100.0f)

#define WATARA_AUDIO_SPEC \
{ \
    .callback = NULL, \
    .channels = 2, \
    .format = AUDIO_S8, \
    .freq = 44100, \
    .samples = 4096, \
} \

#endif