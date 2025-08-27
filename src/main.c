#include "SDL_MAINLOOP.h"

#include "cores.h"
#include "cores/watara/interface.h"
#include "cores/pv1000/interface.h"

core_t cores[] = {
    {
        .init = watara_init,
        .run_frame = watara_run_frame,
        .width = watara_width,
        .height = watara_height,
        .fps = watara_fps,
        .audio_spec = watara_audio_spec
    },
    {
        .init = pv1000_init,
        .run_frame = pv1000_run_frame,
        .width = pv1000_width,
        .height = pv1000_height,
        .fps = pv1000_fps,
        .audio_spec = pv1000_audio_spec
    }
};

void* emu;

void setup(){
    core_t* core = &cores[1];
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
    core_t* core = &cores[1];
    core->run_frame(emu);
}