#include "cores/gbc/gb.h"

#include "SDL_MAINLOOP.h"

typedef void (*sgb_command_t)(sgb_t* sgb);

static void sgb_PAL01(sgb_t* sgb);
static void sgb_PAL23(sgb_t* sgb);
static void sgb_PAL03(sgb_t* sgb);
static void sgb_PAL12(sgb_t* sgb);
static void sgb_ATTR_BLK(sgb_t* sgb);
static void sgb_ATTR_LIN(sgb_t* sgb);
static void sgb_ATTR_DIV(sgb_t* sgb);
static void sgb_ATTR_CHR(sgb_t* sgb);
static void sgb_SOUND(sgb_t* sgb);
static void sgb_SOU_TRN(sgb_t* sgb);
static void sgb_PAL_SET(sgb_t* sgb);
static void sgb_PAL_TRN(sgb_t* sgb);
static void sgb_ATRC_EN(sgb_t* sgb);
static void sgb_TEST_EN(sgb_t* sgb);
static void sgb_ICON_EN(sgb_t* sgb);
static void sgb_DATA_SND(sgb_t* sgb);
static void sgb_DATA_TRN(sgb_t* sgb);
static void sgb_MLT_REQ(sgb_t* sgb);
static void sgb_JUMP(sgb_t* sgb);
static void sgb_CHR_TRN(sgb_t* sgb);
static void sgb_PCT_TRN(sgb_t* sgb);
static void sgb_ATTR_TRN(sgb_t* sgb);
static void sgb_ATTR_SET(sgb_t* sgb);
static void sgb_MASK_EN(sgb_t* sgb);
static void sgb_OBJ_TRN(sgb_t* sgb);
static void sgb_PAL_PRI(sgb_t* sgb);

static sgb_command_t commands[32] = {
    sgb_PAL01,
    sgb_PAL23,
    sgb_PAL03,
    sgb_PAL12,
    sgb_ATTR_BLK,
    sgb_ATTR_LIN,
    sgb_ATTR_DIV,
    sgb_ATTR_CHR,
    sgb_SOUND,
    sgb_SOU_TRN,
    sgb_PAL_SET,
    sgb_PAL_TRN,
    sgb_ATRC_EN,
    sgb_TEST_EN,
    sgb_ICON_EN,
    sgb_DATA_SND,
    sgb_DATA_TRN,
    sgb_MLT_REQ,
    sgb_JUMP,
    sgb_CHR_TRN,
    sgb_PCT_TRN,
    sgb_ATTR_TRN,
    sgb_ATTR_SET,
    sgb_MASK_EN,
    sgb_OBJ_TRN,
    sgb_PAL_PRI
};

static const char command_names[][32] = {
    "PAL01",
    "PAL23",
    "PAL03",
    "PAL12",
    "ATTR_BLK",
    "ATTR_LIN",
    "ATTR_DIV",
    "ATTR_CHR",
    "SOUND",
    "SOU_TRN",
    "PAL_SET",
    "PAL_TRN",
    "ATRC_EN",
    "TEST_EN",
    "ICON_EN",
    "DATA_SND",
    "DATA_TRN",
    "MLT_REQ",
    "JUMP",
    "CHR_TRN",
    "PCT_TRN",
    "ATTR_TRN",
    "ATTR_SET",
    "MASK_EN",
    "OBJ_TRN",
    "PAL_PRI"
};

