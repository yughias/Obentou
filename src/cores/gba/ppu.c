#include "types.h"


#include "SDL_MAINLOOP.h"

#include "cores/gba/ppu.h"
#include "cores/gba/gba.h"
#include "cores/gba/memory.h"
#include "cores/gba/dma.h"
#include "cores/gba/scheduler.h"

#include "cores/gba/ppu_utils.h"

#define NO_AFFINE_WRAP \
if(offX < 0 || offX >= bg_width) \
    continue; \
if(offY < 0 || offY >= bg_height) \
    continue;


#define AFFINE_WRAP \
if(!wrapping){ \
    NO_AFFINE_WRAP \
} else { \
    offX = wrapValue(offX, bg_width); \
    offY = wrapValue(offY, bg_height); \
}

#define LINE_AFFINE_RENDER(bg_idx, getPixel, isWrap) \
int y = ppu->VCOUNT; \
int aff_idx = bg_idx-2; \
u16* bgp = ppu->BGP; \
i16 dx = bgp[aff_idx*4 + 0]; \
i16 dy = bgp[aff_idx*4 + 2]; \
i16 dmx = bgp[aff_idx*4 + 1]; \
i16 dmy = bgp[aff_idx*4 + 3]; \
i32 startX = geti28(ppu->INTERNAL_BGX[aff_idx]); \
i32 startY = geti28(ppu->INTERNAL_BGY[aff_idx]); \
for(int x = 0; x < SCREEN_WIDTH; x++){ \
    i32 offX = startX; \
    i32 offY = startY; \
    startX = geti28(startX + dx); \
    startY = geti28(startY + dy); \
    if(!windowShouldDraw(ppu, x, y, bg_idx, win_mask[x])) \
        continue; \
    offX >>= 8; \
    offY >>= 8; \
    isWrap \
    getPixel \
    int out_col = applyColorEffect(ppu, bg_col, bg_idx, blend_info[x], pixels[x + y * SCREEN_WIDTH], win_mask[x]); \
    pixels[x + y * SCREEN_WIDTH] = out_col; \
    blend_info[x] = bg_idx; \
} \
ppu->INTERNAL_BGX[aff_idx] = geti28(ppu->INTERNAL_BGX[aff_idx] + dmx); \
ppu->INTERNAL_BGY[aff_idx] = geti28(ppu->INTERNAL_BGY[aff_idx] + dmy); \

typedef enum {NO_BG = 0, TILED, AFFINE, BITMAP_3, BITMAP_4, BITMAP_5} bgType;
typedef enum {IN_WIN0 = 0, IN_WIN1 = 1, IN_WINOUT = 2, IN_OBJWIN = 3, WIN_DISABLED = 0xFF} winType; 

static void renderLineMode(ppu_t* ppu, bgType* bg_type);
static void renderLine(ppu_t* ppu);

static void renderLineBitmap3(ppu_t* ppu, u8* blend_info, winType* win_mask);
static void renderLineBitmap4(ppu_t* ppu, u8* blend_info, winType* win_mask);
static void renderLineBitmap5(ppu_t* ppu, u8* blend_info, winType* win_mask);

static void renderLineRegBg(ppu_t* ppu, int bg_idx, u8* blend_info, winType* win_mask);
static void renderLineAffBg(ppu_t* ppu, int bg_idx, u8* blend_info, winType* win_mask);
static void renderLineSprite(ppu_t* ppu, u16* obj_attr_ptr, u8* blend_info, winType* win_mask);
static void getObjMask(ppu_t* ppu, u16* obj_attr_ptr, winType* win_mask);

static void getWinMaskArray(ppu_t* ppu, winType* win_mask);
static winType getWindowType(ppu_t*, u8 x, u8 y, bool* win_active);
static bool windowShouldDraw(ppu_t* ppu, u8 x, u8 y, int renderType, winType win_type);
static u16 applyColorEffect(ppu_t* ppu, u16 inCol, u8 topType, u8 botType, u16 backCol, winType winType);

void gba_ppu_check_vcount(gba_t* gba){
    ppu_t* ppu = &gba->ppu;
    bool new_isVCount = (ppu->VCOUNT == ((ppu->DISPSTAT >> 8) % 228));

    if((ppu->DISPSTAT & (1 << 5)) && new_isVCount && !ppu->isVCount){
        gba->IF |= 0b100;
        gba_check_interrupts(gba);
    }

    ppu->isVCount = new_isVCount;
}

