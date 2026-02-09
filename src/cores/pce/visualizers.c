#include "cores/pce/pce.h"

bool pce_palette_ram(pce_t* pce){
    size(32, 16);
    
    vce_t* vce = &pce->vce;
    for(int y = 0; y < 16; y++){
        for(int x = 0; x < 32; x++){
            int pal_idx = y * 32 + x;
            pal_idx <<= 1;
            u16 rgb333 = vce->cram[pal_idx] | (vce->cram[pal_idx | 1] << 8);
            pixels[x + y * 32] = pce_vce_convert_color(rgb333);
        }
    }
    return true;
}

static void vdc_get_tile_rgb(vdc_t* v, u16 tile_idx, u8 pal_idx, int* tile_rgb){
    for(int y = 0; y < 8; y++){
        for(int x = 0; x < 8; x++){
            u8 col = pce_vdc_get_tile_col_idx(v, tile_idx, x, y);
            tile_rgb[y * 8 + x] = pce_vce_get_pal_col(v->vce, pal_idx, col);
        }
    }
}

static void get_sprite_rgb(vdc_t* v, u16 addr, u8 xs, u8 ys, bool xf, bool yf, u8 pal, int* sprite_rgb){
    for(int y = 0; y < ys; y++){
        for(int x = 0; x < xs; x++){
            u8 col_idx = pce_vdc_get_sprite_col_idx(v, addr, xs, ys, xf, yf, x, y);   
            sprite_rgb[x + y * xs] = pce_vce_get_pal_col(v->vce, pal | 16, col_idx);
        }
    }
}

bool pce_vdc_draw_sprites(pce_t* pce){
    size(8*32, 8*64);

    vdc_t* v = &pce->vdc;
    u16* sat = (u16*)(&v->satb); 

    for(int y = 0; y < 8; y++){
        for(int x = 0; x < 8; x++){
            int idx = x + y * 8;
            u16 addr = sat[idx];
            u8 xs, ys, pal;
            bool xf, yf, f;
            int xpos, ypos;
            pce_vdc_get_sprite_info(v, idx, &addr, &xpos, &ypos, &xs, &ys, &xf, &yf, &f, &pal);
            int sprite_rgb[32*64];
            get_sprite_rgb(v, addr, xs, ys, xf, yf, pal, sprite_rgb);
            for(int py = 0; py < ys; py++){
                for(int px = 0; px < xs; px++){
                    pixels[(x*32+px) + (y*64+py) * width] = sprite_rgb[px + py * xs];
                }
            }
        }
    }

    return true;
}

bool pce_vdc_draw_tilemap(pce_t* pce){
    vdc_t* v = &pce->vdc;

    u8 w, h;
    pce_vdc_get_tilemap_size(v, &w, &h);
    size(w*8, h*8);

    for(int ty = 0; ty < h; ty++){
        for(int tx = 0; tx < w; tx++){
            u16 tile_attr_addr = (tx + ty * w) << 1;
            u16 tile_attr = v->vram[tile_attr_addr] | (v->vram[tile_attr_addr | 1] << 8);
            u16 tile_idx = tile_attr & ((1 << 12) - 1);
            u8 pal_idx = tile_attr >> 12;
            int tile_rgb[64];
            vdc_get_tile_rgb(v, tile_idx, pal_idx, tile_rgb);
            for(int py = 0; py < 8; py++){
                for(int px = 0; px < 8; px++){
                    pixels[(tx*8+px) + (ty*8+py) * width] = tile_rgb[px + py * 8];
                }
            }
        }
    }

    return true;
}