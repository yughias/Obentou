#ifndef __PACMAN_VIDEO_H__
#define __PACMAN_VIDEO_H__

#include "cores/pacman/pacman.h"

void pacman_draw_video(pacman_t* p);
void pacman_draw_tile(int ox, int oy, int* tile);
void pacman_get_tile(pacman_t* p, u8 tile_idx, u8 pal_idx, int* out);
void pacman_get_palette(pacman_t* p, u8 idx, int* pal);

#endif
