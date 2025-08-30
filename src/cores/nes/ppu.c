#include "cores/nes/ppu.h"
#include "cores/nes/nes.h"

#include "cores/nes/ppu_task.h"

static void copy_tile_to_window(int* pixels, int* tile, int x, int y, int w);
static void tile_flip_x(int* tile);
static void tile_flip_y(int* tile);

static const u8 flip_x_lut[] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

static const u8 ppu_colors[64][3]  = {
    {124, 124, 124},
    {0, 0, 252},
    {0, 0, 188},
    {68, 40, 188},
    {148, 0, 132},
    {168, 0, 32},
    {168, 16, 0},
    {136, 20, 0},
    {80, 48, 0},
    {0, 120, 0},
    {0, 104, 0},
    {0, 88, 0},
    {0, 64, 88},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {188, 188, 188},
    {0, 120, 248},
    {0, 88, 248},
    {104, 68, 252},
    {216, 0, 204},
    {228, 0, 88},
    {248, 56, 0},
    {228, 92, 16},
    {172, 124, 0},
    {0, 184, 0},
    {0, 168, 0},
    {0, 168, 68},
    {0, 136, 136},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {248, 248, 248},
    {60, 188, 252},
    {104, 136, 252},
    {152, 120, 248},
    {248, 120, 248},
    {248, 88, 152},
    {248, 120, 88},
    {252, 160, 68},
    {248, 184, 0},
    {184, 248, 24},
    {88, 216, 84},
    {88, 248, 152},
    {0, 232, 216},
    {120, 120, 120},
    {0, 0, 0},
    {0, 0, 0},
    {252, 252, 252},
    {164, 228, 252},
    {184, 184, 248},
    {216, 184, 248},
    {248, 184, 248},
    {248, 164, 192},
    {240, 208, 176},
    {252, 224, 168},
    {248, 216, 120},
    {216, 248, 120},
    {184, 248, 184},
    {184, 248, 216},
    {0, 252, 252},
    {248, 216, 248},
    {0, 0, 0},
    {0, 0, 0}
};

int nes_ppu_convert_to_rgb(u8 col){
    const u8* rgb = ppu_colors[col];
    return color(rgb[0], rgb[1], rgb[2]);
}

u16 nes_ppu_get_vram_addr(VRAM_ALIGN align, u16 addr, u16 vram_size){
    if(align == VRAM_V){
        bool v11 = addr & (1 << 11);
        addr = (addr & ~(1 << 10)) | (v11 << 10);
    } else if(align == VRAM_1_LO){
        addr &= ((1 << 10) - 1);
    }
    return addr & (vram_size - 1);
}

void nes_ppu_get_grayscale_tile(ppu_t* ppu, u16 base, u8 idx, int tile[64]){
    for(int ty = 0; ty < 8; ty++){
        u8 p0 = ppu->read(ppu, base + idx*16+ty);
        u8 p1 = ppu->read(ppu, base + idx*16+8+ty);
        for(int tx = 0; tx < 8; tx++){
            u8 b0 = !!(p0 & (1 << (7 - tx)));
            u8 b1 = !!(p1 & (1 << (7 - tx)));
            u8 col = b0 | (b1 << 1);
            int rgb;
            switch (col) {
                case 0b00:
                rgb = color(0, 0, 0);
                break;

                case 0b01:
                rgb = color(64, 64, 64);
                break;

                case 0b10:
                rgb = color(128, 128, 128);
                break;

                case 0b11:
                rgb = color(255, 255, 255);
                break;
            }
            tile[tx + ty * 8] = rgb;
        }
    }
}

void nes_ppu_get_tile(ppu_t* ppu, u16 base, u8 palette, u8 idx, int tile[64]){
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

void nes_ppu_draw_chr(ppu_t* ppu, SDL_Window** win){
    Uint32 id = SDL_GetWindowID(*win);
    if(!id){
        *win = NULL;
        return;
    }
    SDL_Surface* s = SDL_GetWindowSurface(*win);
    int* pixels = (int*)s->pixels;
    int w = s->w;
    int h = s->h;

    for(int y = 0; y < 16; y++){
        for(int x = 0; x < 32; x++){
            int tile[64];
            nes_ppu_get_grayscale_tile(ppu, 0, x + y * 32, tile);
            copy_tile_to_window(pixels, tile, x*8, y*8, s->w);
        }
    }

    SDL_UpdateWindowSurface(*win);
}


void nes_ppu_draw_nametables(ppu_t* ppu, SDL_Window** win){
    Uint32 id = SDL_GetWindowID(*win);
    if(!id){
        *win = NULL;
        return;
    }
    SDL_Surface* s = SDL_GetWindowSurface(*win);
    int* pixels = (int*)s->pixels;
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
                    copy_tile_to_window(pixels, tile, draw_x, draw_y, s->w);
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
        int x = (i + scroll_x) % s->w;
        int y = scroll_y % s->h;
        pixels[x + y * s->w] = red;
        pixels[x + ((y + SCREEN_HEIGHT) % s->h) * s->w] = red;
    }
    for(int i = 0; i < SCREEN_HEIGHT; i++){
        int x = scroll_x % s->w;
        int y = (i + scroll_y) % s->h;
        pixels[x + y * s->w] = red;
        pixels[((x + SCREEN_WIDTH) % s->w) + y * s->w] = red;
    }

    SDL_UpdateWindowSurface(*win);
}

