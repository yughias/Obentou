#ifndef __PACMAN_VIDEO_H__
#define __PACMAN_VIDEO_H__

#include "cores/pacman/pacman.h"

void pacman_draw_video(pacman_t* p);
void pacman_get_palette(pacman_t* p, u8 idx, int* pal);
int pacman_color_from_rom(pacman_t* p, u8 idx);

#endif
