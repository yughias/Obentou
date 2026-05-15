#ifndef __PACMAN_VISUALIZERS_H__
#define __PACMAN_VISUALIZERS_H__

#include "types.h"

bool pacman_draw_tile_rom(void* ctx);
bool pacman_draw_sprite_rom(void* ctx);
bool pacman_draw_audiorom(void* ctx);
bool pacman_draw_sprites(void* ctx);
bool pacman_draw_palettes(void* ctx);
bool pacman_draw_colors(void* ctx);

#endif
