#include "cores/gbc/gb.h"

#include "SDL_MAINLOOP.h"

static int mono2_to_rgb(u8 mono2) {
    int col = (3 - mono2) * 85;
    return color(col, col, col);
}

bool gb_draw_tilemap(gb_t* gb){
    size(256, 256);

    ppu_t* ppu = &gb->ppu;
    u8* bgTileMap = gb_getTileMap(gb, ppu->LCDC_REG & ppu->BG_TILE_MAP_AREA_MASK);

    for(int y = 0; y < 256; y++){
        for(int x = 0; x < 256; x++){
            bool priority;
            int col = gb_getTileMapPixelRGB(gb, bgTileMap, x, y, &priority, &priority);
            // SGB returns 0-3 colors
            if (gb->console_type == SGB_TYPE)
                col = mono2_to_rgb(col);
            pixels[x + y*stride] = col;
        }
    }

    for(int y = 0; y < LCD_HEIGHT; y++){
        u8 offX1 = ppu->SCX_REG;
        u8 offX2 = ppu->SCX_REG + LCD_WIDTH;
        u8 offY = ppu->SCY_REG + y;
        pixels[offX1 + offY * stride] = color(255, 0, 0);
        pixels[offX2 + offY * stride] = color(255, 0, 0);
    }

    for(int x = 0; x < LCD_WIDTH; x++){
        u8 offX = + ppu->SCX_REG + x;
        u8 offY1 = ppu->SCY_REG;
        u8 offY2 = ppu->SCY_REG + LCD_HEIGHT;
        pixels[offX + offY1 * stride] = color(255, 0, 0);
        pixels[offX + offY2 * stride] = color(255, 0, 0);
    }

    return true;
}

bool gb_draw_tileset(gb_t* gb){
    size(16*8, (gb->console_type == CGB_TYPE ? 64 : 32) * 8);

    for(int ty = 0; ty < (height >> 3); ty++){
        for(int tx = 0; tx < (width >> 3); tx++){
            const u8* tilePtr = gb->VRAM + ((ty > 31) << 13) + (tx + (ty % 32) * 16)*16;
            for(int py = 0; py < 8; py++){
                for(int px = 0; px < 8; px++){
                    bool b0 = tilePtr[py * 2] & (1 << (7 - px));
                    bool b1 = tilePtr[py * 2 + 1] & (1 << (7 - px));
                    u8 col = b0 | (b1 << 1);
                    pixels[(tx*8+px) + (ty*8+py) * stride] = mono2_to_rgb(col);
                }
            }          
        }
    }

    return true;
}

static void draw_oam_at(gb_t* gb, int screenX, int screenY, u8 spriteIdx){
    ppu_t* ppu = &gb->ppu;
    u8* spriteData = &gb->OAM[spriteIdx * 4];
    
    bool flipX, flipY;
    bool backgroundOver;
    bool obp_n;
    u8 palette;
    u8* tilePtr;

    gb_getSpriteAttribute(gb, spriteData, &flipX, &flipY, &backgroundOver, &obp_n, &palette, &tilePtr);

    u8 height = ppu->LCDC_REG & ppu->OBJ_SIZE_MASK ? 16 : 8;
    for(u8 y = 0; y < height; y++)
        for(u8 x = 0; x < 8; x++){
            bool transparent;
            int col = gb_getSpritePixelRGB(gb, tilePtr, x, y, obp_n, palette, flipX, flipY, height, &transparent);
            if (gb->console_type == SGB_TYPE)
                col = mono2_to_rgb(col);
            if (transparent)
                col = color(255, 0, 255);
            pixels[(screenX + x) + (screenY + y)*width] = col;
        }
}

bool gb_draw_sprites(gb_t* gb){
    bool bigSprite = gb->ppu.LCDC_REG & gb->ppu.OBJ_SIZE_MASK;
    
    size(64, bigSprite ? 5*16 : 5*8);

    int offY = bigSprite ? 16 : 8;
    for(int i = 0; i < 40; i++){
        draw_oam_at(gb, (i%8)*8, (i/8)*offY, i);
    }

    return true;
}

static void draw_color_at(int x, int y, int palette, int pal_color, u8* cram){
    u8 lo_byte = cram[palette*8 + pal_color*2];
    u8 hi_byte = cram[palette*8 + pal_color*2 + 1];
    int col = CgbToRgb(lo_byte, hi_byte);
    pixels[x + y*stride] = col;
}

