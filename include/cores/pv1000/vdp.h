#ifndef __VDP_H__
#define __VDP_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct vdp_t {
    uint16_t rom_tile_addr;
    uint16_t ram_tile_addr;
    bool all_tiles_in_rom;
} vdp_t;

void pv1000_vdp_render(vdp_t* vdp, uint8_t* memory);

#endif
