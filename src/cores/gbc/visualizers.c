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
#define Y_ADDR_CUR_MAP        0xD35D
#define Y_ADDR_WALK_COUNTER   0xCFC4
#define Y_ADDR_IS_IN_BATTLE   0xD056
#define Y_ADDR_NUM_SPRITES    0xD4E0
#define Y_ADDR_PIKACHU_OW_FLAGS 0xD42F
#define Y_PIKACHU_HIDE_MASK   ((1 << 5) | (1 << 7))
#define Y_PIKACHU_SPRITE_SLOT 15
#define Y_SPRITE_PICTURE_ID_MAX 0x52
#define Y_ADDR_PLAYER_ANIM_INTRA_FRAME_COUNTER 0xC107
#define Y_ADDR_PLAYER_ANIM_FRAME_COUNTER 0xC108
#define Y_ADDR_PLAYER_FACING_DIR 0xC109
#define Y_ADDR_PLAYER_COLLISION 0xC10C
#define Y_ADDR_PLAYER_X_ADJUSTED 0xC10B
#define Y_ADDR_PLAYER_Y_ADJUSTED 0xC10A



#define YELLOW_MAP_RECURSION_DEPTH 2
#define YELLOW_MAP_BORDER_BLOCKS 3
#define YELLOW_BLOCK_PX 32
// Route 17 and Route 23 are 72 blocks tall. A lower cap drops those headers,
// so the road up to Victory Road is replaced by the border tile.
#define YELLOW_MAX_MAP_BLOCKS 80

#define Y_ROM_MAP_HEADER_BANKS ((0x3F * 0x4000) + (0x43E4 - 0x4000))
#define Y_ROM_MAP_HEADER_PTRS  ((0x3F * 0x4000) + (0x41F2 - 0x4000))
#define Y_CONN_EAST  1
#define Y_CONN_WEST  2
#define Y_CONN_SOUTH 4
#define Y_CONN_NORTH 8
#define Y_CONN_STRUCT_SIZE 11
#define Y_NUM_MAPS 249
#define Y_ADDR_LAST_MAP 0xD364
#define Y_NUM_CITY_MAPS 0x0B
#define Y_FIRST_INDOOR_MAP 0x25
#define Y_TILESET_CEMETERY 15
#define Y_TILESET_CAVERN 17
#define Y_MAP_CERULEAN_CAVE_2F 0xE2
#define Y_MAP_CERULEAN_CAVE_1F 0xE4
#define Y_MAP_TRADE_CENTER 0xEF
#define Y_MAP_COLOSSEUM 0xF0
#define Y_MAP_LORELEI 0xF5
#define Y_MAP_BRUNO 0xF6
#define Y_PAL_GRAYMON 0x19
#define Y_PAL_CAVE 0x23
#define Y_ROM_CGB_BASE_PALS ((0x1C * 0x4000) + (0x6AF9 - 0x4000))
#define Y_NUM_CGB_PALS 0x28
#define Y_OBJ_TRAINER 0x40
#define Y_OBJ_ITEM 0x80
#define Y_MOVE_WALK 0xFE
#define Y_MOVE_STAY 0xFF
#define Y_RANGE_UP_DOWN 0x01
#define Y_RANGE_LEFT_RIGHT 0x02
#define Y_DIR_DOWN 0xD0
#define Y_DIR_UP 0xD1
#define Y_DIR_LEFT 0xD2
#define Y_DIR_RIGHT 0xD3
#define YELLOW_MAX_DRAWN_MAPS 48

#define Y_ADDR_SPRITE_DATA_1  0xC100
#define Y_ADDR_SPRITE_DATA_2  0xC200

#define Y_ROM_TILESETS_TABLE  ((0x03 * 0x4000) + (0x4558 - 0x4000))
#define Y_ROM_SPRITE_TABLE    ((0x05 * 0x4000) + (0x42a9 - 0x4000))

extern int scx_reg;
extern int scy_reg;

// Missable Objects (Toggleable Sprites) Memory Addresses
#define Y_ADDR_MISSABLE_OBJ_FLAGS 0xD5A5
#define Y_ADDR_MISSABLE_OBJ_LIST  0xD5CD
#define Y_ROM_TOGGLE_MAP_PTRS ((0x03 * 0x4000) + (0x469B - 0x4000))
#define Y_TOGGLE_STATES_PTR   0x4892
#define Y_TOGGLE_BANK         0x03

#define Y_SPRITE1_IMAGE_INDEX 2
#define Y_SPRITE1_MOVEMENT_STATUS 1

static bool yellow_event_flag_set(gb_t* gb, int flag_id) {
    if (flag_id < 0 || flag_id >= 256)
        return false;
    u8 flag_byte = Y_WRAM(gb, Y_ADDR_MISSABLE_OBJ_FLAGS + (flag_id / 8));
    return (flag_byte & (1 << (flag_id % 8))) != 0;
}

static bool yellow_pikachu_is_out(gb_t* gb) {
    u8 flags = Y_WRAM(gb, Y_ADDR_PIKACHU_OW_FLAGS);
    return (flags & Y_PIKACHU_HIDE_MASK) == 0;
}

