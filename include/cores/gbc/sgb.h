#ifndef __SGB_H__
#define __SGB_H__

#include "types.h"

#include "utils/serializer.h"

#define SGB_MAX_FADE_COUNTER 200
#define SGB_WIDTH 256
#define SGB_HEIGHT 224

typedef enum SGB_VRAM_TRANSFER {SGB_VRAM_TRANSFER_NONE, SGB_VRAM_TRANSFER_PAL, SGB_VRAM_TRANSFER_ATTR, SGB_VRAM_TRANSFER_TILE_00_7F, SGB_VRAM_TRANSFER_TILE_80_FF, SGB_VRAM_TRANSFER_TILEMAP } SGB_VRAM_TRANSFER;
typedef enum SGB_MASK { SGB_MASK_NONE, SGB_MASK_FREEZE, SGB_MASK_BLACK, SGB_MASK_COLOR0 } SGB_MASK;
typedef struct gb_t gb_t;

#define SGB_STRUCT(X) \
    X(bool, is_receiving, 1, 0) \
    X(size_t, bit_idx, 1, 0) \
    X(size_t, byte_idx, 1, 0) \
    X(size_t, packet_idx, 1, 0) \
    X(bool, ready_for_stop, 1, 0) \
    X(bool, ready_for_pulse, 1, 0) \
    X(u8, packets, 7 * 16, 1, 0) \
    X(bool, multiplayer_enabled, 1, 0) \
    X(u8, multiplayer_mode, 1, 0) \
    X(u8, player_count, 1, 0) \
    X(u16, colors, 4 * 4, 1, 0) \
    X(u8, attributes, 20 * 18, 1, 0) \
    X(u16, pal_ram, 512 * 4, 1, 0) \
    X(u8, atf, 4096, 1, 0) \
    X(u8, tile_ram, 8192, 1, 0) \
    X(u8, tilemap, 4096, 1, 0) \
    X(SGB_VRAM_TRANSFER, vram_transfer, 1, 0) \
    X(SGB_MASK, mask, 1, 0) \
    X(bool, fade_enabled, 1, 0) \
    X(bool, must_render, 0, 0) \
    X(int, gameboy_window[160 * 144], 0, 0) \
    X(int, screen[SGB_WIDTH * SGB_HEIGHT], 0, 0)

DECLARE_SERIALIZABLE_STRUCT(sgb, SGB_STRUCT);

void sgb_write(sgb_t* sgb, bool bit0, bool bit1);
void sgb_render(sgb_t* sgb);
void sgb_show_to_screen(gb_t* gb);
void sgb_init_palettes(sgb_t* sgb, const char* rom_name);
bool gb_has_sgb_functionality(const u8* rom);

#endif