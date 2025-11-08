#include "cores/gbc/gb.h"

#include "SDL_MAINLOOP.h"

#include <stdlib.h>

static u8* getTileMap(gb_t*, bool);
static u8* getTileData(gb_t*, u8);
static int getSpriteRealX(gb_t*, u8);
static int getSpriteRealY(gb_t*, u8);
static void renderLine(gb_t*, u8);

static int getColorFromTileDataRGB(gb_t*, u8*, u8, u8, u8);
static int convertDMG2RGB(gb_t*, u8, u8, u8*);
static int convertCGB2RGB(gb_t*, u8, u8, u8*);
static u8 getColorGb(u8* tilePtr, u8 tilePX, u8 tilePY);
static int getTileMapPixelRGB(gb_t*, u8* tileMapPtr, u8 x, u8 y, bool* dmgPrio, bool* cgbPrio);
static int getSpritePixelRGB(gb_t*, u8* tilePtr, u8 x, u8 y, bool obp_n, u8 palette, bool flipX, bool flipY, bool bigSprite, bool* transparent);
static void getSpriteAttribute(gb_t*, u8* spriteData, bool* flipX, bool* flipY, bool* backgroundOver, bool* obp_n, u8* palette, u8** tilePtr);
static int CgbToRgb(u8, u8);

static u8 colorCorrection[32] = {
    0, 62, 64, 89, 90, 109, 111, 127, 128, 142, 143, 156, 156, 168, 169, 180, 181, 191, 192, 201, 202, 211, 212, 221, 221, 230, 230, 238, 239, 247, 247, 255
};

#define SET_LCDC_MASKS(name) \
ppu->LCD_ENABLE_MASK = name ## _LCD_ENABLE_MASK; \
ppu->WIN_TILE_MAP_AREA_MASK = name ## _WIN_TILE_MAP_AREA_MASK; \
ppu->WIN_ENABLE_MASK = name ## _WIN_ENABLE_MASK; \
ppu->BG_WIN_TILE_DATA_AREA_MASK = name ## _BG_WIN_TILE_DATA_AREA_MASK; \
ppu->BG_TILE_MAP_AREA_MASK = name ## _BG_TILE_MAP_AREA_MASK; \
ppu->OBJ_SIZE_MASK = name ## _OBJ_SIZE_MASK; \
ppu->OBJ_ENABLE_MASK = name ## _OBJ_ENABLE_MASK; \
ppu->BG_WIN_ENABLE_MASK = name ## _BG_WIN_ENABLE_MASK

typedef struct {
    u8 idx;
    int x;
    int y;
} sprite_info;

static int sprite_order_compare(const void* a, const void* b){
    sprite_info* sprite_a = (sprite_info*)a;
    sprite_info* sprite_b = (sprite_info*)b;
    
    if(sprite_a->x != sprite_b->x)
        return sprite_b->x - sprite_a->x;
    else
        return sprite_b->idx - sprite_a->idx;
}

void gb_initLcdcMasks(gb_t* gb){
    ppu_t* ppu = &gb->ppu;
    if(gb->console_type == MEGADUCK_TYPE){
        SET_LCDC_MASKS(MEGADUCK);
        return;
    }

    SET_LCDC_MASKS(DMG);
}

static int CgbToRgb(u8 lo_byte, u8 hi_byte){
    int out_r, out_g, out_b;
    u8 red = lo_byte & 0x1F;
    u8 green = (lo_byte >> 5) | ((hi_byte & 0b11) << 3);
    u8 blue = (hi_byte >> 2) & 0x1F;

    red = colorCorrection[red];
    green = colorCorrection[green];
    blue = colorCorrection[blue];

    out_r = (13*red + 2*green + 1*blue) / 16;
    out_g = (3*green + 1*blue) / 4;
    out_b = (2*green + 14*blue) / 16;

    return color(out_r, out_g, out_b);
}

void gb_writeColorToCRAM(u8 red, u8 green, u8 blue, u8* cram, u8 idx){
    red >>= 3;
    green >>= 3;
    blue >>= 3;
    cram[idx*2] = red | (green << 5);
    cram[idx*2+1] = (green >> 3) | (blue << 2);
}

void gb_initColorPalette(gb_t* gb){
    ppu_t* ppu = &gb->ppu;
    ppu->dmgColors[3] = color(46, 65, 57);
    ppu->dmgColors[2] = color(64, 89, 74);
    ppu->dmgColors[1] = color(95, 120, 66);
    ppu->dmgColors[0] = color(123, 129, 17);

    if(gb->console_type == CGB_TYPE || gb->console_type == DMG_ON_CGB_TYPE)
        ppu->backgroundColor = color(255, 255, 255);
    else
        ppu->backgroundColor = ppu->dmgColors[0];
}

