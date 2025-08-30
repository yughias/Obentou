#ifndef __VDP_H__
#define __VDP_H__

#include "types.h"

typedef struct vdp_t {
    u16 rom_tile_addr;
    u16 ram_tile_addr;
    bool all_tiles_in_rom;
} vdp_t;

void pv1000_vdp_render(vdp_t* vdp, u8* memory);

#endif
