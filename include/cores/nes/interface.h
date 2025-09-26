#ifndef __NES_INTERFACE_H__
#define __NES_INTERFACE_H__

#include "interface.h"

void NES_run_frame(void* ctx);
void* NES_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool NES_detect(const archive_t* rom_archive, const archive_t* bios_archive);
void NES_close(void* ctx, const char* sav_path);

#define NES_WIDTH 256
#define NES_HEIGHT 240
#define NES_FPS 60.0988
#define NES_SOUND_PUSH_RATE (1.789773e6/2.0/44100.0f)
#define NES_sound_callback NULL

#define NES_AUDIO_SPEC \
{ \
    .channels = 1, \
    .format = SDL_AUDIO_F32, \
    .freq = 44100, \
} \

#endif