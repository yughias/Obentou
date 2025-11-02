#ifndef __TMS80_INTERFACE_H__
#define __TMS80_INTERFACE_H__

void TMS80_run_frame(void* ctx);
void* TMS80_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool TMS80_detect(const archive_t* rom_archive, const archive_t* bios_archive);
byte_vec_t TMS80_savestate(void* ctx);
void TMS80_loadstate(void* ctx, byte_vec_t* state);
void TMS80_close(void* ctx, const char* sav_path);

#define TMS80_WIDTH 256
#define TMS80_HEIGHT 192
#define TMS80_FPS 59.9227434033
#define TMS80_SOUND_PUSH_RATE (3579545.0f/44100.0f)
#define TMS80_sound_callback NULL
#define TMS80_has_bios true

#define TMS80_AUDIO_SPEC \
{ \
    .channels = 1, \
    .format = SDL_AUDIO_S16, \
    .freq = 44100, \
} \

#endif