void gb_copyDefaultCgbPalette(gb_t* gb){
    const u8 default_bgp_cram[CRAM_SIZE] = {
        0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F,
        0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F,
        0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F,
        0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F,
        0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F,
        0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F,
        0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F,
        0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F
    };
    
    const u8 default_obp_cram[CRAM_SIZE] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    memcpy(gb->BGP_CRAM, default_bgp_cram, CRAM_SIZE);
    memcpy(gb->OBP_CRAM, default_obp_cram, CRAM_SIZE);
}

void gb_drawBgRamAt(gb_t* gb, int screenX, int screenY){
    ppu_t* ppu = &gb->ppu;
    u8* bgTileMap = getTileMap(gb, ppu->LCDC_REG & ppu->BG_TILE_MAP_AREA_MASK);

    for(int y = 0; y < 256; y++){
        for(int x = 0; x < 256; x++){
            bool priority;
            int col = getTileMapPixelRGB(gb, bgTileMap, x, y, &priority, &priority);
            pixels[screenX+x + (screenY+y)*width] = col;
        }
    }

    for(int y = 0; y < LCD_HEIGHT; y++){
        int offX1 = screenX + (ppu->SCX_REG % 256);
        int offX2 = screenX + ((ppu->SCX_REG + LCD_WIDTH) % 256);
        int offY = screenY + ((ppu->SCY_REG + y) % 256);
        pixels[offX1 + offY * width] = color(255, 0, 0);
        pixels[offX2 + offY * width] = color(255, 0, 0);
    }

    for(int x = 0; x < LCD_WIDTH; x++){
        int offX = screenX + ((ppu->SCX_REG + x) % 256);
        int offY1 = screenY + (ppu->SCY_REG % 256);
        int offY2 = screenY + ((ppu->SCY_REG + LCD_HEIGHT) % 256);
        pixels[offX + offY1 * width] = color(255, 0, 0);
        pixels[offX + offY2 * width] = color(255, 0, 0);
    }
}

void gb_drawWinRamAt(gb_t* gb, int screenX, int screenY){
    ppu_t* ppu = &gb->ppu;
    u8* winTileMap = getTileMap(gb, ppu->LCDC_REG & ppu->WIN_TILE_MAP_AREA_MASK);

    for(int y = 0; y < 256; y++){
        for(int x = 0; x < 256; x++){
            bool priority;
            int col = getTileMapPixelRGB(gb, winTileMap, x, y, &priority, &priority);
            pixels[screenX+x + (screenY+y)*width] = col;
        }
    }
}

void gb_drawColorAt(int x, int y, int palette, int pal_color, u8* cram){
    u8 lo_byte = cram[palette*8 + pal_color*2];
    u8 hi_byte = cram[palette*8 + pal_color*2 + 1];
    int col = CgbToRgb(lo_byte, hi_byte);
    rect(x, y, 8, 8, col);
}

static u8* getTileMap(gb_t* gb, bool addressMode){
    return addressMode ? gb->VRAM + 0x1C00 : gb->VRAM + 0x1800;
}

static u8* getTileData(gb_t* gb, u8 tileIdx){
    ppu_t* ppu = &gb->ppu;
    if(ppu->LCDC_REG & ppu->BG_WIN_TILE_DATA_AREA_MASK){
        return gb->VRAM + (tileIdx*16);
    } else {
        return gb->VRAM + 0x1000 + ((int8_t)tileIdx)*16;
    }
}

static int getColorFromTileDataRGB(gb_t* gb, u8* tilePtr, u8 tilePX, u8 tilePY, u8 colorREG){
    u8 colorGB = getColorGb(tilePtr, tilePX, tilePY);

    if(gb->console_type == CGB_TYPE)
        return convertCGB2RGB(gb, colorGB, colorREG, gb->BGP_CRAM);
    else
        return convertDMG2RGB(gb, colorGB, colorREG, gb->BGP_CRAM);
}

static u8 getColorGb(u8* tilePtr, u8 tilePX, u8 tilePY){
    u8 colorGB = 0;
    colorGB = !!(tilePtr[tilePY*2] & (1 << (7 - tilePX)));
    colorGB |= !!(tilePtr[1+tilePY*2] & (1 << (7 - tilePX))) << 1;

    return colorGB;
}

