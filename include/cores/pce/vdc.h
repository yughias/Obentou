#ifndef __VDC_H__
#define __VDC_H__

#include "types.h"
#include "SDL_MAINLOOP.h"

#include "cores/pce/vce.h"

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

typedef struct vdc_t {
    u8 vram[VRAM_SIZE];
    u16 satb[256];
    u8 selector;
    u16 status;
    u16 regs[0x20];
    u16 read_buffer;
    vce_t* vce;
    u16 yscroll;
    bool irq;

    bool satb_dma;

    int next_event_cycles;
    VDC_EVENT next_event;

    int display_counter;
    int frame_counter;

    u8 bg_buffer[512];
    bool sprite_buffer[512];

    void* ctx;
} vdc_t;

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