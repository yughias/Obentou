#ifndef __JACE_INTERFACE_H__
#define __JACE_INTERFACE_H__

void JACE_run_frame(void* ctx);
void* JACE_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool JACE_detect(const archive_t* rom_archive, const archive_t* bios_archive);
void JACE_close(void* ctx, const char* sav_path);
byte_vec_t JACE_savestate(void* ctx);
bool JACE_loadstate(void* ctx, byte_vec_t* state);
#define JACE_sound_callback NULL

#define JACE_WIDTH 336
#define JACE_HEIGHT 304
#define JACE_FPS 50.00
#define JACE_SOUND_PUSH_RATE (3.25e6/44100.0f)
#define JACE_has_bios false

#define JACE_AUDIO_SPEC \
{ \
    .channels = 1, \
    .format = SDL_AUDIO_S8, \
    .freq = 44100, \
} \

#define JACE_sound_channels \
{ "BEEPER", 0, 64 }

#include "cores/jace/visualizers.h"

#define JACE_widgets \
{ "TAPE STATE", jace_draw_char_ram }

#endif