bool gb_draw_palettes(gb_t* gb){
    int n_palette = 1;
    
    switch (gb->console_type){
        case CGB_TYPE:
        n_palette = 8;
        break;
        case SGB_TYPE:
        n_palette = 4;
        break;
        case DMG_ON_CGB_TYPE:
        n_palette = 2;
        break;
    }

    size(4, n_palette);

    if(gb->console_type == SGB_TYPE) {
        for (int pal = 0; pal < 4; pal++) {
            for (int idx = 0; idx < 4; idx++) {
                u16 col = gb->sgb.colors[idx + pal * 4];
                u8 b = (col >> 10) & 0x1F;
                u8 g = (col >> 5) & 0x1F;
                u8 r = col & 0x1F;
                b = (b << 3) | (b >> 2);
                g = (g << 3) | (g >> 2);
                r = (r << 3) | (r >> 2);
                pixels[idx + pal * stride] = color(r, g, b);
            }
        }
    } else if(gb->console_type == DMG_TYPE || gb->console_type == MEGADUCK_TYPE){
        const ppu_t* ppu = &gb->ppu;
        for(int i = 0; i < 4; i++){
            pixels[i] = ppu->dmgColors[i];
        }
    } else {
        for(int palette = 0; palette < n_palette; palette++){
            for(int pal_color = 0; pal_color < 4; pal_color++){
                draw_color_at(pal_color, palette, palette, pal_color, gb->BGP_CRAM);
                draw_color_at(pal_color, palette, palette, pal_color, gb->OBP_CRAM);
            }
        }
    }

    return true;
}

bool gb_draw_window(gb_t* gb){
    const ppu_t* ppu = &gb->ppu;
    int startX = ppu->WX_REG - 7;
    int startY = ppu->WY_REG;

    int w = startX < LCD_WIDTH && startX >= 0 ? LCD_WIDTH - startX : 0;
    int h = startY < LCD_HEIGHT ? LCD_HEIGHT - startY : 0;

    if(w <= 0 || h <= 0)
        return true;

    size(w, h);

    u8* winTileMap = gb_getTileMap(gb, ppu->LCDC_REG & ppu->WIN_TILE_MAP_AREA_MASK);


    for(int y = 0; y < h; y++){
        for(int x = 0; x < w; x++){
            bool priority;
            int col = gb_getTileMapPixelRGB(gb, winTileMap, x, y, &priority, &priority);
            if (gb->console_type == SGB_TYPE)
                col = mono2_to_rgb(col);
            pixels[x + y*stride] = col;
        }
    }

    return true;
}


// ============================================================================
// POKEMON MEMORY MACROS
// (WRAM Addresses below are for Yellow. For Red/Blue, add +1 to all WRAM addresses)
// ============================================================================
#define Y_WRAM(gb, addr)             ((gb)->WRAM[(addr) - 0xC000])
#define Y_ROM(gb, bank, ptr)         ((gb)->ROM[(bank) * 0x4000 + ((ptr) & 0x3FFF)]) 
#define Y_ROM_ABS(gb, absolute_addr) ((gb)->ROM[absolute_addr])

// WRAM Map State
#define Y_ADDR_MAP_LAYOUT     0xC6E8
#define Y_ADDR_TILESET_ID     0xD366
#define Y_ADDR_MAP_HEIGHT     0xD367
#define Y_ADDR_MAP_WIDTH      0xD368
#define Y_ADDR_PLAYER_Y       0xD360
#define Y_ADDR_PLAYER_X       0xD361
#define Y_ADDR_PLAYER_VEC_X   0xC105
#define Y_ADDR_PLAYER_VEC_Y   0xC103
#define Y_ADDR_PLAYER_ANIM_INTRA_FRAME_COUNTER 0xC107
#define Y_ADDR_PLAYER_ANIM_FRAME_COUNTER 0xC108
#define Y_ADDR_PLAYER_FACING_DIR 0xC109
#define Y_ADDR_PLAYER_COLLISION 0xC10C
#define Y_ADDR_PLAYER_X_ADJUSTED 0xC10B
#define Y_ADDR_PLAYER_Y_ADJUSTED 0xC10A

#define EXTENDED_WIDTH        800
#define EXTENDED_HEIGHT       600

#define Y_ADDR_SPRITE_DATA_1  0xC100
#define Y_ADDR_SPRITE_DATA_2  0xC200

#define Y_ROM_TILESETS_TABLE  ((0x03 * 0x4000) + (0x4558 - 0x4000))
#define Y_ROM_SPRITE_TABLE    ((0x05 * 0x4000) + (0x42a9 - 0x4000))

