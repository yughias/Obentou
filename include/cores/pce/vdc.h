#ifndef __VDC_H__
#define __VDC_H__

#include "types.h"
#include "SDL_MAINLOOP.h"

#include "cores/pce/vce.h"

#include "utils/serializer.h"

#define VRAM_SIZE (1 << 16)

#define SCREEN_WIDTH 256

#define SCREEN_HEIGHT 242
#define TOP_OVERSCAN 14

typedef enum VDC_REGS {
    MAWR = 0,
    MARR,
    VRR_VWR,
    CR = 5,
    RCR,
    BXR,
    BYR,
    MWR,
    HSR,
    HDR,
    VPR,
    VDW,
    VCR,
    DCR,
    SOUR,
    DESR,
    LENR,
    SATB
} VDC_REGS;

typedef enum VDC_STATUS {
    VDC_STAT_COLLISION = 1 << 0,
    VDC_STAT_RCR = 1 << 2,
    VDC_STAT_SATB = 1 << 3,
    VDC_STAT_DMA = 1 << 4,
    VDC_STAT_VBLANK = 1 << 5
} VDC_STATUS;

typedef enum VDC_EVENT {
    START_LINE = 0,
    END_LINE
} VDC_EVENT;

#define VDC_STRUCT(X) \
    X(u8, vram, VRAM_SIZE, 1, 0) \
    X(u16, satb, 256, 1, 0) \
    X(u8, selector, 1, 0) \
    X(u16, status, 1, 0) \
    X(u16, regs, 0x20, 1, 0) \
    X(u16, read_buffer, 1, 0) \
    X(vce_t*, vce, 0, 0) \
    X(u16, yscroll, 1, 0) \
    X(bool, irq, 1, 0) \
    X(bool, satb_dma, 1, 0) \
    X(int, next_event_cycles, 1, 0) \
    X(VDC_EVENT, next_event, 1, 0) \
    X(int, display_counter, 1, 0) \
    X(int, frame_counter, 1, 0) \
    X(u8, bg_buffer, 512, 0, 0) \
    X(bool, sprite_buffer, 512, 0, 0) \
    X(void*, ctx, 0, 0)

DECLARE_SERIALIZABLE_STRUCT(vdc, VDC_STRUCT);

void pce_vdc_write_reg(vdc_t* v, u8 idx, u8 value, bool hi);
u8 pce_vdc_read_reg(vdc_t* v, u8 idx, bool hi);
void pce_vdc_get_tilemap_size(const vdc_t* v, u8* w, u8* h);

void pce_vdc_draw_tilemap(SDL_Window** win, vdc_t* v);
void pce_vdc_draw_sprites(SDL_Window** win, vdc_t* v);
bool pce_vdc_vblank_on(const vdc_t* v);
bool pce_vdc_rcr_on(const vdc_t* v);
void pce_vdc_render_line(vdc_t* v, const vce_t* vce);
void pce_vdc_step(vdc_t* v, u32 cycles);

void pce_vdc_write_io(vdc_t* v, u8 offset, u8 value);
u8 pce_vdc_read_io(vdc_t* v, u8 offset);


#endif