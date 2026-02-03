#ifndef __CORES_H__
#define __CORES_H__

#include "SDL3/SDL.h"

#include "types.h"

#include "utils/controls.h"
#include "utils/archive.h"
#include "utils/vec.h"
#include "utils/serializer.h"

typedef void* (*init_ptr)(const archive_t* rom_archive, const archive_t* bios_archive);
typedef void (*run_frame_ptr)(void* ctx);
typedef bool (*detect_ptr)(const archive_t* rom_archive, const archive_t* bios_archive);
typedef void (*close_ptr)(void* ctx, const char* sav_path);
typedef byte_vec_t (*savestate_ptr)(void* ctx);
typedef void (*loadstate_ptr)(void* ctx, byte_vec_t* state);

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
    savestate_ptr savestate;
    loadstate_ptr loadstate;
    bool has_bios;
} core_t;

typedef struct core_ctx_t {
    int speed_level;
    bool pause;
    const core_t* core;
    archive_t* rom;
    archive_t* bios;
    void* emu;
} core_ctx_t;

typedef struct ctx_args_t {
    core_ctx_t* ctx;
    int value;
} ctx_args_t;

extern const core_t cores[];
extern const size_t n_cores;

const core_t* core_detect(const archive_t* rom_archive, const archive_t* bios_archive, const char* force_core);
void core_restart(core_ctx_t* ctx);
void core_switch_pause(core_ctx_t* ctx);
void core_ctx_set_speed(ctx_args_t* ctx);
void core_ctx_close(core_ctx_t* ctx);
void core_ctx_init(core_ctx_t* ctx, const char* rom_path, const char* bios_path, const char* force_core);
void core_ctx_run_frame(core_ctx_t* ctx);


#endif