static const u16 default_palettes[][4] =
{
    { 0x67BF, 0x265B, 0x10B5, 0x2866 },
    { 0x637B, 0x3AD9, 0x0956, 0x0000 },
    { 0x7F1F, 0x2A7D, 0x30F3, 0x4CE7 },
    { 0x57FF, 0x2618, 0x001F, 0x006A },
    { 0x5B7F, 0x3F0F, 0x222D, 0x10EB },
    { 0x7FBB, 0x2A3C, 0x0015, 0x0900 },
    { 0x2800, 0x7680, 0x01EF, 0x2FFF },
    { 0x73BF, 0x46FF, 0x0110, 0x0066 },
    { 0x533E, 0x2638, 0x01E5, 0x0000 },
    { 0x7FFF, 0x2BBF, 0x00DF, 0x2C0A },
    { 0x7F1F, 0x463D, 0x74CF, 0x4CA5 },
    { 0x53FF, 0x03E0, 0x00DF, 0x2800 },
    { 0x433F, 0x72D2, 0x3045, 0x0822 },
    { 0x7FFA, 0x2A5F, 0x0014, 0x0003 },
    { 0x1EED, 0x215C, 0x42FC, 0x0060 },
    { 0x7FFF, 0x5EF7, 0x39CE, 0x0000 },
    { 0x4F5F, 0x630E, 0x159F, 0x3126 },
    { 0x637B, 0x121C, 0x0140, 0x0840 },
    { 0x66BC, 0x3FFF, 0x7EE0, 0x2C84 },
    { 0x5FFE, 0x3EBC, 0x0321, 0x0000 },
    { 0x63FF, 0x36DC, 0x11F6, 0x392A },
    { 0x65EF, 0x7DBF, 0x035F, 0x2108 },
    { 0x2B6C, 0x7FFF, 0x1CD9, 0x0007 },
    { 0x53FC, 0x1F2F, 0x0E29, 0x0061 },
    { 0x36BE, 0x7EAF, 0x681A, 0x3C00 },
    { 0x7BBE, 0x329D, 0x1DE8, 0x0423 },
    { 0x739F, 0x6A9B, 0x7293, 0x0001 },
    { 0x5FFF, 0x6732, 0x3DA9, 0x2481 },
    { 0x577F, 0x3EBC, 0x456F, 0x1880 },
    { 0x6B57, 0x6E1B, 0x5010, 0x0007 },
    { 0x0F96, 0x2C97, 0x0045, 0x3200 },
    { 0x67FF, 0x2F17, 0x2230, 0x1548 }
};

typedef struct palette_entry_t {
    const char name[16];
    u8 index;
} palette_entry_t;

static palette_entry_t palette_lut[] = {
    {"ZELDA", 4},
    {"SUPER MARIOLAND", 5},
    {"MARIOLAND2", 19},
    {"SUPERMARIOLAND3", 1},
    {"KIRBY DREAM LAND", 10},
    {"HOSHINOKA-BI", 10},
    {"KIRBY'S PINBALL", 2},
    {"YOSSY NO TAMAGO", 11},
    {"MARIO & YOSHI", 11},
    {"YOSSY NO COOKIE", 3},
    {"YOSHI'S COOKIE", 3},
    {"DR.MARIO", 17},
    {"TETRIS", 16},
    {"YAKUMAN", 18},
    {"METROID2", 30},
    {"KAERUNOTAMENI", 8},
    {"GOLF", 23},
    {"ALLEY WAY", 21},
    {"BASEBALL", 14},
    {"TENNIS", 22},
    {"F1RACE", 29},
    {"KID ICARUS", 13},
    {"QIX", 24},
    {"SOLARSTRIKER", 6},
    {"X", 27},
    {"GBWARS", 20},
};

void sgb_init_palettes(sgb_t* sgb, const char* rom_name) {
    for (int i = 0; i < 4; i++)
        sgb->colors[i] = default_palettes[0][i];

    for (int i = 0; i < sizeof(palette_lut) / sizeof(palette_entry_t); i++) {
        if (!memcmp(palette_lut[i].name, rom_name, sizeof(palette_lut[i].name))) {
            for (int j = 0; j < 4; j++)
                sgb->colors[j] = default_palettes[palette_lut[i].index][j];
            return;
        }
    }

}

static void vram_transfer(sgb_t* sgb, u8* dst, size_t n_tiles) {
    sgb->vram_transfer = SGB_VRAM_TRANSFER_NONE;
    for (int i = 0; i < n_tiles; i++) {
        int tx = (i % 20);
        int ty = (i / 20);
        for (int y = 0; y < 8; y++) {
            *dst = 0;
            *(dst + 1) = 0;
            for (int x = 0; x < 8; x++) {
                u8 bits = sgb->gameboy_window[(tx << 3 | x) + (ty << 3 | y) * LCD_WIDTH];
                bool bit0 = bits & 1;
                bool bit1 = bits & 0b10;
                *dst |= bit0 << (7-x);
                *(dst + 1) |= bit1 << (7-x);
            }
            dst += 2;
        }
    }
}

static void command_execute(sgb_t* sgb) {
    u8 command = sgb->packets[0] >> 3;
    if (command >= 0x20)
        return;

    printf("%s\n", command_names[command]);
    commands[command](sgb);

}