void gba_ppu_event_start_scanline(gba_t* gba, u32 vcount){
    ppu_t* ppu = &gba->ppu;
    ppu->isHBlank = false;
    ppu->isVBlank = vcount >= 160;
    ppu->VCOUNT = vcount;

    gba_ppu_check_vcount(gba);

    if(vcount == SCREEN_HEIGHT){
        if(ppu->DISPSTAT & (1 << 3)){
            gba->IF |= 0b1;
            gba_check_interrupts(gba);
        }

        renderPixels();
        
        gba_update_vblank_dma(gba);
        for(int i = 0; i < 2; i++){
            ppu->INTERNAL_BGX[i] = (i32)ppu->BGX[i];
            ppu->INTERNAL_BGY[i] = (i32)ppu->BGY[i];
        }
    }

    gba_scheduler_create_add_event_0_args(&gba->scheduler_head, gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE, gba_ppu_event_start_hblank, DRAW_CYCLES);
}

void gba_ppu_event_start_hblank(gba_t* gba, u32 dummy){
    ppu_t* ppu = &gba->ppu;
    ppu->isHBlank = true;

    if(ppu->DISPSTAT & (1 << 4)){
            gba->IF |= 0b10;
            gba_check_interrupts(gba);
    }

    if(ppu->VCOUNT < SCREEN_HEIGHT){
        renderLine(ppu);
        
        int* line = pixels + ppu->VCOUNT * width;
        for (int i = 0; i < width; i++) {
            u8 r = line[i] & 0x1F;
            u8 g = (line[i] >> 5) & 0x1F;
            u8 b = (line[i] >> 10) & 0x1F;
            r = r << 3 | r >> 2;
            g = g << 3 | g >> 2;
            b = b << 3 | b >> 2;
            line[i] = color(r, g, b);
        }

        gba_update_hblank_dma(gba);
    }

    u32 arg = (ppu->VCOUNT + 1) % N_SCANLINES;
    gba_scheduler_create_add_event_1_args(&gba->scheduler_head, gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE, gba_ppu_event_start_scanline, arg, HBLANK_CYCLES);
}
  
static void renderLine(ppu_t* ppu){
    int y = ppu->VCOUNT;

    if(ppu->DISPCNT & (1 << 7)){
        for(int x = 0; x < SCREEN_WIDTH; x++)
            pixels[x + y * SCREEN_WIDTH] = 0xFFFF;
        return;
    }

    bgType bg_type[4];
    switch(ppu->DISPCNT & 0b111){
        case 0:
        bg_type[0] = TILED;
        bg_type[1] = TILED;
        bg_type[2] = TILED;
        bg_type[3] = TILED;
        break;

        case 1:
        bg_type[0] = TILED;
        bg_type[1] = TILED;
        bg_type[2] = AFFINE;
        bg_type[3] = NO_BG;
        break;

        case 2:
        bg_type[0] = NO_BG;
        bg_type[1] = NO_BG;
        bg_type[2] = AFFINE;
        bg_type[3] = AFFINE;
        break;

        case 3:
        bg_type[0] = NO_BG;
        bg_type[1] = NO_BG;
        bg_type[2] = BITMAP_3;
        bg_type[3] = NO_BG;
        break;

        case 4:
        bg_type[0] = NO_BG;
        bg_type[1] = NO_BG;
        bg_type[2] = BITMAP_4;
        bg_type[3] = NO_BG;
        break;

        case 5:
        bg_type[0] = NO_BG;
        bg_type[1] = NO_BG;
        bg_type[2] = BITMAP_5;
        bg_type[3] = NO_BG;
        break;

        default:
        printf("UNSUPPORTED RENDERING %d!\n", ppu->DISPCNT & 0b111);
    }

    renderLineMode(ppu, bg_type);
}

