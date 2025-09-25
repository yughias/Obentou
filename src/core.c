#include "core.h"

#include "peripherals/sound.h"
#include "peripherals/controls.h"

#include "SDL_MAINLOOP.h"

#include <stdio.h>
#include <stdlib.h>

const core_t* core_detect(const archive_t* rom_archive, const archive_t* bios_archive, const char* force_core){
    for(int i = 0; i < sizeof(cores)/sizeof(core_t); i++){
        if(force_core && !strcmp(cores[i].name, force_core)){
            return &cores[i];
        }
        if(cores[i].detect(rom_archive, bios_archive)){
            printf("Detected core: %s\n", cores[i].name);
            return &cores[i];
        }
    }

    printf("Unknown core!\n");
    exit(EXIT_FAILURE);
    return NULL;
}

void core_ctx_init(core_ctx_t* ctx, const char* rom_path, const char* bios_path, const char* force_core){
    memset(ctx, 0, sizeof(core_ctx_t)); 

    ctx->rom = archive_load(rom_path);
    ctx->bios = archive_load(bios_path);
    ctx->core = core_detect(ctx->rom, ctx->bios, force_core);
}

void core_ctx_run_frame(core_ctx_t* ctx){
    const core_t* core = ctx->core;
    core->run_frame(ctx->emu);
}

void core_restart(core_ctx_t* ctx){
    const core_t* core = ctx->core;
    size(core->width, core->height);
    frameRate(core->fps);

    SDL_AudioSpec audio_spec = core->audio_spec;

    //core->free(ctx->emu);
    ctx->emu = core->init(ctx->rom, ctx->bios);

    sound_open(&audio_spec, core->sound_callback, ctx->emu);
    if(!sound_is_push_rate_set())
        sound_set_push_rate(core->sound_push_rate);

    controls_init(core->control_begin, core->control_end);
}