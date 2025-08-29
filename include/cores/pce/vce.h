#ifndef __VCE_H__
#define __VCE_H__

#include "types.h"

#include "SDL_MAINLOOP.h"

typedef struct vce_t {
    u8 cram[0x400];

    u8 lo_pal;
    u8 hi_pal;

    u8 ctrl;
} vce_t;

void pce_vce_set_col_lo(vce_t* v, u8 value);
void pce_vce_set_col_hi(vce_t* v, u8 value);
u8 pce_vce_get_col_lo(vce_t* v);
u8 pce_vce_get_col_hi(vce_t* v);
int pce_vce_get_pal_col(const vce_t* v, u16 pal_idx, u8 col_idx);
int pce_vce_get_overscan_col(const vce_t* v);
void pce_vce_draw_palette(SDL_Window** win, vce_t* vce);

#endif