static void renderLineRegBg(ppu_t* ppu, int bg_idx, u8* blend_info, winType* win_mask){
    int y = ppu->VCOUNT;
    u8* VRAM = ppu->VRAM;
    u16 bgcnt = ppu->BGCNT[bg_idx];
    u8 char_base_block = (bgcnt >> 2) & 0b11;
    bool mosaic = (bgcnt >> 6) & 1;
    bool color_bpp = (bgcnt >> 7) & 1;
    u8* screen_base_block = &VRAM[((bgcnt >> 8) & 0b11111) << 11];
    u8 bg_size = (bgcnt >> 0xE);
    int bg_width = 256 * (1 + ((bg_size & 1)));
    int bg_height = 256 * (1 + ((bg_size >> 1)));

    for(int x = 0; x < SCREEN_WIDTH; x++){
        if(!windowShouldDraw(ppu, x, y, bg_idx, win_mask[x]))
            continue;
        int offX = (x + (ppu->BGHOFS[bg_idx] & 0x1FF)) % bg_width;
        int offY = (y + (ppu->BGVOFS[bg_idx] & 0x1FF)) % bg_height;
        int sbx = offX / 256;
        int sby = offY / 256;

        int internal_offX = (offX >> 3) % 32;
        int internal_offY = (offY >> 3) % 32;
        u16* sb_map = (u16*)&screen_base_block[sbx * 0x800 + sby * (bg_width << 3)];

        u16 tileData = sb_map[internal_offX + internal_offY * 32];
        int tileIdx = tileData & 0x3FF;
        bool h_flip = (tileData >> 0xA) & 1;
        bool v_flip = (tileData >> 0xB) & 1;
        u8 paletteIdx = tileData >> 12;
        u8 stride = color_bpp ? 64 : 32;
        u8* tilePtr = &VRAM[(char_base_block << 14) + tileIdx*stride];
        u8 px = offX % 8;
        u8 py = offY % 8;

        bool transparent = false;
        int bg_col = getTilePixel(tilePtr, h_flip ? 7 - px : px, v_flip ? 7 - py : py, ppu->PALETTE_RAM, paletteIdx*32, color_bpp, &transparent);
        if(!transparent){
            u16 out_col = applyColorEffect(ppu, bg_col, bg_idx, blend_info[x], pixels[x + y * SCREEN_WIDTH], win_mask[x]);
            pixels[x + y * SCREEN_WIDTH] = out_col;
            blend_info[x] = bg_idx;
        }
    }
}

static void renderLineAffBg(ppu_t* ppu, int bg_idx, u8* blend_info, winType* win_mask){
    u16 bgcnt = ppu->BGCNT[bg_idx];
    u8 char_base_block = (bgcnt >> 2) & 0b11;
    bool mosaic = (bgcnt >> 6) & 1;
    u8* screen_base_block = &ppu->VRAM[((bgcnt >> 8) & 0b11111) << 11];
    bool wrapping = (bgcnt >> 0xD) & 1;
    u8 bg_size = (bgcnt >> 0xE);
    int bg_width = 128 << bg_size;
    int bg_height = 128 << bg_size;

    LINE_AFFINE_RENDER(
        bg_idx,
        u8 tileIdx = screen_base_block[(offX >> 3) + (offY >> 3) * (bg_width >> 3)];
        u8* tilePtr = &ppu->VRAM[(char_base_block << 14) + tileIdx*64];
        u8 px = offX % 8;
        u8 py = offY % 8;

        bool transparent = false;
        u16 bg_col = getTilePixel(tilePtr, px, py, ppu->PALETTE_RAM, 0, true, &transparent);
        if(transparent)
            continue;
        ,
        AFFINE_WRAP
    );
}