void nes_ppu_draw_palettes(ppu_t* ppu, SDL_Window** win){
    Uint32 id = SDL_GetWindowID(*win);
    if(!id){
        *win = NULL;
        return;
    }
    SDL_Surface* s = SDL_GetWindowSurface(*win);
    int* pixels = (int*)s->pixels;

    for(int p = 0; p < 8; p++){
        for(int c = 0; c < 4; c++){
            u8 nes_col = nes_ppu_get_palette_color(ppu, p, c);
            int col = nes_ppu_convert_to_rgb(nes_col);
            for(int y = 0; y < 16; y++)
                for(int x = 0; x < 16; x++)
                    pixels[(c*16+x) + (p*16+y) * s->w] = col;
        }
    }

    SDL_UpdateWindowSurface(*win);
}

void nes_ppu_draw_oam(ppu_t* ppu, SDL_Window** win){
    Uint32 id = SDL_GetWindowID(*win);
    if(!id){
        *win = NULL;
        return;
    }
    SDL_Surface* s = SDL_GetWindowSurface(*win);
    int* pixels = (int*)s->pixels;

    int magenta = color(255, 0, 255);
    int grey = color(100, 100, 100);
    SDL_FillRect(s, NULL, magenta);

    for(int i = 0; i  < 8; i++){
        for(int j = 0; j < s->w; j++){
            pixels[j + i*32 * s->w] = grey;
            pixels[i*32 + j * s->w] = grey;
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
            copy_tile_to_window(pixels, tile, x*32+16-sprite_size/2, y*32+16-sprite_size/2, s->w);
            if(sprite_size == 16){
                nes_ppu_get_tile(ppu, base_addr, 0x4 | (sp.attr & 0b11), idx | !flip_y, tile);
                if(flip_x) tile_flip_x(tile);
                if(flip_y) tile_flip_y(tile);
                copy_tile_to_window(pixels, tile, x*32+16-sprite_size/2, y*32+16-sprite_size/2+8, s->w);
            }
        }
    }

    SDL_UpdateWindowSurface(*win);
}

u8 nes_ppu_get_palette_color(ppu_t* ppu, u8 palette, u8 idx){
    u8* pram = ppu->palette_ram;
    // backdrop color
    if(!idx)
        palette = 0;
    return pram[palette*4+idx] & 0x3F;
}

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

void nes_ppu_inc_addr(ppu_t* ppu){
    ppu->v += ppu->ctrl & (1 << 2) ? 32 : 1;
}

void nes_ppu_sync(ppu_t* ppu){
    for(int i = 0; i < 3; i++)
        nes_ppu_step(ppu);
}

void nes_ppu_step(ppu_t* ppu){
    u8 task_idx = ppu_tasks_per_cycle[ppu->cycles];
    ppu_task_t task = ppu_tasks_idx[task_idx];
    (*task)(ppu);
    ppu->cycles += 1;
}

void nes_ppu_inc_hori(ppu_t* ppu){
    if((ppu->v & 0x001F) == 31){
        ppu->v &= ~0x001F;
        ppu->v ^= 0x0400;
    } else
       ppu->v += 1;
}

void nes_ppu_inc_vert(ppu_t* ppu){
    if ((ppu->v & 0x7000) != 0x7000)
        ppu->v += 0x1000;        
    else {
        ppu->v &= ~0x7000;                 
        int y = (ppu->v & 0x03E0) >> 5;      
        if (y == 29) {
            y = 0;
            ppu->v ^= 0x0800;                  
        } else if (y == 31)
            y = 0;               
        else
            y += 1;                        
        ppu->v = (ppu->v & ~0x03E0) | (y << 5);
    }
}

void nes_ppu_reset_hori(ppu_t* ppu){
    ppu->v &= ~(0x1F);
    ppu->v |= ppu->t & 0x1F;
    ppu->v &= ~(1 << 10);
    ppu->v |= ppu->t & (1 << 10);
}

void nes_ppu_reset_vert(ppu_t* ppu){
    ppu->v &= ~(0b111 << 12);
    ppu->v &= ~(0x1F << 5);
    ppu->v |= ppu->t & (0b111 << 12);
    ppu->v |= ppu->t & (0x1F << 5);
    ppu->v &= ~(1 << 11);
    ppu->v |= ppu->t & (1 << 11);
}

