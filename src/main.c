#include "SDL_MAINLOOP.h"
#include "cores.h"
#include "cores/watara/interface.h"

core_t cores[] = {
    {
        .init = watara_init,
        .run_frame = watara_run_frame,
        .width = watara_width,
        .height = watara_height,
        .fps = watara_fps,
        .audio_spec = watara_audio_spec
    }
};

void* emu;

void setup(){
    core_t* core = &cores[0];
    setScaleMode(NEAREST);
    size(core->width, core->height);
    frameRate(core->fps);

    SDL_AudioDeviceID audio_dev = SDL_OpenAudioDevice(NULL, 0, &core->audio_spec, NULL, 0);

    emu = core->init(getArgv(1), audio_dev);

    SDL_PauseAudioDevice(audio_dev, 0);
}

void loop(){
    core_t* core = &cores[0];
    core->run_frame(emu);
}