void sgb_write(sgb_t* sgb, bool bit0, bool bit1) {
    if (bit0 && bit1) {
        sgb->ready_for_pulse = true;
        return;
    }

    if (!bit0 && !bit1) {
        if (!sgb->ready_for_pulse) 
            return;
   
        if ((sgb->byte_idx > 0 || sgb->bit_idx > 0) || sgb->ready_for_stop) {
            sgb->packet_idx = 0;
        }
            
        sgb->is_receiving = true;
        sgb->bit_idx = 0;
        sgb->byte_idx = 0;
        sgb->ready_for_stop = false;
        sgb->ready_for_pulse = false;
        return;
    }

    if (!sgb->is_receiving || !sgb->ready_for_pulse)
        return;

    sgb->ready_for_pulse = false;

    u8 bit_val = (!bit0 && bit1) ? 0 : 1; 

    if (sgb->ready_for_stop) {
        // stop bit must be 0
        if (bit_val == 1) {
            sgb->packet_idx = 0;
        } else {
            sgb->packet_idx++;
            u8 total_packets = sgb->packets[0] & 0x07;

            if (!total_packets)
                sgb->packet_idx = 0;
            
            if (sgb->packet_idx >= total_packets || sgb->packet_idx >= 7) {
                sgb->packet_idx = 0;
                command_execute(sgb);
            }
        }
        
        sgb->is_receiving = false;
        sgb->ready_for_stop = false;
        return;
    }

    if (!sgb->bit_idx)
        sgb->packets[sgb->packet_idx * 16 + sgb->byte_idx] = 0;

    sgb->packets[sgb->packet_idx * 16 + sgb->byte_idx] |= (bit_val << sgb->bit_idx);

    sgb->bit_idx++;
    if (sgb->bit_idx == 8) {
        sgb->bit_idx = 0;
        sgb->byte_idx++;

        if (sgb->byte_idx == 16) {
            sgb->ready_for_stop = true;
            sgb->byte_idx = 0; 
        }
    }
}

static int sgb_color(u16 bgr555, u8 fade) {
    u8 red = bgr555 & 0x1F;
    u8 green = (bgr555 >> 5) & 0x1F;
    u8 blue = (bgr555 >> 10) & 0x1F;
    red = red > fade ? red - fade : 0;
    green = green > fade ? green - fade : 0;
    blue = blue > fade ? blue - fade : 0;   
    red = (red << 3) | (red >> 2);
    green = (green << 3) | (green >> 2);
    blue = (blue << 3) | (blue >> 2);

    return color(red, green, blue);
}

void sgb_render(sgb_t* sgb) {
    sgb->must_render = true;

    switch (sgb->vram_transfer) {
        case SGB_VRAM_TRANSFER_NONE:
        break;

        case SGB_VRAM_TRANSFER_PAL:
        vram_transfer(sgb, (u8*)sgb->pal_ram, 256);
        break;

        case SGB_VRAM_TRANSFER_ATTR:
        vram_transfer(sgb, (u8*)sgb->atf, 256);
        break;

        case SGB_VRAM_TRANSFER_TILE_00_7F:
        vram_transfer(sgb, sgb->tile_ram, 256);
        break;

        case SGB_VRAM_TRANSFER_TILE_80_FF:
        vram_transfer(sgb, sgb->tile_ram + 4096, 256);
        break;

        case SGB_VRAM_TRANSFER_TILEMAP:
        vram_transfer(sgb, sgb->tilemap, 256);
        break;

        default:
        printf("Unknown VRAM transfer: %d\n", sgb->vram_transfer);
        break;
    }

    int offset_x = (SGB_WIDTH - LCD_WIDTH) / 2;   // (256 - 160) / 2 = 48
    int offset_y = (SGB_HEIGHT - LCD_HEIGHT) / 2; // (224 - 144) / 2 = 40

    switch (sgb->mask) {
        case SGB_MASK_NONE:
        for (int y = 0; y < LCD_HEIGHT; y++) {
            for (int x = 0; x < LCD_WIDTH; x++) {
                u8 idx = sgb->gameboy_window[x + y * LCD_WIDTH] & 0b11;
                u8 palette = idx ? sgb->attributes[(x >> 3) + (y >> 3) * 20] : 0;
                u16 bgr555 = sgb->colors[palette * 4 + idx];
                int dest_x = x + offset_x;
                int dest_y = y + offset_y;
                sgb->screen[dest_x + dest_y * SGB_WIDTH] = bgr555;     
            }
        }
        break;

        case SGB_MASK_FREEZE:
        return;

        case SGB_MASK_BLACK:
        for (int y = offset_y; y < offset_y + LCD_HEIGHT; y++)
            for (int x = offset_x; x < offset_x + LCD_WIDTH; x++)
                sgb->screen[x + y * SGB_WIDTH] = 0;
        return;

        case SGB_MASK_COLOR0:
        for (int y = offset_y; y < offset_y + LCD_HEIGHT; y++)
            for (int x = offset_x; x < offset_x + LCD_WIDTH; x++)
                sgb->screen[x + y * SGB_WIDTH] = sgb->colors[0];
        return;
    }

    for (int y = 0; y < SGB_HEIGHT; y++) {
        for (int x = 0; x < SGB_WIDTH; x++) {
            int tx = x >> 3;
            int ty = y >> 3;
            int px = x & 7;
            int py = y & 7;
            
            u16 map_entry = ((u16*)sgb->tilemap)[tx + (ty << 5)];
            
            u16 tile_num = map_entry & 0xFF;
            u8 pal_num = (map_entry >> 10) & 0x07;
            bool x_flip = (map_entry >> 14) & 0x01;
            bool y_flip = (map_entry >> 15) & 0x01;
            
            
            if (x_flip)
                px = 7 - px;
            if (y_flip)
                py = 7 - py;
            
            
            u8* tile_data = &sgb->tile_ram[tile_num * 32];
            
            u8 bp0 = tile_data[py * 2];
            u8 bp1 = tile_data[py * 2 + 1];
            u8 bp2 = tile_data[py * 2 + 16];
            u8 bp3 = tile_data[py * 2 + 17];
            
            u8 bit = 7 - px;
            u8 color_idx = ((bp0 >> bit) & 1) |
                           (((bp1 >> bit) & 1) << 1) |
                           (((bp2 >> bit) & 1) << 2) |
                           (((bp3 >> bit) & 1) << 3);
    
            bool in_gb_window = (y >= offset_y && y < offset_y + LCD_HEIGHT &&  x >= offset_x && x < offset_x + LCD_WIDTH);
            
            if (!color_idx && in_gb_window)
                continue;

            pal_num = (pal_num >= 4 && pal_num <= 7) ? (pal_num - 4) : 0;
            if (!color_idx)
                pal_num = 0;
            u16 bgr555 = ((u16*)&sgb->tilemap[0x800])[(pal_num << 4) | (color_idx)];
            sgb->screen[x + y * SGB_WIDTH] = bgr555;
        }
    }
}