// Checks if a specific sprite slot (0-15) is flagged as hidden
bool is_sprite_hidden(gb_t* gb, int slot) {
    if (slot == 0) return false; // Slot 0 is the player, never hidden this way
    
    int list_ptr = Y_ADDR_MISSABLE_OBJ_LIST;
    
    while (true) {
        // Read object ID from the list
        u8 obj_id = Y_WRAM(gb, list_ptr++);
        
        // 0xFF marks the end of the toggleable object list
        if (obj_id == 0xFF) {
            return false; // Not in the list -> not hidden
        }
        
        // Read the corresponding flag ID
        u8 flag_id = Y_WRAM(gb, list_ptr++);
        
        // If the object ID matches our current sprite slot...
        if (obj_id == slot) {
            // Check the specific bit in the Missable Object Flags array
            u8 flag_byte = Y_WRAM(gb, Y_ADDR_MISSABLE_OBJ_FLAGS + (flag_id / 8));
            u8 flag_bit = flag_id % 8;
            
            // If the bit is 1, the object is hidden
            return (flag_byte & (1 << flag_bit)) != 0;
        }
    }
}

static struct {
    bool initialized;
    int prev_scx;
    int prev_scy;
    u8 prev_player_x;
    u8 prev_player_y;
    u8 prev_map;
    u8 prev_tileset_id;
    int sub_x;
    int sub_y;
} yellow_scroll;

static void yellow_scroll_reseed(u8 player_x, u8 player_y, u8 cur_map, u8 tileset_id) {
    yellow_scroll.prev_scx = scx_reg;
    yellow_scroll.prev_scy = scy_reg;
    yellow_scroll.prev_player_x = player_x;
    yellow_scroll.prev_player_y = player_y;
    yellow_scroll.prev_map = cur_map;
    yellow_scroll.prev_tileset_id = tileset_id;
    yellow_scroll.sub_x = 0;
    yellow_scroll.sub_y = 0;
    yellow_scroll.initialized = true;
}

static bool yellow_scroll_warp_discontinuity(u8 cur_map, u8 tileset_id) {
    if (cur_map != yellow_scroll.prev_map)
        return true;
    if (tileset_id != yellow_scroll.prev_tileset_id)
        return true;
    return false;
}

static void update_scroll_tracking(gb_t* gb, u8 player_x, u8 player_y, u8 tileset_id, int* out_sub_x, int* out_sub_y) {
    u8 cur_map = Y_WRAM(gb, Y_ADDR_CUR_MAP);

    if (!yellow_scroll.initialized) {
        yellow_scroll_reseed(player_x, player_y, cur_map, tileset_id);
        *out_sub_x = 0;
        *out_sub_y = 0;
        return;
    }

    if (yellow_scroll_warp_discontinuity(cur_map, tileset_id)) {
        yellow_scroll_reseed(player_x, player_y, cur_map, tileset_id);
        *out_sub_x = 0;
        *out_sub_y = 0;
        return;
    }

    u8 walk_counter = Y_WRAM(gb, Y_ADDR_WALK_COUNTER);
    u8 in_battle = Y_WRAM(gb, Y_ADDR_IS_IN_BATTLE);
    if (walk_counter == 0 || in_battle != 0) {
        yellow_scroll_reseed(player_x, player_y, cur_map, tileset_id);
        *out_sub_x = 0;
        *out_sub_y = 0;
        return;
    }

    i8 scroll_dx = (i8)(scx_reg - yellow_scroll.prev_scx);
    i8 scroll_dy = (i8)(scy_reg - yellow_scroll.prev_scy);
    yellow_scroll.sub_x += scroll_dx;
    yellow_scroll.sub_y += scroll_dy;

    int coord_dx = (int)player_x - (int)yellow_scroll.prev_player_x;
    int coord_dy = (int)player_y - (int)yellow_scroll.prev_player_y;

    if (player_x != yellow_scroll.prev_player_x)
        yellow_scroll.sub_x -= coord_dx * 16;
    if (player_y != yellow_scroll.prev_player_y)
        yellow_scroll.sub_y -= coord_dy * 16;

    if (yellow_scroll.sub_x > 16 || yellow_scroll.sub_x < -16 ||
        yellow_scroll.sub_y > 16 || yellow_scroll.sub_y < -16) {
        yellow_scroll_reseed(player_x, player_y, cur_map, tileset_id);
        *out_sub_x = 0;
        *out_sub_y = 0;
        return;
    }

    yellow_scroll.prev_scx = scx_reg;
    yellow_scroll.prev_scy = scy_reg;
    yellow_scroll.prev_player_x = player_x;
    yellow_scroll.prev_player_y = player_y;
    yellow_scroll.prev_map = cur_map;
    yellow_scroll.prev_tileset_id = tileset_id;

    *out_sub_x = yellow_scroll.sub_x;
    *out_sub_y = yellow_scroll.sub_y;
}

typedef struct {
    u8 tileset_id;
    u8 rom_bank;
    u16 block_ptr;
    u16 gfx_ptr;
} yellow_tileset_ctx;

typedef struct {
    u8 map_id;
    u8 dir;
    i8 y;
    i8 x;
} yellow_connection;

typedef struct {
    u8 tileset_id;
    u8 height;
    u8 width;
    u8 bank;
    u16 blocks_ptr;
    u16 objects_ptr;
    yellow_connection conns[4];
    int n_conns;
} yellow_map_header;

static bool yellow_rom_abs(gb_t* gb, size_t addr, u8* out) {
    if (!gb->ROM || addr >= gb->ROM_SIZE)
        return false;
    *out = gb->ROM[addr];
    return true;
}

static bool yellow_rom_byte(gb_t* gb, u8 bank, u16 ptr, u8* out) {
    size_t off = (size_t)bank * 0x4000u + (ptr & 0x3FFFu);
    return yellow_rom_abs(gb, off, out);
}

