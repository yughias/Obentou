#include "core.h"

#include "utils/sound.h"
#include "utils/controls.h"
#include "utils/menu.h"
#include "utils/argument.h"
#include "utils/state.h"
#include "utils/rewind.h"

#include "SDL_MAINLOOP.h"

#include <stdio.h>
#include <stdlib.h>

static void save_sav(core_ctx_t* ctx){
    char sav_path[FILENAME_MAX];
    path_set_ext(archive_get_path(ctx->rom), sav_path, "sav");
    ctx->core->close(ctx->emu, sav_path);
}

static void core_close_emu(core_ctx_t* ctx){
    if(ctx->emu){
        if(ctx->core->savestate && state_get_autosave())
            state_save_autosave(ctx);
        if(ctx->core->close)
            save_sav(ctx);
        free(ctx->emu);
    }
}

const core_t* core_detect(const archive_t* rom_archive, const archive_t* bios_archive, const char* force_core){
    for(int i = 0; i < sizeof(cores)/sizeof(core_t); i++){
        if(force_core && !strcmp(cores[i].name, force_core)){
            return &cores[i];
        }
    }

    for(int i = 0; i < sizeof(cores)/sizeof(core_t); i++){
        if(cores[i].detect(rom_archive, bios_archive)){
            printf("Detected core: %s\n", cores[i].name);
            return &cores[i];
        }
    }

    printf("(ROM): %s \n(BIOS): %s\nUnknown core!\n", archive_get_path(rom_archive), archive_get_path(bios_archive));
    return NULL;
}

void core_ctx_init(core_ctx_t* ctx, const char* rom_path, const char* bios_path, const char* force_core){
    ctx->emu = NULL;
    ctx->rom = archive_load(rom_path);
    ctx->bios = archive_load(bios_path);
    ctx->core = core_detect(ctx->rom, ctx->bios, force_core);

    if(ctx->core){
        if(!bios_path || !bios_path[0]){
            char default_bios_path[FILENAME_MAX];
            argument_get_default_bios(default_bios_path, ctx->core->name);
            archive_free(ctx->bios);
            ctx->bios = archive_load(default_bios_path);
        }
        argument_update_recents(archive_get_path(ctx->rom), archive_get_path(ctx->bios));
    }
}

void core_ctx_set_speed(ctx_args_t* args){
    args->ctx->speed_level = args->value;
    menu_speed_check(args->value);
}

void core_ctx_run_frame(core_ctx_t* ctx){
    const core_t* core = ctx->core;

    if(!core){
        background(0);
        renderPixels();
        return;
    }

    if(!ctx->pause){
        int speed = 1 << (hotkeys_pressed(CONTROL_HOTKEY_TURBO) ? 3 : ctx->speed_level);
        sound_set_push_rate_multiplier(speed);
        sound_pause(false);
        for(int i = 0; i < speed; i++){
            core->run_frame(ctx->emu);
        }
    }
}

void core_switch_pause(core_ctx_t* ctx){
    ctx->pause ^= 1;
    sound_pause(ctx->pause);
    menu_tick_pause(ctx->pause);
}

void core_ctx_close(core_ctx_t* ctx){
    sound_close();

    core_close_emu(ctx);

    archive_free(ctx->rom);
    archive_free(ctx->bios);
}

void core_restart(core_ctx_t* ctx){
    const core_t* core = ctx->core;

    menu_create(ctx);

    if(!core){
        sound_open(NULL, NULL, NULL);
        ctx->emu = NULL;
        size(1, 1);
        frameRate(60);
        return;
    }
    size(core->width, core->height);
    frameRate(core->fps);

    SDL_AudioSpec audio_spec = core->audio_spec;

    bool first_loading = !ctx->emu;

    core_close_emu(ctx);

    ctx->emu = core->init(ctx->rom, ctx->bios);
    float push_rate = sound_get_push_rate();

    sound_open(&audio_spec, core->sound_callback, ctx->emu);
    sound_set_push_rate(core->sound_push_rate < 0 ? push_rate : core->sound_push_rate);

    controls_init(core->control_begin, core->control_end);

    if(first_loading && core->loadstate && state_get_autosave()){
        state_load_autosave(ctx);
    }

    rewind_init();
}