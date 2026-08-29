#ifndef __GAMATE_INTERFACE_H__
#define __GAMATE_INTERFACE_H__

void GAMATE_run_frame(void* ctx);
void* GAMATE_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool GAMATE_detect(const archive_t* rom_archive, const archive_t* bios_archive);
byte_vec_t GAMATE_savestate(void* ctx);
bool GAMATE_loadstate(void* ctx, byte_vec_t* state);
#define GAMATE_free NULL
#define GAMATE_save NULL

#define GAMATE_WIDTH 160
#define GAMATE_HEIGHT 150
#define GAMATE_FPS 60.8093
#define GAMATE_SOUND_PUSH_RATE -1
#define GAMATE_sound_callback NULL
#define GAMATE_has_bios true

#define GAMATE_AUDIO_SPEC \
{ \
    .channels = 2, \
    .format = SDL_AUDIO_S16, \
    .freq = 44100, \
} \

#define GAMATE_sound_channels

#include "cores/gamate/visualizers.h"

#define GAMATE_widgets \
{ "Pixelmap", gamate_draw_pixelmap }

#endif