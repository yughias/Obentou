#ifndef __PPU_H__
#define __PPU_H__

#include "types.h"

#define LCD_WIDTH 160
#define LCD_HEIGHT 144
#define SCANLINE_NUMBER 154
#define SCANLINE_CYCLE 456

#define LY_ADDR 0xFF44
#define LYC_ADDR 0xFF45
#define LCDC_ADDR 0xFF40
#define STAT_ADDR 0xFF41
#define SCX_ADDR 0xFF43
#define SCY_ADDR 0xFF42
#define BGP_ADDR 0xFF47
#define OBP0_ADDR 0xFF48
#define OBP1_ADDR 0xFF49
#define WX_ADDR 0xFF4B
#define WY_ADDR 0xFF4A

#define DMG_LCD_ENABLE_MASK            0b10000000
#define DMG_WIN_TILE_MAP_AREA_MASK     0b01000000
#define DMG_WIN_ENABLE_MASK            0b00100000
#define DMG_BG_WIN_TILE_DATA_AREA_MASK 0b00010000
#define DMG_BG_TILE_MAP_AREA_MASK      0b00001000
#define DMG_OBJ_SIZE_MASK              0b00000100
#define DMG_OBJ_ENABLE_MASK            0b00000010
#define DMG_BG_WIN_ENABLE_MASK         0b00000001

#define BG_ATTRIB_PALETTE_MASK         0b00000111
#define BG_ATTRIB_BANK_MASK            0b00001000
#define BG_ATTRIB_FLIP_X_MASK          0b00100000
#define BG_ATTRIB_FLIP_Y_MASK          0b01000000
#define BG_ATTRIB_PRIORITY_MASK        0b10000000

#define MEGADUCK_LCD_ENABLE_MASK            0b10000000
#define MEGADUCK_WIN_TILE_MAP_AREA_MASK     0b00001000
#define MEGADUCK_WIN_ENABLE_MASK            0b00100000
#define MEGADUCK_BG_WIN_TILE_DATA_AREA_MASK 0b00010000
#define MEGADUCK_BG_TILE_MAP_AREA_MASK      0b00000100
#define MEGADUCK_OBJ_SIZE_MASK              0b00000010
#define MEGADUCK_OBJ_ENABLE_MASK            0b00000001
#define MEGADUCK_BG_WIN_ENABLE_MASK         0b01000000

typedef struct gb_t gb_t;

typedef enum {HBLANK_MODE = 0, VBLANK_MODE, OAM_SCAN_MODE, DRAW_MODE} PPU_MODE;

typedef struct ppu_t {
    u8 LCD_ENABLE_MASK;            
    u8 WIN_TILE_MAP_AREA_MASK;   
    u8 WIN_ENABLE_MASK;    
    u8 BG_WIN_TILE_DATA_AREA_MASK;
    u8 BG_TILE_MAP_AREA_MASK;
    u8 OBJ_SIZE_MASK;     
    u8 OBJ_ENABLE_MASK;            
    u8 BG_WIN_ENABLE_MASK;         

    u8 LY_REG;
    u8 LYC_REG;
    u8 LCDC_REG;
    u8 STAT_REG;
    u8 SCX_REG;
    u8 SCY_REG;
    u8 BGP_REG;
    u8 OBP0_REG;
    u8 OBP1_REG;
    u8 WX_REG;
    u8 WY_REG;

    u8 BCPS_REG;
    u8 OCPS_REG;

    PPU_MODE mode;
    bool lyc_compare;
    size_t counter;
    bool stat_irq;
    u8 internal_ly;

    int dmgColors[4];
    int backgroundColor;
    u8 windowY_counter;
    bool frameSkip;
    bool lastFrameOn;
} ppu_t;

void gb_initColorPalette(gb_t*);
void gb_copyDefaultCgbPalette(gb_t*);
void gb_drawBgRamAt(gb_t*, int, int);
void gb_drawWinRamAt(gb_t*, int, int);
void gb_drawOAMAt(gb_t*, int, int, u8);
void gb_drawColorAt(int, int, int, int, u8*);

void gb_renderLcdOff(ppu_t*);
void gb_initLcdcMasks(gb_t*);
void gb_updatePPU(gb_t*);
u8 gb_getStatRegister(ppu_t*);
void gb_writeColorToCRAM(u8, u8, u8, u8*, u8);

#endif