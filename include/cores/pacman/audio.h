#ifndef __PACMAN_AUDIO_H__
#define __PACMAN_AUDIO_H__

#include "cores/pacman/pacman.h"
#include "SDL3/SDL.h"

#define PACMAN_AUDIO_GAIN 48

void PACMAN_sound_callback(void* userdata, Uint8* stream, int len);

#endif
