#include "SDL_MAINLOOP.h"
#include "utils/sound.h"
#include "utils/controls.h"
#include "utils/camera.h"
#include "utils/argument.h"
#include "utils/menu.h"
#include "utils/state.h"
#include "utils/rewind.h"

#include "core.h"

static core_ctx_t emu_ctx;

static void obentou_exit(){
    sound_close();
    camera_close();
    controls_free();
    core_ctx_close(&emu_ctx);
    controls_save_maps();
    rewind_clear();
}

void setup(){
    setTitle("Obentou");
    onExit = obentou_exit;
    controls_load_maps();

    const char* rom_path;
    const char* bios_path;
    const char* force_core;
    argument_get(&rom_path, &bios_path, &force_core);

    core_ctx_init(&emu_ctx, rom_path, bios_path, force_core);

    core_restart(&emu_ctx);

    setWindowSize(512, 512);
}

void loop(){
    controls_update();
    camera_update();

    if(hotkeys_released(CONTROL_HOTKEY_SAVESTATE)){
        state_save_slot(&emu_ctx);
    }

    if(hotkeys_released(CONTROL_HOTKEY_LOADSTATE)){
        state_load_slot(&emu_ctx);
    }

    if(controls_double_click()){
        menu_fullscreen();
    }

    if(hotkeys_released(CONTROL_HOTKEY_OPEN))
        menu_open_rom(&emu_ctx);

    if(hotkeys_released(CONTROL_HOTKEY_OPEN_BIOS))
        menu_open_bios(&emu_ctx);

    if(hotkeys_released(CONTROL_HOTKEY_SPEEDUP) && emu_ctx.speed_level < 3){
        ctx_args_t speed_args = {.ctx = &emu_ctx, .value = emu_ctx.speed_level + 1};
        core_ctx_set_speed(&speed_args);
    }

    if(hotkeys_released(CONTROL_HOTKEY_SLOWDOWN) && emu_ctx.speed_level != 0){
        ctx_args_t speed_args = {.ctx = &emu_ctx, .value = emu_ctx.speed_level - 1};
        core_ctx_set_speed(&speed_args);
    }

    if(hotkeys_released(CONTROL_HOTKEY_RESET))
        core_restart(&emu_ctx);

    if(hotkeys_released(CONTROL_HOTKEY_PAUSE))
        core_switch_pause(&emu_ctx);

     if(hotkeys_pressed(CONTROL_HOTKEY_REWIND)){
        byte_vec_t* prev = rewind_recover_state();
        if(prev){
            emu_ctx.core->loadstate(emu_ctx.emu, prev); 
            sound_pause(true);
            while(!hasRendered())
                emu_ctx.core->run_frame(emu_ctx.emu);
            sound_dequeue();
        }
    } else {
        if (emu_ctx.core && emu_ctx.core->savestate) {
            byte_vec_t state = emu_ctx.core->savestate(emu_ctx.emu);
            rewind_add_state(&state);
        }
        core_ctx_run_frame(&emu_ctx);
    }

}