void sgb_show_to_screen(gb_t* gb) {
    sgb_t* sgb = &gb->sgb;

    int fade = 0;
    if (sgb->fade_enabled) {
        size_t fade_counter = (gb->startFrame_clock + gb->cpu.cycles) / CYCLES_PER_FRAME;
        if (fade_counter >= SGB_MAX_FADE_COUNTER)
            sgb->fade_enabled = false;
        else if (fade_counter < 32)
            fade = fade_counter;
        else if (fade_counter < SGB_MAX_FADE_COUNTER - 31)
            fade = 31;
        else
            fade = SGB_MAX_FADE_COUNTER - fade_counter;
    }

    for (int i = 0; i < SGB_WIDTH * SGB_HEIGHT; i++) {
        pixels[i] = sgb_color(sgb->screen[i], fade);   
    }

    if (fade || sgb->must_render) {
        renderPixels();
        sgb->must_render = false;
    }
}

bool gb_has_sgb_functionality(const u8* rom) {
    return rom[0x146] == 0x03 && rom[0x14B] == 0x33;
}

static void sgb_pal(sgb_t* sgb, u8 palA, u8 palB) {
    for (int i = 0; i < 4; i++) {
        sgb->colors[palA * 4 + i] = sgb->packets[1 + i * 2] | (sgb->packets[2 + i * 2] << 8);
    }

    for (int i = 0; i < 3; i++) {
        sgb->colors[palB * 4 + i + 1] = sgb->packets[9 + i * 2] | (sgb->packets[10 + i * 2] << 8);
    }

    sgb->colors[0] = sgb->colors[palA * 4];
}

static void load_atf(sgb_t* sgb, u8 atf_number) {
    if (atf_number >= 45)
        return;

    for (int y = 0; y < 18; y++) {
        for (int x = 0; x < 20; x++) {
            u8 shift = 6 - ((x & 0b11) * 2);
            u8 val = (sgb->atf[atf_number * 90 + y * 5 + x / 4] >> shift) & 0b11;
            sgb->attributes[x + y * 20] = val;
        }
    }
}

static void sgb_PAL01(sgb_t* sgb) { sgb_pal(sgb, 0, 1); }
static void sgb_PAL23(sgb_t* sgb) { sgb_pal(sgb, 2, 3); }
static void sgb_PAL03(sgb_t* sgb) { sgb_pal(sgb, 0, 3); }
static void sgb_PAL12(sgb_t* sgb) { sgb_pal(sgb, 1, 2); }

