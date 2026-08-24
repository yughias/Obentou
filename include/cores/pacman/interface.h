#ifndef __PACMAN_INTERFACE_H__
#define __PACMAN_INTERFACE_H__

void PACMAN_run_frame(void* ctx);
void*PACMAN_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool PACMAN_detect(const archive_t* rom_archive, const archive_t* bios_archive);
void PACMAN_free(void* ctx);
byte_vec_t PACMAN_savestate(void* ctx);
bool PACMAN_loadstate(void* ctx, byte_vec_t* state);
#define PACMAN_save NULL

#define PACMAN_WIDTH  224  /* 28 tiles × 8 px */
#define PACMAN_HEIGHT 288  /* 36 tiles × 8 px */
#define PACMAN_FPS    60.0f


#define PACMAN_SOUND_PUSH_RATE (3072000.0f/44100.0f)

#define PACMAN_sound_callback NULL
#define PACMAN_has_bios       false

#define PACMAN_AUDIO_SPEC \
{ \
    .channels = 1, \
    .format = SDL_AUDIO_S16, \
    .freq = 44100, \
} \

#define PACMAN_sound_channels \
{ "VOICE1", 0, 225 }, \
{ "VOICE2", 0, 225 }, \
{ "VOICE3", 0, 225 }

#include "cores/pacman/visualizers.h"

#define PACMAN_widgets \
{ "Tile ROM",    pacman_draw_tile_rom   }, \
{ "Sprite ROM",  pacman_draw_sprite_rom }, \
{ "Palette ROM", pacman_draw_palettes   }, \
{ "Color ROM",   pacman_draw_colors     }, \
{ "Audio ROM",   pacman_draw_audiorom   }, \
{ "Sprites",     pacman_draw_sprites    }

#endif
