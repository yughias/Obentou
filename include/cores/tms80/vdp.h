#ifndef __VDP_H__
#define __VDP_H__

#include "cpus/z80.h"

#include "types.h"

#define SCREEN_WIDTH_SMS 256
#define SCREEN_HEIGHT_SMS 192
#define SCREEN_WIDTH_GG 160
#define SCREEN_HEIGHT_GG 144

#define GG_START_X (SCREEN_WIDTH_SMS >> 1) - (SCREEN_WIDTH_GG >> 1)
#define GG_START_Y (SCREEN_HEIGHT_SMS >> 1) - (SCREEN_HEIGHT_GG >> 1)

#define VBLANK_CYCLES (SCREEN_HEIGHT*CYCLES_PER_LINE)

#define ASPECT_RATIO (4.0f/3.0f)

#define VRAM_SIZE 0x4000
#define CRAM_SIZE_SMS 32
#define CRAM_SIZE_GG 64

typedef enum VDP_REGION {REGION_NTSC, REGION_PAL} VDP_REGION;

typedef struct vdp_t {
    VDP_REGION region;
    u8 VRAM[VRAM_SIZE];
    u8 CRAM[CRAM_SIZE_GG];
    u8 cram_latch;
    u8 cram_size;
    u8 buffer;
    u16 control_port;
    u16 vram_address;
    bool control_port_flag;
    bool cram_dst;
    int v_counter;
    u8 scroll_y;
    u8 status_reg;
    u8 line_reg;
    u8 regs[16];
    int framebuffer[SCREEN_WIDTH_SMS*SCREEN_HEIGHT_SMS];
} vdp_t;

void tms80_vdp_write_to_control_port(vdp_t* vdp, u8 byte);
u8 tms80_vdp_read_from_data_port(vdp_t* vdp);
void tms80_vdp_write_to_data_port(vdp_t* vdp, u8 byte);
u8 tms80_vdp_read_status_register(vdp_t* vdp, z80_t* z80);
void tms80_vdp_fire_interrupt(vdp_t* vdp, z80_t* z80, bool is_vblank);
u8 tms80_vdp_get_v_counter(vdp_t* vdp);
void tms80_vdp_skip_bios(vdp_t* vdp);
void tms80_vdp_render_line(vdp_t* vdp, int line);
void tms80_vdp_show_frame(const vdp_t* vdp);

#endif