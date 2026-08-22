#ifndef __GBA_INTERFACE_H__
#define __GBA_INTERFACE_H__

void GBA_run_frame(void* ctx);
void* GBA_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool GBA_detect(const archive_t* rom_archive, const archive_t* bios_archive);
byte_vec_t GBA_savestate(void* ctx);
bool GBA_loadstate(void* ctx, byte_vec_t* state);
void GBA_free(void* ctx);
void GBA_save(void* ctx, const char* sav_path);

#define GBA_WIDTH 240
#define GBA_HEIGHT 160
#define GBA_FPS 59.72750057
#define GBA_SOUND_PUSH_RATE (16777216.0f/44100.0f)
#define GBA_sound_callback NULL
#define GBA_has_bios true

#define GBA_AUDIO_SPEC \
{ \
    .channels = 2, \
    .format = SDL_AUDIO_S16, \
    .freq = 44100, \
} \

#define GBA_sound_channels \
{ "SQUARE+SWEEP", -15, +15}, \
{ "SQUARE", -15, +15}, \
{ "WAVE RAM", -15, +15}, \
{ "NOISE", -15, +15}, \
{ "DMA1", -512, +512}, \
{ "DMA2", -512, +512}

#define GBA_widgets 

#endif