static void renderLineMode(ppu_t* ppu, bgType* bg_type){
    int y = ppu->VCOUNT;
    u16 backdrop_col = getRgb555FromMemory(ppu->PALETTE_RAM, 0);

    u8 blend_info[SCREEN_WIDTH] = { [0 ... 239] = 5 };
    winType win_mask[SCREEN_WIDTH] = {[0 ... 239] = WIN_DISABLED};

    getWinMaskArray(ppu, win_mask);
    
    for(int x = 0; x < SCREEN_WIDTH; x++)
        pixels[x + y * SCREEN_WIDTH] = applyColorEffect(ppu, backdrop_col, 5, 5, backdrop_col, win_mask[x]);

    for(int prio = 3; prio >= 0; prio--){
        for(int i = 3; i >= 0; i--){
            bool bg_on = ppu->DISPCNT & (1 << (8 + i));
            u8 bg_prio = ppu->BGCNT[i] & 0b11;
            if(bg_on && bg_prio == prio)
                switch(bg_type[i]){
                    case NO_BG:
                    break;

                    case TILED:
                    renderLineRegBg(ppu, i, blend_info, win_mask);
                    break;

                    case AFFINE:
                    renderLineAffBg(ppu, i, blend_info, win_mask);
                    break;

                    case BITMAP_3:
                    renderLineBitmap3(ppu, blend_info, win_mask);
                    break;

                    case BITMAP_4:
                    renderLineBitmap4(ppu, blend_info, win_mask);
                    break;

                    case BITMAP_5:
                    renderLineBitmap5(ppu, blend_info, win_mask);
                    break;
                }
        }

        if(!(ppu->DISPCNT & (1 << 12)))
            continue;

        for(int i = 127; i >= 0; i--){
            u16* obj_attr_ptr = (u16*)(&ppu->OAM[i*8]);
            u8 obj_prio = (obj_attr_ptr[2] >> 0xA) & 0b11;
            u16 tid = obj_attr_ptr[2] & 0x3FF;
            if(tid < 512 && bg_type[2] >= BITMAP_3)
                continue;
            if(obj_prio == prio)
                renderLineSprite(ppu, obj_attr_ptr, blend_info, win_mask);
        }
    }
}

static void renderLineBitmap3(ppu_t* ppu, u8* blend_info, winType* win_mask){
    int bg_width = SCREEN_WIDTH;
    int bg_height = SCREEN_HEIGHT;

    LINE_AFFINE_RENDER(
        2,
        u16 bg_col = getRgb555FromMemory(ppu->VRAM, offX + offY * SCREEN_WIDTH);
        ,
        NO_AFFINE_WRAP
    );
}

static void renderLineBitmap4(ppu_t* ppu, u8* blend_info, winType* win_mask){
    int bg_width = SCREEN_WIDTH;
    int bg_height = SCREEN_HEIGHT;
    int bg_offset = ((bool)(ppu->DISPCNT & (1 << 4))) * 0xA000;

    LINE_AFFINE_RENDER(
        2,
        int idx = offX + offY * SCREEN_WIDTH;
        u8 palIdx = ppu->VRAM[bg_offset + idx];
        if(!palIdx)
            continue;
        u16 bg_col = getRgb555FromMemory(ppu->PALETTE_RAM, palIdx);
        ,
        NO_AFFINE_WRAP
    );
}

static void renderLineBitmap5(ppu_t* ppu, u8* blend_info, winType* win_mask){
    int bg_width = 160;
    int bg_height = 128;
    int bg_offset = ((bool)(ppu->DISPCNT & (1 << 4))) * 0xA000;

    LINE_AFFINE_RENDER(
        2,
        int vram_idx = offX + offY * 160;
        u16 bg_col = getRgb555FromMemory(ppu->VRAM + bg_offset, vram_idx);
        ,  
        NO_AFFINE_WRAP
    );
}

static bool windowShouldDraw(ppu_t* ppu, u8 x, u8 y, int renderType, winType win_type){
    if(win_type == WIN_DISABLED)
        return true;

    for(int i = 0; i < 2; i++){
        if(win_type == IN_WIN0 + i)
            return ppu->WININ & (1 << (renderType + 0x8 * i));
    }

    if(win_type == IN_OBJWIN)
        return ppu->WINOUT & (1 << (renderType + 0x8));

    return ppu->WINOUT & (1 << renderType);
}

void gba_ppu_compose_dispstat(ppu_t* ppu){
    ppu->DISPSTAT &= 0xFFFFFFF8;
    if(ppu->VCOUNT >= SCREEN_HEIGHT)
        ppu->DISPSTAT |= 0b1;
    if(ppu->isHBlank)
        ppu->DISPSTAT |= 0b10;
    if(ppu->VCOUNT == (ppu->DISPSTAT >> 8))
        ppu->DISPSTAT |= 0b100;
}