// Sprite slot on a map that is not the one loaded in WRAM. The save bit is the
// global index of that object's entry in ToggleableObjectStates.
static bool yellow_rom_object_hidden(gb_t* gb, u8 map_id, int sprite_slot) {
    if (map_id >= Y_NUM_MAPS || sprite_slot <= 0)
        return false;

    u8 lo, hi;
    size_t pent = (size_t)Y_ROM_TOGGLE_MAP_PTRS + (size_t)map_id * 2;
    if (!yellow_rom_abs(gb, pent, &lo) || !yellow_rom_abs(gb, pent + 1, &hi))
        return false;

    u16 ptr = (u16)lo | ((u16)hi << 8);
    int diff = (int)ptr - Y_TOGGLE_STATES_PTR;
    if (diff < 0 || (diff % 3) != 0)
        return false;

    int index = diff / 3;
    for (int n = 0; n < 32; n++) {
        u8 entry_map, obj_id;
        if (!yellow_rom_byte(gb, Y_TOGGLE_BANK, ptr, &entry_map) ||
            !yellow_rom_byte(gb, Y_TOGGLE_BANK, (u16)(ptr + 1), &obj_id))
            return false;
        if (entry_map == 0xFF || entry_map != map_id)
            return false;
        if (obj_id == sprite_slot)
            return yellow_event_flag_set(gb, index);
        ptr = (u16)(ptr + 3);
        index++;
    }
    return false;
}

static bool yellow_load_tileset(gb_t* gb, u8 tileset_id, yellow_tileset_ctx* ctx) {
    size_t base = (size_t)Y_ROM_TILESETS_TABLE + (size_t)tileset_id * 12;
    u8 bank, b1, b2, b3, b4;
    if (!yellow_rom_abs(gb, base + 0, &bank) ||
        !yellow_rom_abs(gb, base + 1, &b1) ||
        !yellow_rom_abs(gb, base + 2, &b2) ||
        !yellow_rom_abs(gb, base + 3, &b3) ||
        !yellow_rom_abs(gb, base + 4, &b4))
        return false;
    ctx->tileset_id = tileset_id;
    ctx->rom_bank = bank;
    ctx->block_ptr = (u16)b1 | ((u16)b2 << 8);
    ctx->gfx_ptr = (u16)b3 | ((u16)b4 << 8);
    return true;
}

static bool yellow_read_map_header(gb_t* gb, u8 map_id, yellow_map_header* hdr) {
    if (map_id >= Y_NUM_MAPS)
        return false;

    u8 bank, lo, hi;
    if (!yellow_rom_abs(gb, (size_t)Y_ROM_MAP_HEADER_BANKS + map_id, &bank) ||
        !yellow_rom_abs(gb, (size_t)Y_ROM_MAP_HEADER_PTRS + (size_t)map_id * 2, &lo) ||
        !yellow_rom_abs(gb, (size_t)Y_ROM_MAP_HEADER_PTRS + (size_t)map_id * 2 + 1, &hi))
        return false;

    u16 ptr = (u16)lo | ((u16)hi << 8);
    u8 tileset, height, width, blk_lo, blk_hi, connections;
    if (!yellow_rom_byte(gb, bank, ptr + 0, &tileset) ||
        !yellow_rom_byte(gb, bank, ptr + 1, &height) ||
        !yellow_rom_byte(gb, bank, ptr + 2, &width) ||
        !yellow_rom_byte(gb, bank, ptr + 3, &blk_lo) ||
        !yellow_rom_byte(gb, bank, ptr + 4, &blk_hi) ||
        !yellow_rom_byte(gb, bank, ptr + 9, &connections))
        return false;
    if (width == 0 || height == 0 || width > YELLOW_MAX_MAP_BLOCKS || height > YELLOW_MAX_MAP_BLOCKS)
        return false;

    hdr->tileset_id = tileset;
    hdr->height = height;
    hdr->width = width;
    hdr->bank = bank;
    hdr->blocks_ptr = (u16)blk_lo | ((u16)blk_hi << 8);
    hdr->n_conns = 0;

    static const u8 dirs[4] = { Y_CONN_NORTH, Y_CONN_SOUTH, Y_CONN_WEST, Y_CONN_EAST };
    int off = 10;
    for (int i = 0; i < 4; i++) {
        if (!(connections & dirs[i]))
            continue;
        u8 conn_map, conn_y, conn_x;
        if (!yellow_rom_byte(gb, bank, (u16)(ptr + off), &conn_map) ||
            !yellow_rom_byte(gb, bank, (u16)(ptr + off + 7), &conn_y) ||
            !yellow_rom_byte(gb, bank, (u16)(ptr + off + 8), &conn_x))
            return false;
        yellow_connection* c = &hdr->conns[hdr->n_conns++];
        c->map_id = conn_map;
        c->dir = dirs[i];
        c->y = (i8)conn_y;
        c->x = (i8)conn_x;
        off += Y_CONN_STRUCT_SIZE;
    }

    u8 obj_lo, obj_hi;
    if (!yellow_rom_byte(gb, bank, (u16)(ptr + off), &obj_lo) ||
        !yellow_rom_byte(gb, bank, (u16)(ptr + off + 1), &obj_hi))
        return false;
    hdr->objects_ptr = (u16)obj_lo | ((u16)obj_hi << 8);
    return true;
}

