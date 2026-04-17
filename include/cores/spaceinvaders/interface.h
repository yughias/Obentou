#ifndef __SPACEINVADERS_INTERFACE_H__
#define __SPACEINVADERS_INTERFACE_H__

#include "core.h"

void SPACEINVADERS_run_frame(void* ctx);
void* SPACEINVADERS_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool SPACEINVADERS_detect(const archive_t* rom_archive, const archive_t* bios_archive);
byte_vec_t SPACEINVADERS_savestate(void* ctx);
bool SPACEINVADERS_loadstate(void* ctx, byte_vec_t* state);

#define SPACEINVADERS_close  NULL

#define SPACEINVADERS_WIDTH 224
#define SPACEINVADERS_HEIGHT 256
#define SPACEINVADERS_FPS 60.0f

#define SPACEINVADERS_SOUND_PUSH_RATE -1

#define SPACEINVADERS_sound_callback NULL
#define SPACEINVADERS_has_bios false

#define SPACEINVADERS_AUDIO_SPEC \
{ \
    .channels = 1, \
    .format = SDL_AUDIO_S16, \
    .freq = 44100, \
} \

#define SPACEINVADERS_sound_channels
#define SPACEINVADERS_widgets

#endif