void nes_ppu_read_nt(ppu_t* ppu){
    u16 addr = 0x2000 | (ppu->v & 0x0FFF);
    ppu->nt = ppu->read(ppu, addr);
}

void nes_ppu_read_at(ppu_t* ppu){
    u16 addr = 0x23C0 | (ppu->v & 0x0C00) | ((ppu->v >> 4) & 0x38) | ((ppu->v >> 2) & 0x07);
    ppu->at = ppu->read(ppu, addr);
    u8 coarse_x = ppu->v & 0x1F;
    u8 coarse_y = (ppu->v >> 5) & 0x1F;
    u8 mask = (bool)(coarse_x & 0b10) + (bool)(coarse_y & 0b10) * 2;
    ppu->at = (ppu->at >> (mask*2)) & 0b11;
}

void nes_ppu_read_bg_lsb(ppu_t* ppu){
    u16 pattern = ppu->ctrl & (1 << 4) ? 0x1000 : 0x0000;
    u16 fine_y = ppu->v >> 12;
    u16 addr = pattern + ppu->nt*16+fine_y;
    ppu->bg_lsb = ppu->read(ppu, addr);
}

void nes_ppu_read_bg_msb(ppu_t* ppu){
    u16 pattern = ppu->ctrl & (1 << 4) ? 0x1000 : 0x0000;
    u16 fine_y = ppu->v >> 12;
    u16 addr = pattern + ppu->nt*16+fine_y+8;
    ppu->bg_msb = ppu->read(ppu, addr);
}

void nes_ppu_fill_shifter(ppu_t* ppu){
    ppu->shifter_p0 &= 0xFF00;
    ppu->shifter_p1 &= 0xFF00;
    ppu->shifter_attr0 &= 0xFF00;
    ppu->shifter_attr1 &= 0xFF00;
    
    ppu->shifter_p0 |= ppu->bg_lsb;
    ppu->shifter_p1 |= ppu->bg_msb;

    ppu->shifter_attr0 |= ppu->at & 0b01 ? 0xFF : 00;
    ppu->shifter_attr1 |= ppu->at & 0b10 ? 0xFF : 00;
}

void nes_ppu_shift(ppu_t* ppu){
    ppu->shifter_p0 <<= 1;
    ppu->shifter_p1 <<= 1;
    ppu->shifter_attr0 <<= 1;
    ppu->shifter_attr1 <<= 1;
}

void nes_ppu_put_pixel(ppu_t* ppu){
    u32 intra_scan_cycles = ppu->cycles % PPU_CYCLES_PER_SCANLINE;
    u32 scanline = ppu->cycles / PPU_CYCLES_PER_SCANLINE;
    int screen_x = intra_scan_cycles - 2;
    int screen_y = scanline;
    bool bg_p0 = ppu->shifter_p0 & (0x8000 >> ppu->x);
    bool bg_p1 = ppu->shifter_p1 & (0x8000 >> ppu->x);
    bool bg_a0 = ppu->shifter_attr0 & (0x8000 >> ppu->x);
    bool bg_a1 = ppu->shifter_attr1 & (0x8000 >> ppu->x);
    bool bg_hide = (!(ppu->mask & (1 << 1)) && screen_x < 8) || !(ppu->mask & (1 << 3));
    bool sprite_hide = (!(ppu->mask & (1 << 2)) && screen_x < 8) || !(ppu->mask & (1 << 4));
    u8 bg_col = bg_p0 | (bg_p1 << 1);
    u8 bg_pal = bg_a0 | (bg_a1 << 1);
    if(bg_hide)
        bg_col = 0;

    u8 bg_nes_col = nes_ppu_get_palette_color(ppu, bg_pal, bg_col);
    u8 final_pix = bg_nes_col;

    for(int i = ppu->sprite_count - 1; i >= 0; i--){
        sprite_t* s = &ppu->sprites[i];
        if(!s->x){
            bool s_p0 = s->shifter_p0 & 0x80;
            bool s_p1 = s->shifter_p1 & 0x80;
            u8 s_col = sprite_hide ? 0b00 : s_p0 | (s_p1 << 1);

            s->shifter_p0 <<= 1;
            s->shifter_p1 <<= 1;

            if(!s_col)
                continue;

            if(!i && bg_col && !ppu->hit_0_triggered && ppu->can_hit_0){
                ppu->status |= (1 << 6);
                ppu->hit_0_triggered = true;
            }

            bool is_behind = s->attr & (1 << 5);
            if(is_behind && bg_col)
                final_pix = bg_nes_col;
            else
                final_pix = nes_ppu_get_palette_color(ppu, 0x4 | (s->attr & 0b11), s_col); 
        } else {
            s->x -= 1;
        }
    }

    pixels[screen_x + screen_y * width] = nes_ppu_convert_to_rgb(final_pix);
}

