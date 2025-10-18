#ifndef __WATARA_INTERFACE_H__
#define __WATARA_INTERFACE_H__

#include "types.h"

void WATARA_run_frame(void* ctx);
bool WATARA_detect(const archive_t* rom_archive, const archive_t* bios_archive);
void* WATARA_init(const archive_t* rom_archive, const archive_t* bios_archive);
#define WATARA_close NULL

#define WATARA_WIDTH 160
#define WATARA_HEIGHT 160
#define WATARA_FPS 61.04
#define WATARA_SOUND_PUSH_RATE (4e6/44100.0f)
#define WATARA_sound_callback NULL
#define WATARA_has_bios false

#define WATARA_AUDIO_SPEC \
{ \
    .channels = 2, \
    .format = SDL_AUDIO_S8, \
    .freq = 44100, \
} \

#endif