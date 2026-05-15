#include "cores/pacman/visualizers.h"
#include "cores/pacman/pacman.h"
#include "cores/pacman/video.h"
#include "SDL_MAINLOOP.h"

bool pacman_draw_tile_rom(void* ctx)
{
    pacman_t* p = (pacman_t*)ctx;
    size(16 * 8, 16 * 8);

    int tile[64];
    for (int ty = 0; ty < 16; ty++) {
        for (int tx = 0; tx < 16; tx++) {
            u8 tile_idx = (u8)(ty * 16 + tx);
            pacman_get_tile(p, tile_idx, 1, tile);
            pacman_draw_tile(tx, ty, tile);
        }
    }

    return true;
}

bool pacman_draw_audiorom(void* ctx)
{
    pacman_t* p = (pacman_t*)ctx;

    const int tile_w = 32;
    const int tile_h = 16;
    const int grid_sz = 1;
    const int tiles = 4;

    // total size = tiles * size + (tiles + 1) grid lines
    int total_w = tiles * tile_w + (tiles + 1) * grid_sz;
    int total_h = tiles * tile_h + (tiles + 1) * grid_sz;

    size(total_w, total_h);

    int bg   = color(20, 20, 20);
    int fg   = color(80, 200, 100);
    int grid = color(40, 40, 40);

    for (int i = 0; i < 16; i++) {
        int tx = i % tiles;
        int ty = i / tiles;

        int base_x = tx * (tile_w + grid_sz) + grid_sz;
        int base_y = ty * (tile_h + grid_sz) + grid_sz;

        for (int s = 0; s < tile_w; s++) {
            u8 val = p->audioROM[i * tile_w + s] & 0xF;
            int bar_top = tile_h - 1 - val;

            for (int y = 0; y < tile_h; y++) {
                pixels[(base_x + s) + (base_y + y) * stride] =
                    (y >= bar_top) ? fg : bg;
            }
        }
    }

    for (int i = 0; i <= tiles; i++) {
        int y_line = i * (tile_h + grid_sz);
        for (int x = 0; x < total_w; x++)
            pixels[x + y_line * stride] = grid;

        int x_line = i * (tile_w + grid_sz);
        for (int y = 0; y < total_h; y++)
            pixels[x_line + y * stride] = grid;
    }

    return true;
}


static void draw_sprite(pacman_t* p, int ox, int oy, u8 spr_idx, u8 pal_idx, bool flip_x, bool flip_y)
{
    int spr[256], pal[4];
    pacman_get_palette(p, pal_idx, pal);

    for (int i = 0; i < 64; i++) {
        u8 byte = p->spriteROM[spr_idx * 64 + i];
        for (int j = 0; j < 4; j++) {
            u8 mask = 1 << j;
            u8 pc   = (bool)(byte & mask) | ((bool)(byte & (mask << 4)) << 1);
            u8 px   = i < 32 ? 15 - (i % 8) : 7 - (i % 8);
            u8 py;
            if      (i <  8) py = 12 + (3 - j);
            else if (i < 16) py =  3 - j;
            else if (i < 24) py =  4 + (3 - j);
            else if (i < 32) py =  8 + (3 - j);
            else if (i < 40) py = 12 + (3 - j);
            else if (i < 48) py =  3 - j;
            else if (i < 56) py =  4 + (3 - j);
            else             py =  8 + (3 - j);
            spr[px + py * 16] = pal[pc];
        }
    }

    int sy = flip_y ? 15 : 0, step_y = flip_y ? -1 : 1;
    for (int y = 0; y < 16; y++) {
        int sx = flip_x ? 15 : 0, step_x = flip_x ? -1 : 1;
        for (int x = 0; x < 16; x++) {
            int pix = spr[sx + sy * 16];
            if (pix != pal[0]) {
                int screen_x = ox + x;
                int screen_y = (oy + y) % height;
                pixels[screen_x + screen_y * stride] = pix;
            }
            sx += step_x;
        }
        sy += step_y;
    }
}

bool pacman_draw_sprite_rom(void* ctx)
{
    pacman_t* p = (pacman_t*)ctx;
    size(8 * 16, 8 * 16);

    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            u8  spr_idx = x + y * 8;
            draw_sprite(p, x * 16, y * 16, spr_idx, 1, false, false);
        }

    return true;
}

bool pacman_draw_sprites(void* ctx)
{
    pacman_t* p = (pacman_t*)ctx;
    size(4 * 16, 2 * 16);

    u8* sprites_info  = p->RAM + PACMAN_SPR_RAM_OFF;
    for (int i = 0; i < 8; i++) {
        u8  pal_idx = sprites_info[i * 2 + 1];
        u8  spr_idx = (sprites_info[i * 2] & 0xFC) >> 2;
        bool flip_x = (bool)(sprites_info[i * 2] & 0x2) ^ p->is_270_degree;
        bool flip_y = (bool)(sprites_info[i * 2] & 0x1) ^ p->is_270_degree;
        draw_sprite(p, (i % 4) * 16, (i / 4) * 16, spr_idx, pal_idx, flip_x, flip_y);
    }

    return true;
}

bool pacman_draw_palettes(void* ctx)
{
    pacman_t* p = (pacman_t*)ctx;
    size(8*4, 8);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width / 4; x++) {
            int idx = x + y * width / 4;
            int pal[4];
            pacman_get_palette(p, idx, pal);
            for (int i = 0; i < 4; i++)
                pixels[x*4+i + y * stride] = pal[i];
        }
    }

    return true;
}

bool pacman_draw_colors(void* ctx)
{
    pacman_t* p = (pacman_t*)ctx;
    size(8, 4);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = x + y * width;
            int col = pacman_color_from_rom(p, idx);
            pixels[x + y * stride] = col;
        }
    }

    return true;
}