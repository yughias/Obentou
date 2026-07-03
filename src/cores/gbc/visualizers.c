#include "cores/gbc/gb.h"

#include "SDL_MAINLOOP.h"

bool gb_draw_tilemap(gb_t* gb){
    size(256, 256);

    ppu_t* ppu = &gb->ppu;
    u8* bgTileMap = gb_getTileMap(gb, ppu->LCDC_REG & ppu->BG_TILE_MAP_AREA_MASK);

    for(int y = 0; y < 256; y++){
        for(int x = 0; x < 256; x++){
            bool priority;
            int col = gb_getTileMapPixelRGB(gb, bgTileMap, x, y, &priority, &priority);
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
                    col *= 85;
                    pixels[(tx*8+px) + (ty*8+py) * stride] = color(col, col, col);
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
            pixels[(screenX + x) + (screenY + y)*width] = gb_getSpritePixelRGB(gb, tilePtr, x, y, obp_n, palette, flipX, flipY, height, &transparent);
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
    int n_palette = gb->console_type == CGB_TYPE ? 8 : (gb->console_type == DMG_ON_CGB_TYPE ? 2 : 1);
    size(4, n_palette);

    if(gb->console_type == DMG_TYPE || gb->console_type == MEGADUCK_TYPE){
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
            pixels[x + y*stride] = col;
        }
    }

    return true;
}