static int convertCGB2RGB(gb_t* gb, u8 colorGB, u8 colorREG, u8* cram_palette){
    cram_palette += colorREG*8;
    return CgbToRgb(cram_palette[colorGB*2], cram_palette[colorGB*2+1]);
}

static int convertDMG2RGB(gb_t* gb, u8 colorGB, u8 colorREG, u8* cram_palette){
    ppu_t* ppu = &gb->ppu;
    u8 idx = (colorREG >> (2 * colorGB)) & 0b11;
    if(gb->console_type == DMG_ON_CGB_TYPE)
        return CgbToRgb(cram_palette[idx*2], cram_palette[idx*2+1]);
    else
        return ppu->dmgColors[idx];
}

static int getTileMapPixelRGB(gb_t* gb, u8* tileMapPtr, u8 x, u8 y, bool* dmgPrio, bool* cgbPrio){
    ppu_t* ppu = &gb->ppu;
    u8 byteX = x >> 3;
    u8 byteY = y >> 3;
    u8 tilePX = x & 0b111;
    u8 tilePY = y & 0b111;
    u8 tileIdx = tileMapPtr[byteX+byteY*32];
    u8* tilePtr = getTileData(gb, tileIdx);
    u8 paletteReg = ppu->BGP_REG;
    u8 bg_attributes;

    if(gb->console_type == CGB_TYPE){
        bg_attributes = tileMapPtr[byteX+byteY*32 + 0x2000];
        if(bg_attributes & BG_ATTRIB_BANK_MASK)
            tilePtr += 0x2000;
        if(bg_attributes & BG_ATTRIB_FLIP_X_MASK)
            tilePX = 7 - tilePX;
        if(bg_attributes & BG_ATTRIB_FLIP_Y_MASK)
            tilePY = 7 - tilePY;
        paletteReg = bg_attributes & BG_ATTRIB_PALETTE_MASK;
    }

    u8 colorGB = getColorGb(tilePtr, tilePX, tilePY);
    *dmgPrio |= colorGB;
    if(gb->console_type == CGB_TYPE)
        *cgbPrio |= (bg_attributes & BG_ATTRIB_PRIORITY_MASK) && colorGB;

    return getColorFromTileDataRGB(gb, tilePtr, tilePX, tilePY, paletteReg);
}

