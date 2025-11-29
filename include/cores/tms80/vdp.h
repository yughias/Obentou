#ifndef __VDP_H__
#define __VDP_H__

#include "cpus/z80.h"

#include "types.h"
#include "utils/serializer.h"

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

#define VDP_STRUCT(X) \
X(VDP_REGION, region, 1, 0) \
X(u8, VRAM, VRAM_SIZE, 1, 0) \
X(u8, CRAM, CRAM_SIZE_GG, 1, 0) \
X(u8, cram_latch, 1, 0) \
X(u8, cram_size, 1, 0) \
X(u8, buffer, 1, 0) \
X(u16, control_port, 1, 0) \
X(u16, vram_address, 1, 0) \
X(bool, control_port_flag, 1, 0) \
X(bool, cram_dst, 1, 0) \
X(int, v_counter, 1, 0) \
X(u8, scroll_y, 1, 0) \
X(u8, status_reg, 1, 0) \
X(u8, line_reg, 1, 0) \
X(u8, regs, 16, 1, 0) \
X(int, framebuffer[SCREEN_WIDTH_SMS*SCREEN_HEIGHT_SMS], 0, 0)

DECLARE_SERIALIZABLE_STRUCT(vdp, VDP_STRUCT);

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