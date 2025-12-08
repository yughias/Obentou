#include "cores/pce/vce.h"

#include "SDL_MAINLOOP.h"

#include <string.h>

static int vce_convert_color(u16 rgb333) {
    u8 g = (rgb333 >> 6) & 0b111;
    u8 r = (rgb333 >> 3) & 0b111;
    u8 b = rgb333 & 0b111;
    r = (r << 5) | (r << 2) | (r & 0b11);
    g = (g << 5) | (g << 2) | (g & 0b11);
    b = (b << 5) | (b << 2) | (b & 0b11); 
    return color(r, g, b);
}

static u16 inline get_pal_idx(const vce_t* v) {
    return v->lo_pal | ((v->hi_pal & 1) << 8);
}

int pce_vce_get_pal_col(const vce_t* v, u16 pal_idx, u8 col_idx){
    if(!col_idx)
        pal_idx = 0;
    u16 pal_addr = pal_idx << 5;
    u16 col_addr = pal_addr | (col_idx << 1);
    u16 col = v->cram[col_addr] | (v->cram[col_addr | 1] << 8);
    return vce_convert_color(col);
}

int pce_vce_get_overscan_col(const vce_t* v){
    u16 pal_addr = 0x10 << 5;
    u16 col_addr = pal_addr;
    u16 col = v->cram[col_addr] | (v->cram[col_addr | 1] << 8);
    return vce_convert_color(col);    
}

static void inline vce_increment_pal(vce_t* v) {
    u16 pal_idx = get_pal_idx(v) + 1;
    pal_idx &= 0x1FF;
    v->lo_pal = pal_idx & 0xFF;
    v->hi_pal = pal_idx >> 8;
}

void pce_vce_set_col_lo(vce_t* v, u8 value) {
    u16 pal_idx = get_pal_idx(v) << 1;
    v->cram[pal_idx] = value;
}

void pce_vce_set_col_hi(vce_t* v, u8 value) {
    u16 pal_idx = get_pal_idx(v) << 1;
    v->cram[pal_idx | 1] = value & 1;
    vce_increment_pal(v);
}

u8 pce_vce_get_col_lo(vce_t* v) {
    u16 pal_idx = get_pal_idx(v) << 1;
    return v->cram[pal_idx];
}

u8 pce_vce_get_col_hi(vce_t* v) {
    u16 pal_idx = get_pal_idx(v) << 1;
    u8 out = v->cram[pal_idx | 1];
    vce_increment_pal(v);
    return out | 0xFE;
}

void pce_vce_draw_palette(SDL_Window** win, vce_t* vce){
    Uint32 id = SDL_GetWindowID(*win);
    if(!id){
        *win = NULL;
        return;
    }
    SDL_Surface* s = SDL_GetWindowSurface(*win);
    for(int y = 0; y < 16; y++){
        for(int x = 0; x < 32; x++){
            int pal_idx = y * 32 + x;
            pal_idx <<= 1;
            u16 rgb333 = vce->cram[pal_idx] | (vce->cram[pal_idx | 1] << 8);
            SDL_Rect r = {x * 8, y * 8, 8, 8};
            SDL_FillSurfaceRect(s, &r, vce_convert_color(rgb333));
        }
    }
    SDL_UpdateWindowSurface(*win);
}