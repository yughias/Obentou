#ifndef __CORES_H__
#define __CORES_H__

#include "SDL2/SDL.h"

#include "types.h"

typedef void* (*init_ptr)(const char* filename, SDL_AudioDeviceID device_id);
typedef void (*run_frame_ptr)(void* ctx);

typedef struct core_t {
    init_ptr init;
    run_frame_ptr run_frame;
    const int width, height;
    const float fps;
    SDL_AudioSpec audio_spec;
} core_t;

#define LOAD_CORE(name) \
{ \
    .init = name##_init, \
    .run_frame = name##_run_frame, \
    .width = name##_width, \
    .height = name##_height, \
    .fps = name##_fps, \
    .audio_spec = name##_audio_spec \
} 

#endif