static u8 yellow_overworld_pal_id(gb_t* gb, u8 map_id, u8 tileset_id) {
    u8 pal;
    if (tileset_id == Y_TILESET_CEMETERY)
        pal = Y_PAL_GRAYMON - 1;
    else if (tileset_id == Y_TILESET_CAVERN ||
             (map_id >= Y_MAP_CERULEAN_CAVE_2F && map_id <= Y_MAP_CERULEAN_CAVE_1F) ||
             map_id == Y_MAP_BRUNO)
        pal = Y_PAL_CAVE - 1;
    else if (map_id == Y_MAP_LORELEI)
        pal = 0;
    else if (map_id == Y_MAP_TRADE_CENTER || map_id == Y_MAP_COLOSSEUM)
        pal = Y_PAL_GRAYMON - 1;
    else {
        u8 id = map_id;
        if (map_id >= Y_FIRST_INDOOR_MAP)
            id = Y_WRAM(gb, Y_ADDR_LAST_MAP);
        pal = id < Y_NUM_CITY_MAPS ? id : 0xFF;
    }
    return (u8)(pal + 1);
}

static void yellow_apply_dmg_pal(const u8 base[8], u8 dmg_reg, u8 out[8]) {
    if (dmg_reg == 0)
        dmg_reg = 0xE4;
    for (int i = 0; i < 4; i++) {
        int src = (dmg_reg >> (i * 2)) & 3;
        out[i * 2] = base[src * 2];
        out[i * 2 + 1] = base[src * 2 + 1];
    }
}

static bool yellow_load_map_pals(gb_t* gb, u8 map_id, u8 tileset_id, u8 bg[8], u8 ob[8]) {
    u8 pal_id = yellow_overworld_pal_id(gb, map_id, tileset_id);
    if (pal_id >= Y_NUM_CGB_PALS)
        return false;
    u8 base[8];
    size_t addr = (size_t)Y_ROM_CGB_BASE_PALS + (size_t)pal_id * 8;
    for (int i = 0; i < 8; i++) {
        if (!yellow_rom_abs(gb, addr + i, &base[i]))
            return false;
    }
    yellow_apply_dmg_pal(base, gb->ppu.BGP_REG, bg);
    yellow_apply_dmg_pal(base, gb->ppu.OBP0_REG, ob);
    return true;
}

static void yellow_draw_block(gb_t* gb, u8 block_id, const yellow_tileset_ctx* ts,
    int world_px, int world_py, int map_offset_x, int map_offset_y,
    bool use_vram, bool only_sentinel, int sentinel, const u8* bgp) {
    int screen_x0 = world_px - map_offset_x;
    int screen_y0 = world_py - map_offset_y;
    if (screen_x0 >= width || screen_y0 >= height ||
        screen_x0 + YELLOW_BLOCK_PX <= 0 || screen_y0 + YELLOW_BLOCK_PX <= 0)
        return;

    for (int ty = 0; ty < 4; ty++) {
        for (int tx = 0; tx < 4; tx++) {
            u8 tile_id;
            u16 tile_addr = (u16)(ts->block_ptr + (u16)block_id * 16 + (u16)(ty * 4 + tx));
            if (!yellow_rom_byte(gb, ts->rom_bank, tile_addr, &tile_id))
                continue;

            u8* tile_gfx = NULL;
            if (use_vram) {
                u8* vram = gb_getTileGfx(gb, tile_id);
                bool tile_empty = true;
                for (int t = 0; t < 16; t++) {
                    if (vram[t] != 0) {
                        tile_empty = false;
                        break;
                    }
                }
                if (!tile_empty)
                    tile_gfx = vram;
            }

            for (int py = 0; py < 8; py++) {
                u8 row_byte1, row_byte2;
                if (tile_gfx) {
                    row_byte1 = tile_gfx[(py * 2) + 0];
                    row_byte2 = tile_gfx[(py * 2) + 1];
                } else {
                    u16 gfx_addr = (u16)(ts->gfx_ptr + (u16)tile_id * 16 + (u16)(py * 2));
                    if (!yellow_rom_byte(gb, ts->rom_bank, gfx_addr, &row_byte1) ||
                        !yellow_rom_byte(gb, ts->rom_bank, (u16)(gfx_addr + 1), &row_byte2))
                        continue;
                }

                for (int px = 0; px < 8; px++) {
                    int draw_x = world_px + tx * 8 + px - map_offset_x;
                    int draw_y = world_py + ty * 8 + py - map_offset_y;
                    if (draw_x < 0 || draw_x >= width ||
                        draw_y < 0 || draw_y >= height)
                        continue;

                    int pixel_i = draw_x + draw_y * stride;
                    if (only_sentinel && pixels[pixel_i] != sentinel)
                        continue;

                    int bit_index = 7 - px;
                    u8 b0 = (row_byte1 >> bit_index) & 1;
                    u8 b1 = (row_byte2 >> bit_index) & 1;
                    u8 col_idx = b0 | (b1 << 1);
                    const u8* pal = bgp ? bgp : gb->BGP_CRAM;
                    pixels[pixel_i] = CgbToRgb(pal[col_idx << 1], pal[(col_idx << 1) | 1]);
                }
            }
        }
    }
}

