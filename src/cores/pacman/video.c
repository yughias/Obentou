#include "cores/pacman/video.h"
#include "SDL_MAINLOOP.h"

#include <stdbool.h>

// TODO FIX PALETTES AND COLOR BANKS

static const u8 color_table[8] = {
    0x21, 0x47, 0x97,
    0x21, 0x47, 0x97,
    0x51, 0xAE
};

int pacman_color_from_rom(pacman_t* p, u8 idx)
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
    idx |= p->jrpacman.palettebank << 5;
    for (int i = 0; i < 4; i++)
        pal[i] = pacman_color_from_rom(p, p->paletteROM[idx * 4 + i] | (p->jrpacman.colorbank << 4));
}

static void pacman_draw_sprite(pacman_t* p, int ox, int oy,
                               u8 spr_idx, u8 pal_idx, bool flip_x, bool flip_y)
{
    int spr[256], pal[4];
    pacman_get_palette(p, pal_idx, pal);

    const u8* spriteROM = p->jrpacman.spritebank ? p->spriteROM + 0x1000 : p->spriteROM;

    for (int i = 0; i < 64; i++) {
        u8 byte = spriteROM[spr_idx * 64 + i];
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
                int screen_idx = screen_x + screen_y * width;
                if (screen_x > 0 && screen_x < width && screen_y >= 16 && screen_y < height - 16) {
                    if (!p->jrpacman.bgpriority || !p->bg_priority_map[screen_idx])
                        pixels[screen_idx] = pix;
                }
            }
            sx += step_x;
        }
        sy += step_y;
    }
}

static u16 pacman_get_idx(const pacman_t* p, int screen_x, int y) {
    int x = screen_x >> 3;
    if (y < 2)
        return 0x3C2 + x + y * 0x20;
    if (y >= PACMAN_VIDEO_ROWS - 2)
        return 0x02 + x + (y - PACMAN_VIDEO_ROWS + 2) * 0x20;
    return 0x40 + (y-2) + x * 0x20;
}

static u8 pacman_get_tile_idx(const pacman_t* p, int screen_x, int y) {
    return p->RAM[pacman_get_idx(p, screen_x, y)];
}

static u8 pacman_get_palette_idx(const pacman_t* p, int screen_x, int y) {
    return p->RAM[pacman_get_idx(p, screen_x, y) + PACMAN_PAL_RAM_OFF];
}

static u8 jrpacman_get_tile_idx(const pacman_t* p, int screen_x, int y) {
    int x = screen_x >> 3;
    if (y == 0)
        return p->RAM[1858 + x];
    if (y == 1)
        return p->RAM[1890 + x];
   
    if (y == 34)
        return p->RAM[1794 + x];
    if (y == 35)
        return p->RAM[1826 + x];

    return p->RAM[64 + (y-2) + x * 32];
}

static u8 jrpacman_get_palette_idx(const pacman_t* p, int screen_x, int y) {
    int x = screen_x >> 3;
    if (y == 0)
        return p->RAM[1986 + x];
    if (y == 1)
        return p->RAM[2018 + x];
    if (y == 34)
        return p->RAM[1922 + x];
    if (y == 35)
        return p->RAM[1954 + x];

    return p->RAM[y-2];
}

static int pacman_get_tile_pix(pacman_t* p, u8 tile_idx, u8 pal_idx, int px, int py, bool* bg_priority)
{
    int pal[4];
    u16 tile_bank = p->jrpacman.tilebank ? 0x1000 : 0;

    // rotate coordinates
    int tmp = 7 - px;
    px = py;
    py = tmp;

    pacman_get_palette(p, pal_idx, pal);

    u8 byte = p->tileROM[tile_bank + (tile_idx << 4) + ((px >= 4) ? (7 - py) : (15 - py))];

    u8 mask = 1 << (3 - (px & 3));
    u8 pc   = (bool)(byte & mask) |
              (((bool)(byte & (mask << 4))) << 1);

    *bg_priority = pc;

    return pal[pc];
}

void pacman_draw_video(pacman_t* p)
{
    u8* sprites_info  = p->RAM + PACMAN_SPR_RAM_OFF;
    bool is_jrpacman = p->type == PACMAN_TYPE_JRPACMAN;

    for (int y = 0; y < PACMAN_VIDEO_ROWS*8; y++) {
        for (int x = 0; x < PACMAN_VIDEO_COLS*8; x++) {
            int ty = y >> 3;
            int screen_x = x;
            if( ty >= 2 && ty < PACMAN_VIDEO_ROWS - 2)
                screen_x = (x + p->jrpacman.scroll) % (8*54);
            int screen_idx = width - 1 - x + y * width;
            u8 tile_idx = is_jrpacman ? jrpacman_get_tile_idx(p, screen_x, ty) : pacman_get_tile_idx(p, screen_x, ty);
            u8 pal_idx  = is_jrpacman ? jrpacman_get_palette_idx(p, screen_x, ty) : pacman_get_palette_idx(p, screen_x, ty);
            int pix = pacman_get_tile_pix(p, tile_idx, pal_idx, screen_x & 7, y & 7, &p->bg_priority_map[screen_idx]);
            pixels[screen_idx] = pix;
        }
    }

    /* 8 hardware sprites, drawn back-to-front */
    for (int i = 7; i >= 0; i--) {
        u8  pal_idx = sprites_info[i * 2 + 1];
        u8  spr_idx = (sprites_info[i * 2] & 0xFC) >> 2;
        // TODO ROTATE 270 DEG DEPENDS ON GAME
        bool flip_x = (bool)(sprites_info[i * 2] & 0x2) ^ p->is_270_degree;
        bool flip_y = (bool)(sprites_info[i * 2] & 0x1) ^ p->is_270_degree;
        int cx = p->is_270_degree ? p->SPRITE_COORDS[i * 2] - 15 - 16 : width - p->SPRITE_COORDS[i * 2] + 15;
        int cy = p->is_270_degree ? p->SPRITE_COORDS[i * 2 + 1] : height - p->SPRITE_COORDS[i * 2 + 1] - 16;
        pacman_draw_sprite(p, cx, cy, spr_idx, pal_idx, flip_x, flip_y);
    }
}
