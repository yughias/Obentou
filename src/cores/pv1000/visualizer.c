#include "cores/pv1000/pv1000.h"

#include "SDL_MAINLOOP.h"

bool pv1000_draw_tileset(pv1000_t* pv){
    size(16*8, 16*8);

    vdp_t* vdp = &pv->vdp;
    for(int x = 0; x < 16; x++){
        for(int y = 0; y < 16; y++){
            int tile_idx = x + y*16;
            u8* tile;
            if(vdp->all_tiles_in_rom || tile_idx < 0xE0){
                tile = &pv->memory[vdp->rom_tile_addr + tile_idx*32];
            } else {
                tile_idx &= 0x1F;
                tile = &pv->memory[vdp->ram_tile_addr + tile_idx*32];
            }

            pv1000_vdp_render_tile(tile, x*8, y*8);
        }
    }

    return true;
}