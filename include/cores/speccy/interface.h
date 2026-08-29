#ifndef __SPECCY_INTERFACE_H__
#define __SPECCY_INTERFACE_H__

void SPECCY_run_frame(void* ctx);
void* SPECCY_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool SPECCY_detect(const archive_t* rom_archive, const archive_t* bios_archive);
byte_vec_t SPECCY_savestate(void* ctx);
bool SPECCY_loadstate(void* ctx, byte_vec_t* state);
#define SPECCY_sound_callback NULL
#define SPECCY_free NULL
#define SPECCY_save NULL

#define SPECCY_WIDTH 356
#define SPECCY_HEIGHT 296
#define SPECCY_FPS 50.08
#define SPECCY_SOUND_PUSH_RATE (3.5e6/44100.0f)
#define SPECCY_has_bios false

#define SPECCY_AUDIO_SPEC \
{ \
    .channels = 1, \
    .format = SDL_AUDIO_S16, \
    .freq = 44100, \
} \

#define SPECCY_sound_channels \
{ "AY CH0", 0, 8191 }, \
{ "AY CH1", 0, 8191 }, \
{ "AY CH2", 0, 8191 }, \
{ "BEEPER", 0,    1 }

#include "cores/speccy/visualizers.h"

#define SPECCY_widgets \
{ "TAPE STATE", speccy_draw_tape_state }

#endif