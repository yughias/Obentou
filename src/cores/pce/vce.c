#include "cores/pce/vce.h"

#include "SDL_MAINLOOP.h"

#include <string.h>

int pce_vce_convert_color(u16 rgb333) {
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
    return pce_vce_convert_color(col);
}

int pce_vce_get_overscan_col(const vce_t* v){
    u16 pal_addr = 0x10 << 5;
    u16 col_addr = pal_addr;
    u16 col = v->cram[col_addr] | (v->cram[col_addr | 1] << 8);
    return pce_vce_convert_color(col);    
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
