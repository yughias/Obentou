#ifndef __GBC_INTERFACE_H__
#define __GBC_INTERFACE_H__

void GBC_run_frame(void* ctx);
void* GBC_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool GBC_detect(const archive_t* rom_archive, const archive_t* bios_archive);
byte_vec_t GBC_savestate(void* ctx);
bool GBC_loadstate(void* ctx, byte_vec_t* state);
void GBC_close(void* ctx, const char* sav_path);

#define GBC_WIDTH 160
#define GBC_HEIGHT 144
#define GBC_FPS 59.727500569606
#define GBC_SOUND_PUSH_RATE (4194304.0f/44100.0f)
#define GBC_sound_callback NULL
#define GBC_has_bios true

#define GBC_AUDIO_SPEC \
{ \
    .channels = 2, \
    .format = SDL_AUDIO_S16, \
    .freq = 44100, \
} \

#define GBC_sound_channels \
{ "SQUARE+SWEEP", 0, 15}, \
{ "SQUARE", 0 , 15}, \
{ "WAVE RAM", 0, 15}, \
{ "NOISE", 0, 15} \

#endif