void nes_ppu_load_oam(ppu_t* ppu){
    u32 scanline = ppu->cycles / PPU_CYCLES_PER_SCANLINE;
    ppu->sprite_count = 0;
    ppu->can_hit_0 = false;
    u8 sprite_size = ppu->ctrl & (1 << 5) ? 16 : 8;
    memset(ppu->sprites, 0xFF, sizeof(ppu->sprites));

    for(int i = 0; i < 64; i++){
        u8 oam_y = ppu->oam[i*4];
        u8 oam_idx = ppu->oam[i*4+1];
        u8 oam_attr = ppu->oam[i*4+2];
        u8 oam_x = ppu->oam[i*4+3];
        if(scanline >= oam_y && scanline < oam_y + sprite_size){
            if(ppu->sprite_count == 8){
                ppu->status |= (1 << 5);
                break;
            }
            sprite_t* s = &ppu->sprites[ppu->sprite_count];
            s->x = oam_x;
            s->y = scanline - oam_y;
            if(oam_attr & (1 << 7))
                s->y = sprite_size - 1 - s->y;
            s->idx = oam_idx;
            s->attr = oam_attr;
            ppu->can_hit_0 |= (i == 0);
            ppu->sprite_count += 1;
        }
    }
}

void nes_ppu_fetch_sprite_lo(ppu_t* ppu){
    u32 intra_scan_cycles = ppu->cycles % PPU_CYCLES_PER_SCANLINE;
    u8 i = (intra_scan_cycles - 261) >> 3;
    u8 sprite_size = ppu->ctrl & (1 << 5) ? 16 : 8;
    sprite_t* s = &ppu->sprites[i];
    u16 base_addr;
    u8 idx;
    u8 y = s->y;
    if(sprite_size == 8){
        base_addr = ppu->ctrl & (1 << 3) ? 0x1000 : 0x0000;
        idx = s->idx;
    } else {
        base_addr = s->idx & 1 ? 0x1000 : 0x0000;
        idx = s->idx & 0XFE;
        if(s->y >= 8){
            y -= 8;
            idx += 1;
        }
    }
    y &= 0b111;
    u16 tile_addr = base_addr + idx*16;
    s->shifter_p0 = ppu->read(ppu, tile_addr+y);
    if(s->attr & (1 << 6))
        s->shifter_p0 = flip_x_lut[s->shifter_p0];
}


void nes_ppu_fetch_sprite_hi(ppu_t* ppu){
    u32 intra_scan_cycles = ppu->cycles % PPU_CYCLES_PER_SCANLINE;
    u8 i = (intra_scan_cycles - 263) >> 3;
    u8 sprite_size = ppu->ctrl & (1 << 5) ? 16 : 8;
    sprite_t* s = &ppu->sprites[i];
    u16 base_addr;
    u8 idx;
    u8 y = s->y;
    if(sprite_size == 8){
        base_addr = ppu->ctrl & (1 << 3) ? 0x1000 : 0x0000;
        idx = s->idx;
    } else {
        base_addr = s->idx & 1 ? 0x1000 : 0x0000;
        idx = s->idx & 0XFE;
        if(s->y >= 8){
            y -= 8;
            idx += 1;
        }
    }
    y &= 0b111;
    u16 tile_addr = base_addr + idx*16;
    s->shifter_p1 = ppu->read(ppu, tile_addr+y+8);
    if(s->attr & (1 << 6))
        s->shifter_p1 = flip_x_lut[s->shifter_p1];
}

void nes_ppu_fetch_sprites(ppu_t* ppu){
    u8 sprite_size = ppu->ctrl & (1 << 5) ? 16 : 8;
    for(int i = 0; i < 8; i++){
        sprite_t* s = &ppu->sprites[i];
        u16 base_addr;
        u8 idx;
        if(sprite_size == 8){
            base_addr = ppu->ctrl & (1 << 3) ? 0x1000 : 0x0000;
            idx = s->idx;
        } else {
            base_addr = s->idx & 1 ? 0x1000 : 0x0000;
            idx = s->idx & 0XFE;
            if(s->y >= 8){
                s->y -= 8;
                idx += 1;
            }
        }
        u16 tile_addr = base_addr + idx*16;
        s->shifter_p0 = ppu->read(ppu, tile_addr+s->y);
        s->shifter_p1 = ppu->read(ppu, tile_addr+s->y+8);
        if(s->attr & (1 << 6)){
            s->shifter_p0 = flip_x_lut[s->shifter_p0];
            s->shifter_p1 = flip_x_lut[s->shifter_p1];
        }
    }
}