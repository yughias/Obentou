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

    ctx->speed = 1;
    ctx->rom = archive_load(rom_path);
    ctx->bios = archive_load(bios_path);
    ctx->core = core_detect(ctx->rom, ctx->bios, force_core);
}

void core_ctx_run_frame(core_ctx_t* ctx){
    const core_t* core = ctx->core;

    ctx->speed = hotkeys_pressed(CONTROL_HOTKEY_TURBO) ? 8 : 1;

    sound_set_push_rate_multiplier(ctx->speed);

    if(hotkeys_released(CONTROL_HOTKEY_RESET))
        core_restart(ctx);

    ctx->pause ^= hotkeys_released(CONTROL_HOTKEY_PAUSE);

    if(!ctx->pause){
        for(int i = 0; i < ctx->speed; i++){
            core->run_frame(ctx->emu);
        }
    }
}

void core_ctx_close(core_ctx_t* ctx){
    if(ctx->emu){
        if(ctx->core->close){
            char sav_path[FILENAME_MAX];
            path_set_ext(archive_get_path(ctx->rom), sav_path, "sav");
            ctx->core->close(ctx->emu, sav_path);
        }
        free(ctx->emu);
    }

    archive_free(ctx->rom);
    archive_free(ctx->bios);
}

void core_restart(core_ctx_t* ctx){
    const core_t* core = ctx->core;
    size(core->width, core->height);
    frameRate(core->fps);

    SDL_AudioSpec audio_spec = core->audio_spec;

    if (ctx->emu){
        if(core->close){
            char sav_path[FILENAME_MAX];
            path_set_ext(archive_get_path(ctx->rom), sav_path, "sav");
            core->close(ctx->emu, sav_path);
        }
        free(ctx->emu);
    }
    ctx->emu = core->init(ctx->rom, ctx->bios);

    sound_open(&audio_spec, core->sound_callback, ctx->emu);
    if(!sound_is_push_rate_set())
        sound_set_push_rate(core->sound_push_rate);

    controls_init(core->control_begin, core->control_end);
}