static void yellow_draw_picture(gb_t* gb, u8 picture_id, int draw_base_x, int draw_base_y,
    const u8* obp, int tile_base, bool flip_x) {
    if (picture_id == 0 || picture_id > Y_SPRITE_PICTURE_ID_MAX)
        return;
    if (draw_base_x < -16 || draw_base_x >= width ||
        draw_base_y < -16 || draw_base_y >= height)
        return;

    int entry = Y_ROM_SPRITE_TABLE + ((picture_id - 1) * 4);
    u8 lo, hi, sprite_bank;
    if (!yellow_rom_abs(gb, (size_t)entry + 0, &lo) ||
        !yellow_rom_abs(gb, (size_t)entry + 1, &hi) ||
        !yellow_rom_abs(gb, (size_t)entry + 3, &sprite_bank))
        return;
    u16 sprite_ptr = (u16)lo | ((u16)hi << 8);
    const u8* pal = obp ? obp : gb->OBP_CRAM;

    for (int tile = 0; tile < 4; tile++) {
        int tile_col = tile % 2;
        int tile_row = tile / 2;
        int tile_index = tile_base + tile_row * 2 + tile_col;

        for (int py = 0; py < 8; py++) {
            u8 row_byte1, row_byte2;
            u16 row_addr = (u16)(sprite_ptr + (u16)(tile_index * 16 + py * 2));
            if (!yellow_rom_byte(gb, sprite_bank, row_addr, &row_byte1) ||
                !yellow_rom_byte(gb, sprite_bank, (u16)(row_addr + 1), &row_byte2))
                continue;

            for (int px = 0; px < 8; px++) {
                int local_x = tile_col * 8 + px;
                int draw_x = draw_base_x + (flip_x ? 15 - local_x : local_x);
                int draw_y = draw_base_y + tile_row * 8 + py;
                if (draw_x < 0 || draw_x >= width ||
                    draw_y < 0 || draw_y >= height)
                    continue;

                int bit_index = 7 - px;
                u8 b0 = (row_byte1 >> bit_index) & 1;
                u8 b1 = (row_byte2 >> bit_index) & 1;
                u8 col_idx = b0 | (b1 << 1);
                if (col_idx == 0)
                    continue;
                pixels[draw_x + draw_y * stride] = CgbToRgb(pal[col_idx << 1], pal[(col_idx << 1) | 1]);
            }
        }
    }
}

static void yellow_still_pose(u8 movement, u8 range, bool is_item, int* tile_base, bool* flip_x) {
    u8 facing = Y_DIR_DOWN;
    if (!is_item && movement == Y_MOVE_STAY && range >= Y_DIR_DOWN && range <= Y_DIR_RIGHT)
        facing = range;
    else if (!is_item && movement == Y_MOVE_WALK && range == Y_RANGE_LEFT_RIGHT)
        facing = Y_DIR_LEFT;

    *flip_x = facing == Y_DIR_RIGHT;
    if (facing == Y_DIR_UP)
        *tile_base = 4;
    else if (facing == Y_DIR_LEFT || facing == Y_DIR_RIGHT)
        *tile_base = 8;
    else
        *tile_base = 0;
}

static void yellow_draw_rom_objects(gb_t* gb, u8 map_id, const yellow_map_header* hdr,
    int origin_bx, int origin_by, int map_offset_x, int map_offset_y, const u8* obp) {
    u16 ptr = hdr->objects_ptr;
    u8 border, nwarps, nbg, nobj;
    if (!yellow_rom_byte(gb, hdr->bank, ptr, &border))
        return;
    (void)border;
    if (!yellow_rom_byte(gb, hdr->bank, (u16)(ptr + 1), &nwarps) || nwarps > 32)
        return;
    ptr = (u16)(ptr + 2 + (u16)nwarps * 4);
    if (!yellow_rom_byte(gb, hdr->bank, ptr, &nbg) || nbg > 16)
        return;
    ptr = (u16)(ptr + 1 + (u16)nbg * 3);
    if (!yellow_rom_byte(gb, hdr->bank, ptr, &nobj) || nobj > 16)
        return;
    ptr = (u16)(ptr + 1);

    for (int i = 0; i < nobj; i++) {
        u8 picture, obj_y, obj_x, movement, range, text_id;
        if (!yellow_rom_byte(gb, hdr->bank, ptr, &picture) ||
            !yellow_rom_byte(gb, hdr->bank, (u16)(ptr + 1), &obj_y) ||
            !yellow_rom_byte(gb, hdr->bank, (u16)(ptr + 2), &obj_x) ||
            !yellow_rom_byte(gb, hdr->bank, (u16)(ptr + 3), &movement) ||
            !yellow_rom_byte(gb, hdr->bank, (u16)(ptr + 4), &range) ||
            !yellow_rom_byte(gb, hdr->bank, (u16)(ptr + 5), &text_id))
            return;

        if (!yellow_rom_object_hidden(gb, map_id, i + 1)) {
            int tile_base;
            bool flip_x;
            yellow_still_pose(movement, range, (text_id & Y_OBJ_ITEM) != 0, &tile_base, &flip_x);
            int grid_x = (int)obj_x - 4;
            int grid_y = (int)obj_y - 4;
            int world_px = origin_bx * YELLOW_BLOCK_PX + grid_x * 16;
            int world_py = origin_by * YELLOW_BLOCK_PX + grid_y * 16 - 4;
            yellow_draw_picture(gb, picture,
                world_px - map_offset_x, world_py - map_offset_y, obp, tile_base, flip_x);
        }

        int size = 6;
        if (text_id & Y_OBJ_TRAINER)
            size = 8;
        else if (text_id & Y_OBJ_ITEM)
            size = 7;
        ptr = (u16)(ptr + size);
    }
}