static void sgb_ATTR_BLK(sgb_t* sgb) {
    int n_data = sgb->packets[1];
    if (!n_data || n_data > 0x12)
        return;
    
    u8* bytes = sgb->packets + 2;

    for (int i = 0; i < n_data; i++) {
        u8 control_code = (*bytes++);
        bool is_inside = control_code & (1 << 0);
        bool is_border = control_code & (1 << 1);
        bool is_outside = control_code & (1 << 2);
        
        u8 color_palette = (*bytes++);
        u8 pal_inside = color_palette & 0b11;
        u8 pal_border = (color_palette >> 2) & 0b11;
        u8 pal_outside = (color_palette >> 4) & 0b11;

        u8 x0 = (*bytes++) & 0x1F;
        u8 y0 = (*bytes++) & 0x1F;
        u8 x1 = (*bytes++) & 0x1F;
        u8 y1 = (*bytes++) & 0x1F;

        if (is_inside && !is_outside && !is_border) {
            is_border = true;
            pal_border = pal_inside;
        }

        if (is_outside && !is_inside && !is_border) {
            is_border = true;
            pal_border = pal_outside;
        }

        for (int y = 0; y < 18; y++) {
            for (int x = 0; x < 20; x++) {
                bool inside = x > x0 && x < x1 && y > y0 && y < y1;
                bool outside = x < x0 || x > x1 || y < y0 || y > y1;
                bool border = !inside && !outside;

                if (is_inside && inside)
                    sgb->attributes[x + y * 20] = pal_inside;

                if (is_outside && outside)
                    sgb->attributes[x + y * 20] = pal_outside;

                if (is_border && border)
                    sgb->attributes[x + y * 20] = pal_border;
            }
        }
    }
}

static void sgb_ATTR_LIN(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }
static void sgb_ATTR_DIV(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }
static void sgb_ATTR_CHR(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }
static void sgb_SOUND(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }
static void sgb_SOU_TRN(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }

static void sgb_PAL_SET(sgb_t* sgb) {
    u8 flags = sgb->packets[9];
    bool apply_atf = flags & (1 << 7);
    bool cancel_mask = flags & (1 << 6);
    if (apply_atf) {
        u8 atf_number = flags & 0x3F;
        load_atf(sgb, atf_number);
    }
    if (cancel_mask) {
        sgb->mask = SGB_MASK_NONE;
    }
    
    for (int i = 0; i < 4; i++) {
        u16 pal = sgb->packets[1 + i * 2] | (sgb->packets[2 + i * 2] << 8);
        pal &= 0x1FF;
        for (int j = 0; j < 4; j++)
            sgb->colors[i * 4 + j] = sgb->pal_ram[pal * 4 + j];
        sgb->colors[i * 4] = sgb->colors[0];
    }
}

static void sgb_PAL_TRN(sgb_t* sgb) { sgb->vram_transfer = SGB_VRAM_TRANSFER_PAL; }

static void sgb_ATRC_EN(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }
static void sgb_TEST_EN(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }
static void sgb_ICON_EN(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }
static void sgb_DATA_SND(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }
static void sgb_DATA_TRN(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }

static void sgb_MLT_REQ(sgb_t* sgb) {
    sgb->multiplayer_enabled = sgb->packets[1] & 0x01;
    if (!sgb->multiplayer_enabled)
        return;

    sgb->multiplayer_mode = (sgb->packets[1] & 0b10) ? 4 : 2;
    sgb->player_count = (sgb->player_count - 1) & (sgb->multiplayer_mode - 1);
}

static void sgb_JUMP(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }

static void sgb_CHR_TRN(sgb_t* sgb) {
    bool tile_dst = sgb->packets[1] & 0x01;
    sgb->vram_transfer = tile_dst ? SGB_VRAM_TRANSFER_TILE_80_FF : SGB_VRAM_TRANSFER_TILE_00_7F;
}

static void sgb_PCT_TRN(sgb_t* sgb) {
    sgb->vram_transfer = SGB_VRAM_TRANSFER_TILEMAP;
}

static void sgb_ATTR_TRN(sgb_t* sgb) { sgb->vram_transfer = SGB_VRAM_TRANSFER_ATTR; }

static void sgb_ATTR_SET(sgb_t* sgb) {
    u8 byte = sgb->packets[1];
    bool mask_cancel = byte & (1 << 6);
    u8 atf = byte & 0x3F;
    if (mask_cancel)
        sgb->mask = SGB_MASK_NONE;
    load_atf(sgb, atf);
}

static void sgb_MASK_EN(sgb_t* sgb) {
    SGB_MASK old_mask = sgb->mask;
    sgb->mask = sgb->packets[1] & 0b11;
}

static void sgb_OBJ_TRN(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }
static void sgb_PAL_PRI(sgb_t* sgb) { printf("%s: NOT IMPLEMENTED!\n", __func__); }