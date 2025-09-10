#ifndef __CORES_H__
#define __CORES_H__

#include "SDL2/SDL.h"

#include "types.h"

#include "peripherals/controls.h"

typedef void* (*init_ptr)(const char* filename);
typedef void (*run_frame_ptr)(void* ctx);
typedef bool (*detect_ptr)(const char* filename);

typedef struct core_t {
    detect_ptr detect;
    init_ptr init;
    run_frame_ptr run_frame;
    const int width, height;
    const float fps;
    const float sound_push_rate;
    SDL_AudioSpec audio_spec;
    control_t control_begin;
    control_t control_end;
} core_t;

#define LOAD_CORE(name) \
{ \
    .init = name##_init, \
    .detect = name##_detect, \
    .run_frame = name##_run_frame, \
    .width = name##_WIDTH, \
    .height = name##_HEIGHT, \
    .fps = name##_FPS, \
    .audio_spec = name##_AUDIO_SPEC, \
    .sound_push_rate = name##_SOUND_PUSH_RATE, \
    .control_begin = CONTROL_##name##_BEGIN, \
    .control_end = CONTROL_##name##_END \
}

#include "cores/watara/interface.h"
#include "cores/pv1000/interface.h"
#include "cores/pce/interface.h"
#include "cores/bytepusher/interface.h"
#include "cores/tms80/interface.h"
#include "cores/nes/interface.h"
#include "cores/gbc/interface.h"

static const core_t cores[] = {
    LOAD_CORE(WATARA),
    LOAD_CORE(PV1000),
    LOAD_CORE(PCE),
    LOAD_CORE(BYTEPUSHER),
    LOAD_CORE(TMS80),
    LOAD_CORE(NES),
    LOAD_CORE(GBC)
};

#endif