static void renderLine(gb_t* gb, u8 y){
    ppu_t* ppu = &gb->ppu;
    int col;
    u8* bgTileMap = getTileMap(gb, ppu->LCDC_REG & ppu->BG_TILE_MAP_AREA_MASK);
    u8* winTileMap = getTileMap(gb, ppu->LCDC_REG & ppu->WIN_TILE_MAP_AREA_MASK);

    bool bg_win_enabled = ppu->LCDC_REG & ppu->BG_WIN_ENABLE_MASK;

    bool cgbPrio[LCD_WIDTH] = {false};
    bool dmgPrio[LCD_WIDTH] = {false};
    
    for(u8 x = 0; x < LCD_WIDTH; x++){
        if(gb->console_type != CGB_TYPE && !bg_win_enabled)
            col = ppu->backgroundColor;
        else
            col = getTileMapPixelRGB(gb, bgTileMap, (ppu->SCX_REG + x) % 256, (ppu->SCY_REG + y) % 256, &dmgPrio[x], &cgbPrio[x]);
        
        pixels[x + y * LCD_WIDTH] = col;
    }

    if(gb->console_type == MEGADUCK_TYPE)
        if((ppu->LCDC_REG & (ppu->BG_WIN_TILE_DATA_AREA_MASK | ppu->WIN_TILE_MAP_AREA_MASK | ppu->BG_TILE_MAP_AREA_MASK)) == 0b01100)
            return;

    if((ppu->LCDC_REG & ppu->WIN_ENABLE_MASK) && (ppu->LCDC_REG & ppu->BG_WIN_ENABLE_MASK)){
        int winX = ppu->WX_REG - 7;
        if(y >= ppu->WY_REG){
            if(winX < LCD_WIDTH){
                for(int offX = 0; winX < LCD_WIDTH; offX = (offX + 1) % 256){
                    if(winX >= 0)
                        pixels[winX + y * LCD_WIDTH] = getTileMapPixelRGB(gb, winTileMap, offX % 256, ppu->windowY_counter % 256, &dmgPrio[winX], &cgbPrio[winX]);
                    winX++;
                }
                ppu->windowY_counter++;
            }
        }
    }

    if(!(ppu->LCDC_REG & ppu->OBJ_ENABLE_MASK))
        return;

    sprite_info sprites[10];
    u8 drawnSprites = 0;
    bool bigSprite = ppu->LCDC_REG & ppu->OBJ_SIZE_MASK;
    for(int i = 0; i < 40 && drawnSprites < 10; i++){
        // these values can be negative!
        int spriteTopY = getSpriteRealY(gb, i);
        if(y >= spriteTopY && ((y < spriteTopY + 8) || (y < spriteTopY + 16 && bigSprite))){
            int spriteTopX = getSpriteRealX(gb, i);
            sprites[drawnSprites].idx = i;
            sprites[drawnSprites].x = spriteTopX;
            sprites[drawnSprites].y = spriteTopY;
            drawnSprites++;
        }
    }

    int startVal;
    int step;
    int limit;

    if(gb->console_type != CGB_TYPE){
        startVal = 0;
        limit = drawnSprites;
        step = 1;
        qsort(sprites, drawnSprites, sizeof(sprite_info), sprite_order_compare);
    } else {
        startVal = drawnSprites - 1;
        limit = -1;
        step = -1;
    }

    for(int i = startVal; i != limit; i += step){
        int spriteTopX = sprites[i].x;
        int spriteTopY = sprites[i].y;
        if(spriteTopX < 168){
            int screenX = spriteTopX;
            int spriteX = 0;
            int spriteY = y - spriteTopY;
            if(screenX < 0){
                screenX = 0;
                spriteX = -spriteTopX;
            }
            u8* spriteData = &gb->OAM[sprites[i].idx*4];
            while(screenX < LCD_WIDTH && spriteX < 8){
                bool flipX, flipY;
                bool backgroundOver;
                bool obp_n;
                u8 palette;
                u8* tilePtr;

                getSpriteAttribute(gb, spriteData, &flipX, &flipY, &backgroundOver, &obp_n, &palette, &tilePtr);

                bool transparent;
                col = getSpritePixelRGB(gb, tilePtr, spriteX, spriteY, obp_n, palette, flipX, flipY, bigSprite, &transparent);
                if(!transparent)
                    if(
                        // basic dmg background priority condition
                        (!backgroundOver || (backgroundOver && !dmgPrio[screenX])) &&
                        // cgb priority condition
                        (gb->console_type != CGB_TYPE || !cgbPrio[screenX] || !bg_win_enabled)
                    )
                        pixels[screenX + y * LCD_WIDTH] = col;

                screenX++;
                spriteX++;
            }
        }
    }
}

static void getSpriteAttribute(gb_t* gb, u8* spriteData, bool* flipX, bool* flipY, bool* backgroundOver, bool* obp_n, u8* palette, u8** tilePtr){
    ppu_t* ppu = &gb->ppu;
    *flipX = spriteData[3] & 0b100000;
    *flipY = spriteData[3] & 0b1000000;
    *backgroundOver = spriteData[3] & 0b10000000;
    *obp_n = spriteData[3] & 0b10000;
    if(gb->console_type == CGB_TYPE)
        *palette = spriteData[3] & 0b111;
    else
        *palette = *obp_n ? ppu->OBP1_REG : ppu->OBP0_REG; 
    u8 tileIdx = spriteData[2];
    if(ppu->LCDC_REG & ppu->OBJ_SIZE_MASK)
        tileIdx &= 0b11111110;
    *tilePtr = gb->VRAM + tileIdx*16;
    if(gb->console_type == CGB_TYPE && (spriteData[3] & 0b1000))
        *tilePtr += 0x2000;
}

static int getSpritePixelRGB(gb_t* gb, u8* tilePtr, u8 x, u8 y, bool obp_n, u8 palette, bool flipX, bool flipY, bool bigSprite, bool* transparent){
    if(flipX)
        x = 7 - x;
    
    if(flipY){
        if(bigSprite)
            y = 15 - y;
        else
            y = 7 - y;
    }

    u8 colorGB = 0;
    colorGB = !!(tilePtr[y*2] & (1 << (7 - x)));
    colorGB |= !!(tilePtr[1+y*2] & (1 << (7 - x))) << 1;

    if(colorGB == 0){
        *transparent = true;
        return -1;
    } else {
        *transparent = false;
    }

    if(gb->console_type == CGB_TYPE)
        return convertCGB2RGB(gb, colorGB, palette, gb->OBP_CRAM);
    else
        return convertDMG2RGB(gb, colorGB, palette, gb->OBP_CRAM);
}

