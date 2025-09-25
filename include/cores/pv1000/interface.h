#ifndef __PV1000_INTERFACE_H__
#define __PV1000_INTERFACE_H__

#include "interface.h"

void PV1000_run_frame(void* ctx);
void* PV1000_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool PV1000_detect(const archive_t* rom_archive, const archive_t* bios_archive);
void PV1000_sound_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount);

#define PV1000_WIDTH 224
#define PV1000_HEIGHT 192
#define PV1000_FPS 59.9227434033
#define PV1000_SOUND_PUSH_RATE -1

#define PV1000_AUDIO_SPEC \
{ \
    .channels = 1, \
    .format = SDL_AUDIO_S8, \
    .freq = 44100, \
} \

#endif