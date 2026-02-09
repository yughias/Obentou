#include "cores/nes/nes.h"

static void copy_tile_to_window(int* pixels, int* tile, int x, int y, int w){
    for(int ty = 0; ty < 8; ty++)
        for(int tx = 0; tx < 8; tx++)
            pixels[(x+tx) + (y+ty) * w] = tile[tx + ty * 8];
}

static void tile_flip_x(int* tile){
    for(int y = 0; y < 8; y++){
        for(int x = 0; x < 4; x++){
            int tmp = tile[x+y*8];
            tile[x+y*8] = tile[(7-x)+y*8];
            tile[(7-x)+y*8] = tmp;
        }
    }
}

static void tile_flip_y(int* tile){
    for(int y = 0; y < 4; y++){
        for(int x = 0; x < 8; x++){
            int tmp = tile[x+y*8];
            tile[x+y*8] = tile[x+(7-y)*8];
            tile[x+(7-y)*8] = tmp;
        }
    }
}

bool nes_ppu_draw_tileset(nes_t* nes){
    size(32*8, 16*8);

    for(int y = 0; y < 16; y++){
        for(int x = 0; x < 32; x++){
            int tile[64];
            nes_ppu_get_grayscale_tile(&nes->ppu, 0, x + y * 32, tile);
            copy_tile_to_window(pixels, tile, x*8, y*8, width);
        }
    }

    return true;
}

bool nes_ppu_draw_palettes(nes_t* nes){
    size(4, 8);

    ppu_t* ppu = &nes->ppu;

    for(int p = 0; p < 8; p++){
        for(int c = 0; c < 4; c++){
            u8 nes_col = nes_ppu_get_palette_color(ppu, p, c);
            int col = nes_ppu_convert_to_rgb(nes_col);
            pixels[c + p * stride] = col;
        }
    }

    return true;
}

static void nes_ppu_get_tile(ppu_t* ppu, u16 base, u8 palette, u8 idx, int tile[64]){
    for(int py = 0; py < 8; py++){
        u8 p0 = ppu->read(ppu, base + idx*16+py);
        u8 p1 = ppu->read(ppu, base + idx*16+8+py);
        for(int px = 0; px < 8; px++){
            u8 b0 = !!(p0 & (1 << (7 - px)));
            u8 b1 = !!(p1 & (1 << (7 - px)));
            u8 col = b0 | (b1 << 1);
            u8 nes_col = nes_ppu_get_palette_color(ppu, palette, col);
            tile[px + py * 8] = nes_ppu_convert_to_rgb(nes_col);
        }
    }
}


bool nes_ppu_draw_nametables(nes_t* nes){
    size(32*8*2, 32*8*2);

    ppu_t* ppu = &nes->ppu;
    u16 pattern_base = ppu->ctrl & (1 << 4) ? 0x1000 : 0x0000; 

    for(int y = 0; y < 2; y++){
        for(int x = 0; x < 2; x++){
            u16 nt_addr = 0x2000 + (x + y*2)*0x400;
            u16 attr_addr = nt_addr + 0x3C0;
            for(int tx = 0; tx < 32; tx++){
                for(int ty = 0; ty < 30; ty++){
                    int tile[64];
                    u8 idx = ppu->read(ppu, nt_addr + tx + ty * 32);
                    u8 attr = ppu->read(ppu, attr_addr + (tx >> 2) + (ty >> 2) * 8);
                    u8 pal_idx = (bool)(tx & 0b10) + (bool)(ty & 0b10) * 2;
                    u8 palette = (attr >> (pal_idx*2)) & 0b11;
                    nes_ppu_get_tile(ppu, pattern_base, palette, idx, tile);
                    int draw_x = x*32*8 + tx*8;
                    int draw_y = y*30*8 + ty*8;
                    copy_tile_to_window(pixels, tile, draw_x, draw_y, width);
                }
            }
        }
    }

    int red = color(255, 0, 0);
    int scroll_x = ppu->x;
    scroll_x |= (ppu->t & 0x1F) << 3;
    if(ppu->t & (1 << 10))
        scroll_x += 256;
    int scroll_y = (ppu->t >> 12) & 0b111;
    scroll_y |= ((ppu->t >> 5) & 0x1F) << 3;
    if(ppu->t & (1 << 11))
        scroll_y += 240;
    for(int i = 0; i < SCREEN_WIDTH; i++){
        int x = (i + scroll_x) % width;
        int y = scroll_y % height;
        pixels[x + y * width] = red;
        pixels[x + ((y + SCREEN_HEIGHT) % height) * width] = red;
    }
    for(int i = 0; i < SCREEN_HEIGHT; i++){
        int x = scroll_x % width;
        int y = (i + scroll_y) % height;
        pixels[x + y * width] = red;
        pixels[((x + SCREEN_WIDTH) % width) + y * width] = red;
    }

    return true;
}

bool nes_ppu_draw_oam(nes_t* nes){
    size(8*32, 8*32);

    ppu_t* ppu = &nes->ppu;

    int magenta = color(255, 0, 255);
    int grey = color(100, 100, 100);

    for(int i = 0; i < width*height; i++)
        pixels[i] = magenta;

    for(int i = 0; i  < 8; i++){
        for(int j = 0; j < width; j++){
            pixels[j + i*32 * width] = grey;
            pixels[i*32 + j * width] = grey;
        }
    }

    for(int y = 0; y < 8; y++){
        for(int x = 0; x < 8; x++){
            int i = x + y * 8;
            sprite_t sp = {.idx = ppu->oam[1+4*i], .attr = ppu->oam[2+4*i]};
            bool flip_y = sp.attr & (1 << 7);
            bool flip_x = sp.attr & (1 << 6);
            u8 sprite_size = ppu->ctrl & (1 << 5) ? 16 : 8;
            u16 base_addr;
            u8 idx = sp.idx;
            if(sprite_size == 8){
                base_addr = ppu->ctrl & (1 << 3) ? 0x1000 : 0x0000;
            } else {
                base_addr = sp.idx & 1 ? 0x1000 : 0x0000;
                idx &= 0XFE;
            }
            int tile[64];
            nes_ppu_get_tile(ppu, base_addr, 0x4 | (sp.attr & 0b11), idx | flip_y, tile);
            if(flip_x) tile_flip_x(tile);
            if(flip_y) tile_flip_y(tile);
            copy_tile_to_window(pixels, tile, x*32+16-sprite_size/2, y*32+16-sprite_size/2, width);
            if(sprite_size == 16){
                nes_ppu_get_tile(ppu, base_addr, 0x4 | (sp.attr & 0b11), idx | !flip_y, tile);
                if(flip_x) tile_flip_x(tile);
                if(flip_y) tile_flip_y(tile);
                copy_tile_to_window(pixels, tile, x*32+16-sprite_size/2, y*32+16-sprite_size/2+8, width);
            }
        }
    }

    return true;
}