typedef struct {
    int origin_bx;
    int origin_by;
    u8 width;
    u8 height;
    u8 border_block;
    u8 conn_mask;
    bool use_vram;
    bool has_pal;
    u8 bg_pal[8];
    yellow_tileset_ctx ts;
} yellow_drawn_map;

typedef struct {
    yellow_drawn_map maps[YELLOW_MAX_DRAWN_MAPS];
    int count;
} yellow_map_list;

// CheckMapConnections stores the new map id and player coords, then fades music,
// and only afterwards copies blocks into wOverworldMap. A frame in between
// would draw the previous map's blocks with the new map's size and camera.
static bool yellow_wram_blocks_ready(gb_t* gb, const yellow_map_header* hdr, u8 map_width, u8 map_height) {
    if (hdr->width != map_width || hdr->height != map_height)
        return false;

    int memory_width = map_width + YELLOW_MAP_BORDER_BLOCKS * 2;
    int samples = 0;
    int mismatches = 0;
    for (int by = 0; by < map_height; by++) {
        for (int bx = 0; bx < map_width; bx++) {
            u8 rom_block;
            u16 addr = (u16)(hdr->blocks_ptr + (u16)(by * map_width + bx));
            if (!yellow_rom_byte(gb, hdr->bank, addr, &rom_block))
                return false;
            int wx = bx + YELLOW_MAP_BORDER_BLOCKS;
            int wy = by + YELLOW_MAP_BORDER_BLOCKS;
            u8 wram_block = Y_WRAM(gb, Y_ADDR_MAP_LAYOUT + wx + wy * memory_width);
            samples++;
            if (wram_block != rom_block)
                mismatches++;
        }
    }
    return samples > 0 && mismatches * 5 <= samples;
}

static void yellow_record_map(gb_t* gb, yellow_map_list* list, u8 map_id, const yellow_map_header* hdr,
    const yellow_tileset_ctx* ts, int origin_bx, int origin_by, u8 current_tileset, bool current_map) {
    if (list->count >= YELLOW_MAX_DRAWN_MAPS)
        return;
    u8 border;
    if (!yellow_rom_byte(gb, hdr->bank, hdr->objects_ptr, &border))
        return;

    yellow_drawn_map* m = &list->maps[list->count++];
    m->origin_bx = origin_bx;
    m->origin_by = origin_by;
    m->width = hdr->width;
    m->height = hdr->height;
    m->border_block = border;
    m->conn_mask = 0;
    for (int i = 0; i < hdr->n_conns; i++)
        m->conn_mask |= hdr->conns[i].dir;
    m->ts = *ts;
    m->use_vram = hdr->tileset_id == current_tileset;
    m->has_pal = false;
    if (!current_map) {
        u8 ob_pal[8];
        m->has_pal = yellow_load_map_pals(gb, map_id, hdr->tileset_id, m->bg_pal, ob_pal);
    }
}

static int yellow_floor_div(int n, int d) {
    int q = n / d;
    if (n % d < 0)
        q--;
    return q;
}

static int yellow_outside_dist(const yellow_drawn_map* m, int bx, int by) {
    int dx = 0;
    int dy = 0;
    if (bx < m->origin_bx)
        dx = m->origin_bx - bx;
    else if (bx >= m->origin_bx + m->width)
        dx = bx - (m->origin_bx + m->width - 1);
    if (by < m->origin_by)
        dy = m->origin_by - by;
    else if (by >= m->origin_by + m->height)
        dy = by - (m->origin_by + m->height - 1);
    return dx > dy ? dx : dy;
}

static bool yellow_map_may_fill(const yellow_drawn_map* m, int bx, int by) {
    bool north = by < m->origin_by;
    bool south = by >= m->origin_by + m->height;
    bool west = bx < m->origin_bx;
    bool east = bx >= m->origin_bx + m->width;
    if (!north && !south && !west && !east)
        return false;
    if (north && (m->conn_mask & Y_CONN_NORTH))
        return false;
    if (south && (m->conn_mask & Y_CONN_SOUTH))
        return false;
    if (west && (m->conn_mask & Y_CONN_WEST))
        return false;
    if (east && (m->conn_mask & Y_CONN_EAST))
        return false;
    return true;
}

static void yellow_fill_outside(gb_t* gb, const yellow_map_list* list,
    int map_offset_x, int map_offset_y, int sentinel) {
    if (list->count == 0)
        return;

    int bx0 = yellow_floor_div(map_offset_x, YELLOW_BLOCK_PX);
    int by0 = yellow_floor_div(map_offset_y, YELLOW_BLOCK_PX);
    int bx1 = yellow_floor_div(map_offset_x + width - 1, YELLOW_BLOCK_PX);
    int by1 = yellow_floor_div(map_offset_y + height - 1, YELLOW_BLOCK_PX);

    for (int by = by0; by <= by1; by++) {
        for (int bx = bx0; bx <= bx1; bx++) {
            int sample_x = bx * YELLOW_BLOCK_PX - map_offset_x;
            int sample_y = by * YELLOW_BLOCK_PX - map_offset_y;
            if (sample_x < 0)
                sample_x = 0;
            if (sample_y < 0)
                sample_y = 0;
            if (sample_x >= width || sample_y >= height)
                continue;
            if (pixels[sample_x + sample_y * stride] != sentinel)
                continue;

            const yellow_drawn_map* best = NULL;
            int best_dist = 1 << 30;
            const yellow_drawn_map* fallback = NULL;
            int fallback_dist = 1 << 30;
            for (int i = 0; i < list->count; i++) {
                const yellow_drawn_map* m = &list->maps[i];
                int dist = yellow_outside_dist(m, bx, by);
                if (dist <= 0)
                    continue;
                if (dist < fallback_dist) {
                    fallback = m;
                    fallback_dist = dist;
                }
                if (!yellow_map_may_fill(m, bx, by))
                    continue;
                if (dist < best_dist) {
                    best = m;
                    best_dist = dist;
                }
            }
            const yellow_drawn_map* chosen = best ? best : fallback;
            if (!chosen)
                continue;
            yellow_draw_block(gb, chosen->border_block, &chosen->ts,
                bx * YELLOW_BLOCK_PX, by * YELLOW_BLOCK_PX,
                map_offset_x, map_offset_y, chosen->use_vram, true, sentinel,
                chosen->has_pal ? chosen->bg_pal : NULL);
        }
    }
}

