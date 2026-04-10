#include "cores/pacman/video.h"
#include "SDL_MAINLOOP.h"

#include <stdbool.h>

static const u8 color_table[8] = {
    0x21, 0x47, 0x97,
    0x21, 0x47, 0x97,
    0x51, 0xAE
};

static int pacman_color_from_rom(pacman_t* p, u8 idx)
{
    u8 c = p->colorROM[idx];
    u8 r = 0, g = 0, b = 0;
    for (int i = 0; i < 3; i++) { r += (c & 1) * color_table[i];   c >>= 1; }
    for (int i = 0; i < 3; i++) { g += (c & 1) * color_table[3+i]; c >>= 1; }
    for (int i = 0; i < 2; i++) { b += (c & 1) * color_table[6+i]; c >>= 1; }
    return color(r, g, b);
}

void pacman_get_palette(pacman_t* p, u8 idx, int* pal)
{
    idx &= 0x1F;
    for (int i = 0; i < 4; i++)
        pal[i] = pacman_color_from_rom(p, p->paletteROM[idx * 4 + i]);
}

void pacman_get_tile(pacman_t* p, u8 tile_idx, u8 pal_idx, int* out)
{
    int pal[4];
    pacman_get_palette(p, pal_idx, pal);
    for (int i = 0; i < 16; i++) {
        u8 byte = p->tileROM[tile_idx * 16 + i];
        for (int j = 0; j < 4; j++) {
            u8 mask = 1 << j;
            u8 pc   = (bool)(byte & mask) | ((bool)(byte & (mask << 4)) << 1);
            u8 ox   = 7 - (i % 8);
            u8 oy   = i < 8 ? 4 + (3 - j) : 3 - j;
            out[ox + oy * 8] = pal[pc];
        }
    }
}

void pacman_draw_tile(int ox, int oy, int* tile)
{
    ox *= 8; oy *= 8;
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            pixels[(ox + x) + (oy + y) * width] = tile[x + y * 8];
}

static void pacman_draw_sprite(pacman_t* p, int ox, int oy,
                               u8 spr_idx, u8 pal_idx, bool flip_x, bool flip_y)
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
                if (screen_x > 0 && screen_x < width &&
                    screen_y >= 16 && screen_y < height - 16)
                    pixels[screen_x + screen_y * width] = pix;
            }
            sx += step_x;
        }
        sy += step_y;
    }
}

void pacman_draw_video(pacman_t* p)
{
    u8* tiles_info    = p->RAM + PACMAN_TILE_RAM_OFF;
    u8* palettes_info = p->RAM + PACMAN_PAL_RAM_OFF;
    u8* sprites_info  = p->RAM + PACMAN_SPR_RAM_OFF;

    int tile[64];

    /* Bottom two score/status rows */
    for (int y = 0; y < 2; y++)
        for (int x = 0; x < PACMAN_VIDEO_COLS; x++) {
            int d = 0x02 + x + y * 0x20;
            pacman_get_tile(p, tiles_info[d], palettes_info[d], tile);
            pacman_draw_tile(PACMAN_VIDEO_COLS - 1 - x, PACMAN_VIDEO_ROWS - 2 + y, tile);
        }

    /* Top two score/status rows */
    for (int y = 0; y < 2; y++)
        for (int x = 0; x < PACMAN_VIDEO_COLS; x++) {
            int d = 0x3C2 + x + y * 0x20;
            pacman_get_tile(p, tiles_info[d], palettes_info[d], tile);
            pacman_draw_tile(PACMAN_VIDEO_COLS - 1 - x, y, tile);
        }

    /* Central 32 gameplay rows */
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < PACMAN_VIDEO_COLS; x++) {
            int d = 0x40 + y + x * 0x20;
            pacman_get_tile(p, tiles_info[d], palettes_info[d], tile);
            pacman_draw_tile(PACMAN_VIDEO_COLS - 1 - x, 2 + y, tile);
        }

    /* 8 hardware sprites, drawn back-to-front */
    for (int i = 7; i >= 0; i--) {
        u8  pal_idx = sprites_info[i * 2 + 1];
        u8  spr_idx = (sprites_info[i * 2] & 0xFC) >> 2;
        bool flip_x = sprites_info[i * 2] & 0x2;
        bool flip_y = sprites_info[i * 2] & 0x1;
        int cx = width  - p->SPRITE_COORDS[i * 2]     + 15;
        int cy = height - p->SPRITE_COORDS[i * 2 + 1] - 16;
        pacman_draw_sprite(p, cx, cy, spr_idx, pal_idx, flip_x, flip_y);
    }
}