static u16 applyColorEffect(ppu_t* ppu, u16 inCol, u8 topType, u8 botType, u16 backCol, winType winType){
    u16 bldcnt = ppu->BLDCNT;
    if(winType != WIN_DISABLED){
        if(winType <= IN_WIN1)
            if(!(ppu->WININ & (1 << (5 + 8 * winType))))
                return inCol;
        if(winType == IN_OBJWIN && !(ppu->WINOUT & (1 << 0xD)))
            return inCol;
        if(winType == IN_WINOUT && !(ppu->WINOUT & (1 << 5)))
            return inCol;
    }

    u8 blend_mode = (bldcnt >> 6) & 0b11;
    switch(blend_mode){ 
        case 0b00:
        return inCol;
        break;

        case 0b01:
        return isBldTop(bldcnt, topType) && isBldBot(bldcnt, botType) ? colorBlend(ppu->BLDALPHA, inCol, backCol) : inCol;
        break;

        case 0b10:
        return isBldTop(bldcnt, topType) ? brightnessBlend(ppu->BLDY, inCol, 1) : inCol;
        break;

        case 0b11:
        return isBldTop(bldcnt, topType) ? brightnessBlend(ppu->BLDY, inCol, -1) : inCol;
    }
}

static void renderLineSprite(ppu_t* ppu, u16* obj_attr_ptr, u8* blend_info, winType* win_mask){
    int y = ppu->VCOUNT;
    bool mapping_1d = ppu->DISPCNT & (1 << 6);
    u8* VRAM = ppu->VRAM;
    u8* PALETTE_RAM = ppu->PALETTE_RAM;
    u8* OAM = ppu->OAM;

    u8 mode = (obj_attr_ptr[0] >> 8) & 0b11;
    if(mode == 0b10)
        return;

    u8 gfx_mode = (obj_attr_ptr[0] >> 0xA) & 0b11;
    if(gfx_mode >= 0b10)
        return;

    bool is_8bpp = (obj_attr_ptr[0] >> 0xD) & 1;
    bool h_flip;
    bool v_flip;
    int sprite_width;
    int sprite_height;
    int sprite_x;
    int sprite_y;
    int signed_sprite_x;
    int signed_sprite_y;
    int render_limit_width;
    int render_limit_height;
    u8 stride = is_8bpp ? 64 : 32;
    bool alpha_blending = ((obj_attr_ptr[0] >> 0xA) & 0b11) == 0b01;
    i16 aff_mat[4];
    getSpriteDim(obj_attr_ptr, &sprite_width, &sprite_height);
    getSpritePos(obj_attr_ptr, &sprite_x, &sprite_y);
    signed_sprite_x = sprite_x & 0x100 ? 0xFFFFFE00 | sprite_x : sprite_x;
    render_limit_width = sprite_width;
    render_limit_height = sprite_height;

    if(mode == 0b00){
        h_flip = (obj_attr_ptr[1] >> 0xC) & 1;
        v_flip = (obj_attr_ptr[1] >> 0xD) & 1;
    } else {
        u8 mat_idx = (obj_attr_ptr[1] >> 0x9) & 0x1F;
        aff_mat[0] = *(i16*)(&OAM[mat_idx*0x20 + 0x06]);
        aff_mat[1] = *(i16*)(&OAM[mat_idx*0x20 + 0x0E]);
        aff_mat[2] = *(i16*)(&OAM[mat_idx*0x20 + 0x16]);
        aff_mat[3] = *(i16*)(&OAM[mat_idx*0x20 + 0x1E]);
        if(mode == 0b11){
            render_limit_width <<= 1;
            render_limit_height <<= 1;
        }
    }

    int y_min = sprite_y & 255;
    int y_max = (y_min + render_limit_height) & 255;
    if(y_max < y_min)
        y_min -= 256;

    if(y < y_min || y >= y_max)
        return;

    u16 tid = obj_attr_ptr[2] & 0x3FF;
    u8 palBank = obj_attr_ptr[2] >> 0xC;

    for(int x = 0; x < render_limit_width; x++){
        int actual_sprite_x = signed_sprite_x + x;
        u8* blend_info_ptr = &blend_info[actual_sprite_x];
        if(actual_sprite_x < 0 || actual_sprite_x >= SCREEN_WIDTH)
            continue;
        if(!windowShouldDraw(ppu, actual_sprite_x, y, 4, win_mask[actual_sprite_x]))
            continue;
        bool transparent;
        u8 px;
        u8 py;
        u8 offX;
        u8 offY;
        if(mode == 0b00){
            px = h_flip ? (sprite_width - 1 - x) % 8 : x % 8;
            py = v_flip ? (sprite_height - 1 - (y - y_min)) % 8 : (y - y_min) % 8;
            offX = h_flip ? (sprite_width - 1 - x) >> 3 : x >> 3;
            offY = v_flip ? (sprite_height - 1 - (y - y_min)) >> 3 : (y - y_min) >> 3;
        } else {
            i32 center_x = (sprite_width << 7);
            i32 center_y = (sprite_height << 7);
            i32 aff_x = (x << 8) - center_x;
            i32 aff_y = ((y - y_min) << 8) - center_y;
            if(mode == 0b11){
                aff_x -= sprite_width << 7;
                aff_y -= sprite_height << 7;
            }
            applyRotScaleMat(aff_mat, &aff_x, &aff_y);
            aff_x += center_x;
            aff_y += center_y;
            aff_x >>= 8;
            aff_y >>= 8;
            if(aff_x < 0 || aff_x >= sprite_width)
                continue;
            if(aff_y < 0 || aff_y >= sprite_height)
                continue;
            offX = aff_x >> 3;
            offY = aff_y >> 3;
            px = aff_x % 8;
            py = aff_y % 8;
        }
        u8* tilePtr = &VRAM[0x10000 +  tid*32];
        if(mapping_1d)
            tilePtr += (offX + offY * (sprite_width >> 3)) * stride;
        else
            tilePtr += (offX * stride + offY * 32 * 32);
        int sprite_col = getTilePixel(tilePtr, px, py, &PALETTE_RAM[512], palBank*32, is_8bpp, &transparent);
        if(!transparent) {
            u16 out_col;
            if(gfx_mode == 0b01 && isBldBot(ppu->BLDCNT, blend_info[actual_sprite_x]))
                out_col = colorBlend(ppu->BLDALPHA, sprite_col, pixels[actual_sprite_x + y * SCREEN_WIDTH]);
            else 
                out_col = applyColorEffect(ppu, sprite_col, 4, blend_info[actual_sprite_x], pixels[actual_sprite_x + y * SCREEN_WIDTH], win_mask[actual_sprite_x]);
            pixels[actual_sprite_x + y * SCREEN_WIDTH] = out_col;
            blend_info[actual_sprite_x] = 4;
        }
    }
}

