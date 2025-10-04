#ifndef __CORES_H__
#define __CORES_H__

#include "SDL3/SDL.h"

#include "types.h"

#include "utils/controls.h"
#include "utils/archive.h"

#include "cores/watara/interface.h"
#include "cores/pv1000/interface.h"
#include "cores/pce/interface.h"
#include "cores/bytepusher/interface.h"
#include "cores/tms80/interface.h"
#include "cores/nes/interface.h"
#include "cores/gbc/interface.h"

typedef void* (*init_ptr)(const archive_t* rom_archive, const archive_t* bios_archive);
typedef void (*run_frame_ptr)(void* ctx);
typedef bool (*detect_ptr)(const archive_t* rom_archive, const archive_t* bios_archive);
typedef void (*close_ptr)(void* ctx, const char* sav_path);

typedef struct core_t {
    const char name[32];
    detect_ptr detect;
    init_ptr init;
    run_frame_ptr run_frame;
    close_ptr close;
    const int width, height;
    const float fps;
    const float sound_push_rate;
    SDL_AudioSpec audio_spec;
    SDL_AudioStreamCallback sound_callback;
    control_t control_begin;
    control_t control_end;
} core_t;

#define LOAD_CORE(core) \
{ \
    .name = #core, \
    .init = core##_init, \
    .detect = core##_detect, \
    .run_frame = core##_run_frame, \
    .close = core##_close, \
    .width = core##_WIDTH, \
    .height = core##_HEIGHT, \
    .fps = core##_FPS, \
    .audio_spec = core##_AUDIO_SPEC, \
    .sound_push_rate = core##_SOUND_PUSH_RATE, \
    .sound_callback = core##_sound_callback, \
    .control_begin = CONTROL_##core##_BEGIN, \
    .control_end = CONTROL_##core##_END \
}

static const core_t cores[] = {
    LOAD_CORE(WATARA),
    LOAD_CORE(PV1000),
    LOAD_CORE(PCE),
    LOAD_CORE(BYTEPUSHER),
    LOAD_CORE(TMS80),
    LOAD_CORE(NES),
    LOAD_CORE(GBC)
};

#undef LOAD_CORE

typedef struct core_ctx_t {
    SDL_AtomicInt must_wait;

    int speed_level;
    bool pause;
    const core_t* core;
    archive_t* rom;
    archive_t* bios;
    void* emu;
} core_ctx_t;

typedef struct ctx_set_speed_args_t {
    core_ctx_t* ctx;
    int speed;
} ctx_set_speed_args_t;

const core_t* core_detect(const archive_t* rom_archive, const archive_t* bios_archive, const char* force_core);
void core_restart(core_ctx_t* ctx);
void core_switch_pause(core_ctx_t* ctx);
void core_ctx_set_speed(ctx_set_speed_args_t* ctx);
void core_ctx_close(core_ctx_t* ctx);
void core_ctx_init(core_ctx_t* ctx, const char* rom_path, const char* bios_path, const char* force_core);
void core_ctx_run_frame(core_ctx_t* ctx);


#endif