static void yellow_draw_rom_map(gb_t* gb, u8 map_id, int origin_bx, int origin_by, int depth,
    bool visited[256], u8 current_tileset, int map_offset_x, int map_offset_y, yellow_map_list* list) {
    yellow_map_header hdr;
    if (!yellow_read_map_header(gb, map_id, &hdr))
        return;

    bool first_visit = !visited[map_id];
    visited[map_id] = true;

    if (first_visit) {
        yellow_tileset_ctx ts;
        if (yellow_load_tileset(gb, hdr.tileset_id, &ts)) {
            yellow_record_map(gb, list, map_id, &hdr, &ts, origin_bx, origin_by,
                current_tileset, false);
            int map_x0 = origin_bx * YELLOW_BLOCK_PX - map_offset_x;
            int map_y0 = origin_by * YELLOW_BLOCK_PX - map_offset_y;
            int map_x1 = map_x0 + (int)hdr.width * YELLOW_BLOCK_PX;
            int map_y1 = map_y0 + (int)hdr.height * YELLOW_BLOCK_PX;
            bool on_screen = map_x1 > 0 && map_y1 > 0 &&
                map_x0 < width && map_y0 < height;
            if (on_screen) {
                bool use_vram = hdr.tileset_id == current_tileset;
                u8 bg_pal[8], ob_pal[8];
                const u8* bgp = NULL;
                const u8* obp = NULL;
                if (yellow_load_map_pals(gb, map_id, hdr.tileset_id, bg_pal, ob_pal)) {
                    bgp = bg_pal;
                    obp = ob_pal;
                }
                for (int by = 0; by < hdr.height; by++) {
                    for (int bx = 0; bx < hdr.width; bx++) {
                        u8 block_id;
                        u16 addr = (u16)(hdr.blocks_ptr + (u16)(by * hdr.width + bx));
                        if (!yellow_rom_byte(gb, hdr.bank, addr, &block_id))
                            continue;
                        yellow_draw_block(gb, block_id, &ts,
                            (origin_bx + bx) * YELLOW_BLOCK_PX,
                            (origin_by + by) * YELLOW_BLOCK_PX,
                            map_offset_x, map_offset_y, use_vram, false, 0, bgp);
                    }
                }
                yellow_draw_rom_objects(gb, map_id, &hdr, origin_bx, origin_by,
                    map_offset_x, map_offset_y, obp);
            }
        }
    }

    if (depth <= 0)
        return;

    for (int i = 0; i < hdr.n_conns; i++) {
        yellow_connection* c = &hdr.conns[i];
        if (visited[c->map_id])
            continue;

        yellow_map_header child;
        if (!yellow_read_map_header(gb, c->map_id, &child))
            continue;

        int offset = (c->dir == Y_CONN_NORTH || c->dir == Y_CONN_SOUTH)
            ? -((int)c->x) / 2
            : -((int)c->y) / 2;
        int dx = 0;
        int dy = 0;
        if (c->dir == Y_CONN_NORTH) {
            dx = offset;
            dy = -(int)child.height;
        } else if (c->dir == Y_CONN_SOUTH) {
            dx = offset;
            dy = (int)hdr.height;
        } else if (c->dir == Y_CONN_WEST) {
            dx = -(int)child.width;
            dy = offset;
        } else if (c->dir == Y_CONN_EAST) {
            dx = (int)hdr.width;
            dy = offset;
        }

        yellow_draw_rom_map(gb, c->map_id, origin_bx + dx, origin_by + dy, depth - 1,
            visited, current_tileset, map_offset_x, map_offset_y, list);
    }
}

static void yellow_draw_wram_map(gb_t* gb, const yellow_tileset_ctx* ts, u8 map_width, u8 map_height,
    int map_offset_x, int map_offset_y, bool border, bool only_sentinel, int sentinel) {
    int memory_width = map_width + YELLOW_MAP_BORDER_BLOCKS * 2;
    int memory_height = map_height + YELLOW_MAP_BORDER_BLOCKS * 2;

    for (int by = 0; by < memory_height; by++) {
        for (int bx = 0; bx < memory_width; bx++) {
            bool is_border = bx < YELLOW_MAP_BORDER_BLOCKS || by < YELLOW_MAP_BORDER_BLOCKS ||
                bx >= map_width + YELLOW_MAP_BORDER_BLOCKS ||
                by >= map_height + YELLOW_MAP_BORDER_BLOCKS;
            if (is_border != border)
                continue;

            u8 block_id = Y_WRAM(gb, Y_ADDR_MAP_LAYOUT + bx + (by * memory_width));
            yellow_draw_block(gb, block_id, ts,
                bx * YELLOW_BLOCK_PX - YELLOW_MAP_BORDER_BLOCKS * YELLOW_BLOCK_PX,
                by * YELLOW_BLOCK_PX - YELLOW_MAP_BORDER_BLOCKS * YELLOW_BLOCK_PX,
                map_offset_x, map_offset_y, true, only_sentinel, sentinel, NULL);
        }
    }
}