extern int scx_reg;
extern int scy_reg;

bool gb_draw_yellow_revamped(gb_t* gb) {
    size(EXTENDED_WIDTH, EXTENDED_HEIGHT);
    
    for(int i = 0; i < EXTENDED_WIDTH * EXTENDED_HEIGHT; i++) {
        pixels[i] = color(30, 30, 30);
    }

    u8 tileset_id = Y_WRAM(gb, Y_ADDR_TILESET_ID);
    u8 map_height = Y_WRAM(gb, Y_ADDR_MAP_HEIGHT);
    u8 map_width  = Y_WRAM(gb, Y_ADDR_MAP_WIDTH);
    u8 player_x   = Y_WRAM(gb, Y_ADDR_PLAYER_X);
    u8 player_y   = Y_WRAM(gb, Y_ADDR_PLAYER_Y);

    int header_offset = Y_ROM_TILESETS_TABLE + (tileset_id * 12);
    u8 rom_bank   = Y_ROM_ABS(gb, header_offset + 0);
    u16 block_ptr = Y_ROM_ABS(gb, header_offset + 1) | (Y_ROM_ABS(gb, header_offset + 2) << 8);
    u16 gfx_ptr   = Y_ROM_ABS(gb, header_offset + 3) | (Y_ROM_ABS(gb, header_offset + 4) << 8);

    int gb_screen_canvas_x = (EXTENDED_WIDTH - LCD_WIDTH) / 2;
    int gb_screen_canvas_y = (EXTENDED_HEIGHT - LCD_HEIGHT) / 2;

    int player_px = player_x * 16;
    int player_py = player_y * 16;

    int facing_dir = Y_WRAM(gb, Y_ADDR_PLAYER_FACING_DIR);
    int frame_counter = Y_WRAM(gb, Y_ADDR_PLAYER_ANIM_INTRA_FRAME_COUNTER) | (Y_WRAM(gb, Y_ADDR_PLAYER_ANIM_FRAME_COUNTER) << 2);
    i8 player_vec_x = Y_WRAM(gb, Y_ADDR_PLAYER_VEC_X);
    i8 player_vec_y = Y_WRAM(gb, Y_ADDR_PLAYER_VEC_Y);

    int subpixel_scroll_x = 0;//gb->ppu.SCX_REG & 0xF;
    int subpixel_scroll_y = 0;//gb->ppu.SCY_REG & 0xF;

    int camera_x = player_px - 64 + subpixel_scroll_x;
    int camera_y = player_py - 64 + subpixel_scroll_y;

    int map_offset_x = camera_x - gb_screen_canvas_x;
    int map_offset_y = camera_y - gb_screen_canvas_y;


    int memory_width = map_width + 6; 
    int memory_height = map_height + 6;

    for (int by = 0; by < memory_height; by++) {
        for (int bx = 0; bx < memory_width; bx++) {
            
            u8 block_id = Y_WRAM(gb, Y_ADDR_MAP_LAYOUT + bx + (by * memory_width));
            
            for (int ty = 0; ty < 4; ty++) {
                for (int tx = 0; tx < 4; tx++) {
                    u8 tile_id = Y_ROM(gb, rom_bank, block_ptr + (block_id * 16) + (ty * 4) + tx);
                    
                    for (int py = 0; py < 8; py++) {
                        u8 row_byte1 = Y_ROM(gb, rom_bank, gfx_ptr + (tile_id * 16) + (py * 2) + 0);
                        u8 row_byte2 = Y_ROM(gb, rom_bank, gfx_ptr + (tile_id * 16) + (py * 2) + 1);
                        
                        for (int px = 0; px < 8; px++) {
                            
                            int absolute_px = (bx * 32) + (tx * 8) + px - 96;
                            int absolute_py = (by * 32) + (ty * 8) + py - 96;

                            int draw_x = absolute_px - map_offset_x;
                            int draw_y = absolute_py - map_offset_y;
                            
                            // Culling
                            if (draw_x < 0 || draw_x >= EXTENDED_WIDTH || 
                                draw_y < 0 || draw_y >= EXTENDED_HEIGHT) {
                                continue;
                            }
                            
                            int bit_index = 7 - px;
                            u8 b0 = (row_byte1 >> bit_index) & 1;
                            u8 b1 = (row_byte2 >> bit_index) & 1;
                            u8 col_idx = b0 | (b1 << 1);
                            
                            pixels[draw_x + draw_y * stride] = mono2_to_rgb(col_idx);
                        }
                    }
                }
            }
        }
    }

    for (int slot = 0; slot < 16; slot++) {
        u8 picture_id = Y_WRAM(gb, Y_ADDR_SPRITE_DATA_1 + (slot * 16) + 0);
        if (picture_id == 0 || picture_id == 0xFF) continue;

        int npc_y = Y_WRAM(gb, Y_ADDR_SPRITE_DATA_2 + (slot * 16) + 4) - 4;
        int npc_x = Y_WRAM(gb, Y_ADDR_SPRITE_DATA_2 + (slot * 16) + 5) - 4;

        int expected_screen_y = npc_y * 16 - camera_y;
        int expected_screen_x = npc_x * 16 - camera_x;

        u8 y_pixels = Y_WRAM(gb, Y_ADDR_SPRITE_DATA_1 + (slot * 16) + 4);
        u8 x_pixels = Y_WRAM(gb, Y_ADDR_SPRITE_DATA_1 + (slot * 16) + 6);

        int8_t delta_y = (int8_t)(y_pixels - (expected_screen_y & 0xFF));
        int8_t delta_x = (int8_t)(x_pixels  - (expected_screen_x & 0xFF));

        int draw_base_y = gb_screen_canvas_y + expected_screen_y + delta_y;
        int draw_base_x = gb_screen_canvas_x + expected_screen_x + delta_x;

        // Culling
        if (draw_base_x < -16 || draw_base_x >= EXTENDED_WIDTH || 
            draw_base_y < -16 || draw_base_y >= EXTENDED_HEIGHT) {
            continue;
        }

        int entry = Y_ROM_SPRITE_TABLE + ((picture_id - 1) * 4);
        u16 sprite_ptr = Y_ROM_ABS(gb, entry + 0) | (Y_ROM_ABS(gb, entry + 1) << 8);
        u8 sprite_bank = Y_ROM_ABS(gb, entry + 3);

        for (int tile = 0; tile < 4; tile++) {
            int tile_offset_x = (tile % 2 == 1) ? 8 : 0;
            int tile_offset_y = (tile >= 2) ? 8 : 0;

            for (int py = 0; py < 8; py++) {
                u8 row_byte1 = Y_ROM(gb, sprite_bank, sprite_ptr + (tile * 16) + (py * 2) + 0);
                u8 row_byte2 = Y_ROM(gb, sprite_bank, sprite_ptr + (tile * 16) + (py * 2) + 1);

                for (int px = 0; px < 8; px++) {
                    int draw_x = draw_base_x + tile_offset_x + px;
                    int draw_y = draw_base_y + tile_offset_y + py;

                    int bit_index = 7 - px;
                    u8 b0 = (row_byte1 >> bit_index) & 1;
                    u8 b1 = (row_byte2 >> bit_index) & 1;
                    u8 col_idx = b0 | (b1 << 1);

                    if (col_idx == 0) continue;

                    if (draw_x >= 0 && draw_x < EXTENDED_WIDTH && draw_y >= 0 && draw_y < EXTENDED_HEIGHT) {
                        pixels[draw_x + draw_y * stride] = mono2_to_rgb(col_idx);
                    }
                }
            }
        }
    }

    SDL_Surface* main_surf = getMainWindowSurface();
    if (main_surf && main_surf->pixels) {
        u32* main_pixels = (u32*)main_surf->pixels;
        int main_pitch = main_surf->pitch / 4; 
        
        for (int y = 0; y < LCD_HEIGHT; y++) {
            for (int x = 0; x < LCD_WIDTH; x++) {
                int dest_x = gb_screen_canvas_x + x;
                int dest_y = gb_screen_canvas_y + y;
                pixels[dest_x + dest_y * stride] = main_pixels[x + y * main_pitch];
            }
        }
    }
    
    for (int i = 0; i < LCD_WIDTH; i++) {
        pixels[(gb_screen_canvas_x + i) + (gb_screen_canvas_y) * stride] = color(255, 0, 0);
        pixels[(gb_screen_canvas_x + i) + (gb_screen_canvas_y + LCD_HEIGHT - 1) * stride] = color(255, 0, 0);
    }
    for (int i = 0; i < LCD_HEIGHT; i++) {
        pixels[(gb_screen_canvas_x) + (gb_screen_canvas_y + i) * stride] = color(255, 0, 0);
        pixels[(gb_screen_canvas_x + LCD_WIDTH - 1) + (gb_screen_canvas_y + i) * stride] = color(255, 0, 0);
    }

    return true;
}
