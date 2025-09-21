#include "SDL_MAINLOOP.h"
#include "peripherals/sound.h"
#include "peripherals/controls.h"
#include "peripherals/camera.h"

#include "cores.h"

static void* emu;
static const core_t* core = NULL;

static void detect_core(const char* filename){
    for(int i = 0; i < sizeof(cores)/sizeof(core_t); i++){
        if(cores[i].detect(filename)){
            core = &cores[i];
            return;
        }
    }

    fprintf(stderr, "Unknown core: %s\n", filename);
    exit(EXIT_FAILURE);
}

void foo(){
    printf("AH\n");
}

void setup(){
    if(getArgc() < 2){
        fprintf(stderr, "Usage: %s <rom>\n", getArgv(0));
        exit(EXIT_FAILURE);
    }
    
    detect_core(getArgv(1));

    size(core->width, core->height);
    frameRate(core->fps);

    SDL_AudioSpec audio_spec = core->audio_spec;

    emu = core->init(getArgv(1));

    sound_open(&audio_spec, core->sound_callback, emu);
    if(!sound_is_push_rate_set())
        sound_set_push_rate(core->sound_push_rate);

    controls_init(core->control_begin, core->control_end);

    setWindowSize(512, 512);
}

void loop(){
    controls_update();
    camera_update();

    core->run_frame(emu);
}