bool gb_draw_yellow_revamped(gb_t* gb) {
    u8 map_height = Y_WRAM(gb, Y_ADDR_MAP_HEIGHT);
    u8 map_width  = Y_WRAM(gb, Y_ADDR_MAP_WIDTH);
    u8 cur_map    = Y_WRAM(gb, Y_ADDR_CUR_MAP);
    yellow_map_header cur_hdr;
    if (!yellow_read_map_header(gb, cur_map, &cur_hdr) ||
        !yellow_wram_blocks_ready(gb, &cur_hdr, map_width, map_height))
        return false;

    int sentinel = color(30, 30, 30);
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            pixels[x + y * stride] = sentinel;
        }
    }

    u8 tileset_id = Y_WRAM(gb, Y_ADDR_TILESET_ID);
    u8 player_x   = Y_WRAM(gb, Y_ADDR_PLAYER_X);
    u8 player_y   = Y_WRAM(gb, Y_ADDR_PLAYER_Y);

    yellow_tileset_ctx tileset;
    bool have_tileset = yellow_load_tileset(gb, tileset_id, &tileset);

    int gb_screen_canvas_x = (width - LCD_WIDTH) / 2;
    int gb_screen_canvas_y = (height - LCD_HEIGHT) / 2;

    int player_px = player_x * 16;
    int player_py = player_y * 16;

    int sub_x;
    int sub_y;
    update_scroll_tracking(gb, player_x, player_y, tileset_id, &sub_x, &sub_y);

    int camera_x = player_px - 64 + sub_x;
    int camera_y = player_py - 64 + sub_y;

    int map_offset_x = camera_x - gb_screen_canvas_x;
    int map_offset_y = camera_y - gb_screen_canvas_y;

    if (have_tileset) {
        yellow_draw_wram_map(gb, &tileset, map_width, map_height,
            map_offset_x, map_offset_y, false, false, sentinel);

        bool visited[256] = {0};
        visited[cur_map] = true;
        yellow_map_list drawn = {0};
        yellow_record_map(gb, &drawn, cur_map, &cur_hdr, &tileset, 0, 0, tileset_id, true);
        yellow_draw_rom_map(gb, cur_map, 0, 0, YELLOW_MAP_RECURSION_DEPTH, visited,
            tileset_id, map_offset_x, map_offset_y, &drawn);

        yellow_draw_wram_map(gb, &tileset, map_width, map_height,
            map_offset_x, map_offset_y, true, true, sentinel);
        yellow_fill_outside(gb, &drawn, map_offset_x, map_offset_y, sentinel);
    }

    u8 num_sprites = Y_WRAM(gb, Y_ADDR_NUM_SPRITES);

    for (int slot = 1; slot < 16; slot++) {
        if (slot == Y_PIKACHU_SPRITE_SLOT) {
            if (!yellow_pikachu_is_out(gb))
                continue;
        } else if (slot > num_sprites) {
            continue;
        }

        u8 picture_id = Y_WRAM(gb, Y_ADDR_SPRITE_DATA_1 + (slot * 16) + 0);
        if (picture_id == 0 || picture_id > Y_SPRITE_PICTURE_ID_MAX) continue;

        u8 movement_status = Y_WRAM(gb, Y_ADDR_SPRITE_DATA_1 + (slot * 16) + Y_SPRITE1_MOVEMENT_STATUS);
        if (movement_status == 0) continue;

        if (is_sprite_hidden(gb, slot)) continue;

        int npc_y = Y_WRAM(gb, Y_ADDR_SPRITE_DATA_2 + (slot * 16) + 4) - 4;
        int npc_x = Y_WRAM(gb, Y_ADDR_SPRITE_DATA_2 + (slot * 16) + 5) - 4;

        if (npc_x < 0 || npc_y < 0) continue;

        int expected_screen_x = npc_x * 16 - camera_x;
        int expected_screen_y = npc_y * 16 - camera_y;

        u8 image_index = Y_WRAM(gb, Y_ADDR_SPRITE_DATA_1 + (slot * 16) + Y_SPRITE1_IMAGE_INDEX);
        int draw_base_x;
        int draw_base_y;

        if (image_index != 0xff) {
            u8 y_pixels = Y_WRAM(gb, Y_ADDR_SPRITE_DATA_1 + (slot * 16) + 4);
            u8 x_pixels = Y_WRAM(gb, Y_ADDR_SPRITE_DATA_1 + (slot * 16) + 6);
            int delta_x = (i8)(u8)(x_pixels - (u8)expected_screen_x);
            int delta_y = (i8)(u8)(y_pixels - (u8)expected_screen_y);
            draw_base_x = gb_screen_canvas_x + expected_screen_x + delta_x;
            draw_base_y = gb_screen_canvas_y + expected_screen_y + delta_y;
        } else {
            draw_base_x = gb_screen_canvas_x + expected_screen_x;
            draw_base_y = gb_screen_canvas_y + expected_screen_y;
        }

        yellow_draw_picture(gb, picture_id, draw_base_x, draw_base_y, NULL, 0, false);
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

    return true;
}
