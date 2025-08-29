#ifndef _BYTEPUSHER_H_
#define _BYTEPUSHER_H_

#include <SDL_MAINLOOP.h>
#include "types.h"

#define HERTZ_PER_FRAME 65536
#define MEMORY_SIZE     0x1000008
#define BYTEPUSHER_W    256
#define BYTEPUSHER_H    256

typedef struct bytepusher_t {
    u8 memory[MEMORY_SIZE];
    SDL_AudioDeviceID audio_dev;
} bytepusher_t;

#endif