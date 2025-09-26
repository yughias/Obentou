#ifndef __BYTEPUSHER_INTERFACE_H__
#define __BYTEPUSHER_INTERFACE_H__

void BYTEPUSHER_run_frame(void* ctx);
void* BYTEPUSHER_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool BYTEPUSHER_detect(const archive_t* rom_archive, const archive_t* bios_archive);
#define BYTEPUSHER_close NULL

#define BYTEPUSHER_WIDTH 256
#define BYTEPUSHER_HEIGHT 256
#define BYTEPUSHER_FPS 60
#define BYTEPUSHER_SOUND_PUSH_RATE -1
#define BYTEPUSHER_sound_callback NULL

#define BYTEPUSHER_AUDIO_SPEC \
{ \
    .channels = 1, \
    .format = SDL_AUDIO_S8, \
    .freq = 15360, \
} \

#endif