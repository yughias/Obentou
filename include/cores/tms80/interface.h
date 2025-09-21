#ifndef __TMS80_INTERFACE_H__
#define __TMS80_INTERFACE_H__

#include "interface.h"

void TMS80_run_frame(void* ctx);
void* TMS80_init(const char* filename);
bool TMS80_detect(const char* filename);

#define TMS80_WIDTH 256
#define TMS80_HEIGHT 192
#define TMS80_FPS 59.9227434033
#define TMS80_SOUND_PUSH_RATE (3579545.0f/44100.0f)
#define TMS80_sound_callback NULL

#define TMS80_AUDIO_SPEC \
{ \
    .channels = 1, \
    .format = SDL_AUDIO_S16, \
    .freq = 44100, \
} \

#endif