static void getObjMask(ppu_t* ppu, u16* obj_attr_ptr, winType* win_mask){
    int y = ppu->VCOUNT;
    bool mapping_1d = ppu->DISPCNT & (1 << 6);
    u8* VRAM = ppu->VRAM;
    u8* PALETTE_RAM = ppu->PALETTE_RAM;
    u8* OAM = ppu->OAM;

    u8 mode = (obj_attr_ptr[0] >> 8) & 0b11;
    if(mode == 0b10)
        return;

    u8 gfx_mode = (obj_attr_ptr[0] >> 0xA) & 0b11;
    if(gfx_mode != 0b10)
        return;

    bool is_8bpp = (obj_attr_ptr[0] >> 0xD) & 1;
    bool h_flip;
    bool v_flip;
    int sprite_width;
    int sprite_height;
    int sprite_x;
    int sprite_y;
    int signed_sprite_x;
    int signed_sprite_y;
    int render_limit_width;
    int render_limit_height;
    u8 stride = is_8bpp ? 64 : 32;
    bool alpha_blending = ((obj_attr_ptr[0] >> 0xA) & 0b11) == 0b01;
    i16 aff_mat[4];
    getSpriteDim(obj_attr_ptr, &sprite_width, &sprite_height);
    getSpritePos(obj_attr_ptr, &sprite_x, &sprite_y);
    signed_sprite_x = sprite_x & 0x100 ? 0xFFFFFE00 | sprite_x : sprite_x;
    render_limit_width = sprite_width;
    render_limit_height = sprite_height;

    if(mode == 0b00){
        h_flip = (obj_attr_ptr[1] >> 0xC) & 1;
        v_flip = (obj_attr_ptr[1] >> 0xD) & 1;
    } else {
        u8 mat_idx = (obj_attr_ptr[1] >> 0x9) & 0x1F;
        aff_mat[0] = *(i16*)(&OAM[mat_idx*0x20 + 0x06]);
        aff_mat[1] = *(i16*)(&OAM[mat_idx*0x20 + 0x0E]);
        aff_mat[2] = *(i16*)(&OAM[mat_idx*0x20 + 0x16]);
        aff_mat[3] = *(i16*)(&OAM[mat_idx*0x20 + 0x1E]);
        if(mode == 0b11){
            render_limit_width <<= 1;
            render_limit_height <<= 1;
        }
    }

    int y_min = sprite_y & 255;
    int y_max = (y_min + render_limit_height) & 255;
    if(y_max < y_min)
        y_min -= 256;

    if(y < y_min || y >= y_max)
        return;

    u16 tid = obj_attr_ptr[2] & 0x3FF;
    u8 palBank = obj_attr_ptr[2] >> 0xC;

    for(int x = 0; x < render_limit_width; x++){
        int actual_sprite_x = signed_sprite_x + x;
        if(win_mask[actual_sprite_x] != IN_WINOUT)
            continue;
        if(actual_sprite_x < 0 || actual_sprite_x >= SCREEN_WIDTH)
            continue;
        bool transparent;
        u8 px;
        u8 py;
        u8 offX;
        u8 offY;
        if(mode == 0b00){
            px = h_flip ? (sprite_width - 1 - x) % 8 : x % 8;
            py = v_flip ? (sprite_height - 1 - (y - y_min)) % 8 : (y - y_min) % 8;
            offX = h_flip ? (sprite_width - 1 - x) >> 3 : x >> 3;
            offY = v_flip ? (sprite_height - 1 - (y - y_min)) >> 3 : (y - y_min) >> 3;
        } else {
            i32 center_x = (sprite_width << 7);
            i32 center_y = (sprite_height << 7);
            i32 aff_x = (x << 8) - center_x;
            i32 aff_y = ((y - y_min) << 8) - center_y;
            if(mode == 0b11){
                aff_x -= sprite_width << 7;
                aff_y -= sprite_height << 7;
            }
            applyRotScaleMat(aff_mat, &aff_x, &aff_y);
            aff_x += center_x;
            aff_y += center_y;
            aff_x >>= 8;
            aff_y >>= 8;
            if(aff_x < 0 || aff_x >= sprite_width)
                continue;
            if(aff_y < 0 || aff_y >= sprite_height)
                continue;
            offX = aff_x >> 3;
            offY = aff_y >> 3;
            px = aff_x % 8;
            py = aff_y % 8;
        }
        u8* tilePtr = &VRAM[0x10000 +  tid*32];
        if(mapping_1d)
            tilePtr += (offX + offY * (sprite_width >> 3)) * stride;
        else
            tilePtr += (offX * stride + offY * 32 * 32);
        getTilePixel(tilePtr, px, py, &PALETTE_RAM[512], palBank*32, is_8bpp, &transparent);
        if(!transparent)
            win_mask[actual_sprite_x] = IN_OBJWIN;
    }
}

static void getWinMaskArray(ppu_t* ppu, winType* win_mask){
    bool win_active[2];
    bool obj_win_active = (ppu->DISPCNT >> 15) & 1;
    win_active[0] = (ppu->DISPCNT >> 13) & 1;
    win_active[1] = (ppu->DISPCNT >> 14) & 1;

    if(!win_active[0] && !win_active[1] && !obj_win_active)
        return;

    for(int x = 0;  x < SCREEN_WIDTH; x++){
        win_mask[x] = getWindowType(ppu, x, ppu->VCOUNT, win_active);        
    }

    if(obj_win_active){
        for(int i = 0; i < 128; i++){
            u16* obj_attr_ptr = (u16*)(&ppu->OAM[i*8]);
            getObjMask(ppu, obj_attr_ptr, win_mask);
        }
    }
}

static winType getWindowType(ppu_t* ppu, u8 x, u8 y, bool* win_active){
     for(int i = 0; i < 2; i++){
        if(win_active[i] && isInsideWindow(x, y, ppu->WINH[i], ppu->WINV[i])){
            return IN_WIN0 + i; 
        }
    }

    return IN_WINOUT;
}