void gb_drawOAMAt(gb_t* gb, int screenX, int screenY, u8 spriteIdx){
    ppu_t* ppu = &gb->ppu;
    u8* spriteData = &gb->OAM[spriteIdx * 4];
    
    bool flipX, flipY;
    bool backgroundOver;
    bool obp_n;
    u8 palette;
    u8* tilePtr;

    getSpriteAttribute(gb, spriteData, &flipX, &flipY, &backgroundOver, &obp_n, &palette, &tilePtr);

    u8 height = ppu->LCDC_REG & ppu->OBJ_SIZE_MASK ? 16 : 8;
    for(u8 y = 0; y < height; y++)
        for(u8 x = 0; x < 8; x++){
            bool transparent;
            pixels[(screenX + x) + (screenY + y)*width] = getSpritePixelRGB(gb, tilePtr, x, y, obp_n, palette, flipX, flipY, height, &transparent);
        }
}

static int getSpriteRealX(gb_t* gb, u8 spriteIdx){
    u8 x = gb->OAM[spriteIdx*4 + 1];
    return x - 8;
}

static int getSpriteRealY(gb_t* gb, u8 spriteIdx){
    u8 y = gb->OAM[spriteIdx*4];
    return y - 16;
}

u8 gb_getStatRegister(ppu_t* ppu){
    u8 output_val;
    output_val = ppu->STAT_REG & 0b11111000;
    output_val |= ppu->mode;
    output_val |= ppu->lyc_compare << 2;
    output_val |= 0b10000000;
    return output_val;
}

void gb_updatePPU(gb_t* gb){
    ppu_t* ppu = &gb->ppu;
    if(!(ppu->LCDC_REG & ppu->LCD_ENABLE_MASK)){
        ppu->LY_REG = 0;
        ppu->internal_ly = 0;
        ppu->mode = 0;
        ppu->counter = 0;
        ppu->windowY_counter = 0;
        ppu->frameSkip = true;

        if(ppu->lastFrameOn)
            gb_renderLcdOff(ppu);

        ppu->lastFrameOn = false;
        return;
    }

    ppu->lastFrameOn = true;
    
    bool old_stat_irq = ppu->stat_irq;
    ppu->stat_irq = false;

    if(ppu->internal_ly < LCD_HEIGHT && ppu->counter == 80 && !ppu->frameSkip)
        renderLine(gb, ppu->LY_REG);

    // search more about exact ppu_counter
    if(ppu->internal_ly == 144 && ppu->counter == 4){
        gb->cpu.IF |= VBLANK_IRQ;

        if(ppu->STAT_REG & (1 << 5))
            ppu->stat_irq = true;

        if(!ppu->frameSkip)
            renderPixels();

        ppu->windowY_counter = 0;
        ppu->frameSkip = false;

        // TODO
        //emulateGameShark(gb);
    }

    // old stat mode timing
    if(ppu->internal_ly < LCD_HEIGHT){
        PPU_MODE old_mode = ppu->mode;

        if(ppu->counter < 80)
            ppu->mode = OAM_SCAN_MODE;
        else if(ppu->counter < 80 + 172)
            ppu->mode = DRAW_MODE;
        else
            ppu->mode = HBLANK_MODE;

        if(gb->console_type == CGB_TYPE)
            if(old_mode != HBLANK_MODE && ppu->mode == HBLANK_MODE && !gb->cpu.HALTED)
                gb_stepHDMA(gb);
    } else {
        ppu->mode = VBLANK_MODE;
    }

    if(ppu->internal_ly == 153 && ppu->counter == 8)
        ppu->LY_REG = 0;

    u8 stat_interruput_source = (ppu->STAT_REG >> 3) & 0b111;
    if((stat_interruput_source & (1 << ppu->mode)))
        ppu->stat_irq = true;

    ppu->lyc_compare = ppu->LY_REG == ppu->LYC_REG;
    if((ppu->lyc_compare == true) && (ppu->STAT_REG & 0x40))
        ppu->stat_irq = true;

    if(!old_stat_irq && ppu->stat_irq)
        gb->cpu.IF |= STAT_IRQ;

    if(ppu->counter == SCANLINE_CYCLE){        
        ppu->internal_ly = (ppu->internal_ly + 1) % SCANLINE_NUMBER;
        ppu->LY_REG = ppu->internal_ly;
        ppu->counter = 0;
    }   else
        ppu->counter++;
}

void gb_renderLcdOff(ppu_t* ppu){
    for(int i = 0; i < width*height; i++)
        pixels[i] = ppu->backgroundColor;
    renderPixels();
}