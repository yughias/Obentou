#ifndef __PPU_H__
#define __PPU_H__

#include "types.h"
#include "cores/nes/ines.h"
#include "cpus/m6502.h"
#include "SDL_MAINLOOP.h"

#define BASIC_VRAM_SIZE (1 << 11)
#define EXTENDED_VRAM_SIZE (1 << 12)
#define PALETTE_RAM_SIZE 0x20
#define OAM_SIZE 256

typedef struct ppu_t ppu_t;
typedef u8 (*ppu_read_func)(ppu_t*, u16);
typedef void (*ppu_write_func)(ppu_t*, u16, u8);

typedef struct sprite_t {
    u8 x;
    u8 y;
    u8 attr;
    u8 idx;
    u8 shifter_p0;
    u8 shifter_p1;
} sprite_t;

typedef struct ppu_t {
    u8* vram;
    u16 vram_size;
    u8 palette_ram[PALETTE_RAM_SIZE];
    u8 oam[OAM_SIZE];
    u8 oam_addr;
    u8 mask;
    u8 ctrl;
    u8 status;
    u8 data;

    u8 x;
    u16 v;
    u16 t;
    bool w;
    
    u8 nt;
    u8 at;
    u8 bg_lsb;
    u8 bg_msb;

    u16 shifter_p0;
    u16 shifter_p1;
    u16 shifter_attr0;
    u16 shifter_attr1;

    u8 sprite_count;
    sprite_t sprites[8];
    bool can_hit_0;
    bool hit_0_triggered;
    bool nmi_pin;
    bool end_of_frame;

    void* ctx;

    u32 cycles;

    ppu_read_func read;
    ppu_write_func write;
} ppu_t;

void nes_ppu_inc_addr(ppu_t* ppu);
u16 nes_ppu_get_vram_addr(VRAM_ALIGN align, u16 addr, u16 vram_size);

void nes_ppu_get_grayscale_tile(ppu_t* ppu, u16 base, u8 idx, int tile[64]);
void nes_ppu_get_tile(ppu_t* ppu, u16 base, u8 palette, u8 idx, int tile[64]);
void nes_ppu_draw_chr(ppu_t* ppu, SDL_Window** win);
void nes_ppu_draw_nametables(ppu_t* ppu, SDL_Window** win);
void nes_ppu_draw_palettes(ppu_t* ppu, SDL_Window** win);
void nes_ppu_draw_oam(ppu_t* ppu, SDL_Window** win);
u8 nes_ppu_get_palette_color(ppu_t* ppu, u8 palette, u8 idx);

int nes_ppu_convert_to_rgb(u8 nes_col);

void nes_ppu_sync(ppu_t* ppu);
void nes_ppu_step(ppu_t* ppu);

void nes_ppu_inc_hori(ppu_t* ppu);
void nes_ppu_inc_vert(ppu_t* ppu);
void nes_ppu_reset_hori(ppu_t* ppu);
void nes_ppu_reset_vert(ppu_t* ppu);

void nes_ppu_read_nt(ppu_t* ppu);
void nes_ppu_read_at(ppu_t* ppu);
void nes_ppu_read_bg_lsb(ppu_t* ppu);
void nes_ppu_read_bg_msb(ppu_t* ppu);
void nes_ppu_fill_shifter(ppu_t* ppu);
void nes_ppu_shift(ppu_t* ppu);
void nes_ppu_put_pixel(ppu_t* ppu);
void nes_ppu_load_oam(ppu_t* ppu);
void nes_ppu_fetch_sprites(ppu_t* ppu);
void nes_ppu_fetch_sprite_lo(ppu_t* ppu);
void nes_ppu_fetch_sprite_hi(ppu_t* ppu);

#endif