#ifndef __NES_INTERFACE_H__
#define __NES_INTERFACE_H__

void NES_run_frame(void* ctx);
void* NES_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool NES_detect(const archive_t* rom_archive, const archive_t* bios_archive);
byte_vec_t NES_savestate(void* ctx);
bool NES_loadstate(void* ctx, byte_vec_t* state);
void NES_close(void* ctx, const char* sav_path);

#define NES_WIDTH 256
#define NES_HEIGHT 240
#define NES_FPS 60.0988
#define NES_SOUND_PUSH_RATE (1.789773e6/2/44100)
#define NES_sound_callback NULL
#define NES_has_bios false

#define NES_AUDIO_SPEC \
{ \
    .channels = 1, \
    .format = SDL_AUDIO_F32, \
    .freq = 44100, \
} \

#define NES_sound_channels \
{ "SQUARE 1", 0, 15 }, \
{ "SQUARE 2", 0, 15 }, \
{ "TRIANGLE", 0, 15 }, \
{ "NOISE", 0, 15 }, \
{ "DMC", 0, 127 }

#include "cores/nes/visualizers.h"

#define NES_widgets \
{ "Nametables", nes_ppu_draw_nametables }, \
{ "Tileset", nes_ppu_draw_tileset }, \
{ "OAM", nes_ppu_draw_oam }, \
{ "Palettes", nes_ppu_draw_palettes }

#endif