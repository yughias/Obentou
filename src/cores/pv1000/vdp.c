#include "cores/pv1000/vdp.h"
#include "SDL_MAINLOOP.h"

static void render_tile(u8* tile, int x0, int y0);

void pv1000_vdp_render(vdp_t* vdp, u8* memory){
    u8* tilemap = &memory[0xB800];

    for(int x = 0; x < 28; x++){
        for(int y = 0; y < 24; y++){
            int tile_idx = tilemap[(x+2) + y*32];
            u8* tile;
            if(vdp->all_tiles_in_rom || tile_idx < 0xE0){
                tile = &memory[vdp->rom_tile_addr + tile_idx*32];
            } else {
                tile_idx &= 0x1F;
                tile = &memory[vdp->ram_tile_addr + tile_idx*32];
            }

            render_tile(tile, x*8, y*8);
        }
    }

    renderPixels();
}

static void render_tile(u8* tile, int x0, int y0){
    for(int y = 0; y < 8; y++) {
        u8 r_plane = tile[y +  8];
        u8 g_plane = tile[y + 16];
        u8 b_plane = tile[y + 24];

        for(int x = 0; x < 8; x++) {
            int pos = 7-x;

            u8 r = ((r_plane >> pos) & 1)*255;
            u8 g = ((g_plane >> pos) & 1)*255;
            u8 b = ((b_plane >> pos) & 1)*255;

            pixels[(x0 + x) + (y0 + y) * width] = color(r, g, b);
        }
    }
}
