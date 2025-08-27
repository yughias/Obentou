#include "SDL_MAINLOOP.h"

#include "cores.h"
#include "cores/watara/interface.h"
#include "cores/pv1000/interface.h"

const core_t cores[] = {
    LOAD_CORE(watara),
    LOAD_CORE(pv1000),
};

void* emu;
const core_t* core = NULL;

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

void setup(){
    detect_core(getArgv(1));

    setScaleMode(NEAREST);
    size(core->width, core->height);
    frameRate(core->fps);

    SDL_AudioSpec audio_spec = core->audio_spec;
    SDL_AudioDeviceID audio_dev = 0;

    if(audio_spec.callback){
        emu = core->init(getArgv(1), 0);
        audio_spec.userdata = emu;
        audio_dev = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);
    }  else {
        audio_dev = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);
        emu = core->init(getArgv(1), audio_dev);
    }

    SDL_PauseAudioDevice(audio_dev, 0);
}

void loop(){
    core->run_frame(emu);
}