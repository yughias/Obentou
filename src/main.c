#include "SDL_MAINLOOP.h"
#include "peripherals/sound.h"
#include "peripherals/controls.h"
#include "peripherals/camera.h"
#include "peripherals/argument.h"

#include "core.h"

static core_ctx_t* emu_ctx;

static void obentou_exit(){
    sound_close();
    camera_close();
    controls_free();
    core_ctx_close(emu_ctx);
    free(emu_ctx);
}

void setup(){
    setTitle("MULTI-SYSTEM EMU");
    onExit = obentou_exit;
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

    core_ctx_run_frame(emu_ctx);
}