#include "SDL_MAINLOOP.h"
#include "peripherals/sound.h"
#include "peripherals/controls.h"
#include "peripherals/camera.h"
#include "peripherals/argument.h"

#include "core.h"

static core_ctx_t* emu_ctx;

void setup(){
    setTitle("MULTI-SYSTEM EMU");
    controls_load_maps();

    const char* rom_path;
    const char* bios_path;
    const char* force_core;
    argument_get(&rom_path, &bios_path, &force_core);
    
    printf("ROM: %s\n", rom_path);
    printf("BIOS: %s\n", bios_path);

    emu_ctx = malloc(sizeof(core_ctx_t));
    core_ctx_init(emu_ctx, rom_path, bios_path, force_core);

    core_restart(emu_ctx);

    setWindowSize(512, 512);
}

void loop(){
    controls_update();
    camera_update();

    const bool* ks = SDL_GetKeyboardState(NULL);
    if(ks[SDL_SCANCODE_0])
        core_restart(emu_ctx);

    core_ctx_run_frame(emu_ctx);
}