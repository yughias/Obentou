#include "cores/tms80/vdp.h"
#include "SDL_MAINLOOP.h"

#define SMS_RENDER_TILES_LINE(startX, endX) \
for(int screenX = startX; screenX < endX; screenX++){ \
    u8 x = h_scroll_lock && screenY < 16 ? screenX : screenX - vdp->regs[8]; \
    int tx = x >> 3; \
    int px = x & 7; \
    u16 tile_attr = tilemap[tx + ty * 32]; \
    bool priority = tile_attr & (1 << 12); \
    u8 col_idx = vdp_sms_get_tile_color_index(vdp, px, py, tile_attr); \
    drawn_pixels[screenX] = priority && (col_idx & 0xF); \
    vdp->framebuffer[screenX + screenY * SCREEN_WIDTH_SMS] = vdp_get_color(vdp, col_idx); \
}

static u8 sms_vdp_skip_bios[16] = {
    0x36, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0x0, 0x0, 0x0, 0xFF, 0x0, 0x0, 0x0, 0x0, 0x0
};

static u8 rgb_colors_sg[16][3] = {
    {0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00},
    {0x20, 0xC0, 0x20},
    {0x60, 0xE0, 0x60},
    {0x20, 0x20, 0xE0},
    {0x40, 0x60, 0xE0},
    {0xA0, 0x20, 0x20},
    {0x40, 0xC0, 0xE0},
    {0xE0, 0x20, 0x20},
    {0xE0, 0x60, 0x60},
    {0xC0, 0xC0, 0x20},
    {0xC0, 0xC0, 0x80},
    {0x20, 0x80, 0x20},
    {0xC0, 0x40, 0xA0},
    {0xA0, 0xA0, 0xA0},
    {0xE0, 0xE0, 0xE0}
};

static u8 rgb_colors_sms[64][3] = {
    { 0, 0, 0 },
    { 85, 0, 0 },
    { 170, 0, 0 },
    { 255, 0, 0 },
    { 0, 85, 0 },
    { 85, 85, 0 },
    { 170, 85, 0 },
    { 255, 85, 0 },
    { 0, 170, 0 },
    { 85, 170, 0 },
    { 170, 170, 0 },
    { 255, 170, 0 },
    { 0, 255, 0 },
    { 85, 255, 0 },
    { 170, 255, 0 },
    { 255, 255, 0 },
    { 0, 0, 85 },
    { 85, 0, 85 },
    { 170, 0, 85 },
    { 255, 0, 85 },
    { 0, 85, 85 },
    { 85, 85, 85 },
    { 170, 85, 85 },
    { 255, 85, 85 },
    { 0, 170, 85 },
    { 85, 170, 85 },
    { 170, 170, 85 },
    { 255, 170, 85 },
    { 0, 255, 85 },
    { 85, 255, 85 },
    { 170, 255, 85 },
    { 255, 255, 85 },
    { 0, 0, 170 },
    { 85, 0, 170 },
    { 170, 0, 170 },
    { 255, 0, 170 },
    { 0, 85, 170 },
    { 85, 85, 170 },
    { 170, 85, 170 },
    { 255, 85, 170 },
    { 0, 170, 170 },
    { 85, 170, 170 },
    { 170, 170, 170 },
    { 255, 170, 170 },
    { 0, 255, 170 },
    { 85, 255, 170 },
    { 170, 255, 170 },
    { 255, 255, 170 },
    { 0, 0, 255 },
    { 85, 0, 255 },
    { 170, 0, 255 },
    { 255, 0, 255 },
    { 0, 85, 255 },
    { 85, 85, 255 },
    { 170, 85, 255 },
    { 255, 85, 255 },
    { 0, 170, 255 },
    { 85, 170, 255 },
    { 170, 170, 255 },
    { 255, 170, 255 },
    { 0, 255, 255 },
    { 85, 255, 255 },
    { 170, 255, 255 },
    { 255, 255, 255 }
};

u8 tms80_vdp_get_v_counter(vdp_t* vdp){
    switch(vdp->region){
        case REGION_NTSC:
        return vdp->v_counter > 0xDA ? vdp->v_counter - 0x06 : vdp->v_counter;

        case REGION_PAL:
        return vdp->v_counter > 0xF2 ? vdp->v_counter - 0x39 : vdp->v_counter;
    }
}

void tms80_vdp_write_to_status_register(vdp_t* vdp){
    u8 reg_number = (vdp->control_port >> 8) & 0b1111;
    u8 data = vdp->control_port & 0xFF;

    vdp->regs[reg_number] = data;
}

void tms80_vdp_fill_buffer(vdp_t* vdp){
    vdp->buffer = vdp->VRAM[vdp->vram_address];
    vdp->vram_address = (vdp->vram_address + 1) & 0x3FFF;
}

u8 tms80_vdp_read_from_data_port(vdp_t* vdp){
    u8 out = vdp->buffer;
    tms80_vdp_fill_buffer(vdp);
    vdp->control_port_flag = 0;
    return out;
}

void tms80_vdp_write_to_data_port(vdp_t* vdp, u8 byte){
    if(vdp->cram_dst){
        if(vdp->cram_size == CRAM_SIZE_GG){
            if(vdp->vram_address & 1){
                vdp->CRAM[(vdp->vram_address & ~1) & (vdp->cram_size - 1)] = vdp->cram_latch;
                vdp->CRAM[vdp->vram_address & (vdp->cram_size - 1)] = byte;  
            } else 
                vdp->cram_latch = byte;
        } else
            vdp->CRAM[vdp->vram_address & (vdp->cram_size - 1)] = byte;
    } else
        vdp->VRAM[vdp->vram_address] = byte;

    vdp->buffer = byte;
    vdp->vram_address = (vdp->vram_address + 1) & 0x3FFF;
    vdp->control_port_flag = 0;
}

void tms80_vdp_write_to_control_port(vdp_t* vdp, u8 byte){
    if(vdp->control_port_flag){
        vdp->control_port &= 0xFF;
        vdp->control_port |= byte << 8;

        vdp->vram_address &= 0xFF;
        vdp->vram_address |= (byte & 0x3F) << 8;

        switch (vdp->control_port >> 14){
            case 0b00:
            tms80_vdp_fill_buffer(vdp);
            vdp->cram_dst = false;
            break;

            case 0b01:
            vdp->cram_dst = false;
            break;

            case 0b10:
            tms80_vdp_write_to_status_register(vdp);
            vdp->cram_dst = false;
            break;

            case 0b11:
            vdp->cram_dst = true;
            break;
        }
    } else {
        vdp->control_port &= ~0xFF;
        vdp->control_port |= byte;

        vdp->vram_address &= ~0xFF;
        vdp->vram_address |= byte;
    }

    vdp->control_port_flag ^= 1;
}

static u16 vdp_get_tile_map_addr(vdp_t* vdp){
    return (vdp->regs[2] & 0xF) << 10;
}

static u16 vdp_get_color_table_addr(vdp_t* vdp, int mode){
    u8 mask = (mode == 4) ? 0x80 : 0xFF;
    return (vdp->regs[3] & mask) * 0x40;
}

static u16 vdp_get_tile_data_addr(vdp_t* vdp, int mode){
    u8 mask = (mode == 4) ? 0b100 : 0b111;
    return (vdp->regs[4] & mask) * 0x800;
}

static void vdp_render_tiles_legacy(vdp_t* vdp, int mode, int line){
    u16 tile_map_addr = vdp_get_tile_map_addr(vdp);
    u16 color_table_addr = vdp_get_color_table_addr(vdp, mode);
    u16 tile_data_addr = vdp_get_tile_data_addr(vdp, mode);

    u8* tilemap = &vdp->VRAM[tile_map_addr];
    u8* color_table = &vdp->VRAM[color_table_addr];

    u8 pg_mask = vdp->regs[4] & 0b11;
    u8 ct_mask = vdp->regs[3] & 0x7F;

    int y = line >> 3;
    int yy = line & 7;

    for(int x = 0; x < 32; x++){
        u16 tile_idx = tilemap[x + y * 32];
        if(mode == 4)
            tile_idx += 0x100 * (y/8);
        
        u16 pg_idx = tile_idx;
        if(mode == 4)
            pg_idx = (tile_idx & (pg_mask << 8)) | (tile_idx & 0xFF);

        u8* tile_data = &vdp->VRAM[tile_data_addr + pg_idx*8];
        
        u16 ct_idx = tile_idx;
        if(mode == 4)
            ct_idx = (tile_idx & (ct_mask << 3)) | (tile_idx & 0b111);
        
        u8 row = tile_data[yy];
        u8 color_data = color_table[mode == 4 ? (ct_idx << 3) + yy : ct_idx >> 3];
        u8 background_col = color_data & 0xF;
        u8 foreground_col = color_data >> 4;

        for(int xx = 0; xx < 8; xx++){
            u8 pos = 7-xx;
            bool color_type = (row & (1 << pos));
            u8 color_idx = color_type ? foreground_col : background_col;
            u8* rgb_color = rgb_colors_sg[ color_idx ? color_idx : vdp->regs[7] & 0xF ];
            vdp->framebuffer[(x*8+xx) + (y*8+yy)*SCREEN_WIDTH_SMS] = color(rgb_color[0], rgb_color[1], rgb_color[2]); 
        }
    }
}

static void vdp_put_sprite_pixel(int x, int y, vdp_t* vdp, bool* collision_array, bool* drawn_pixels, bool color_type, int col){
    if(x < 0 || x >= SCREEN_WIDTH_SMS)
        return;
    
    if(y >= SCREEN_HEIGHT_SMS)
        return;

    if(collision_array[x])
        vdp->status_reg |= 0x20;
    else 
        collision_array[x] = true;

    if(color_type && !drawn_pixels[x]){
        vdp->framebuffer[x + y*SCREEN_WIDTH_SMS] = col; 
        drawn_pixels[x] = true;
    }
}

static void vdp_render_text_legacy(vdp_t* vdp, int line){
    u8 background_idx = vdp->regs[7] & 0xF;
    u8 foreground_idx = vdp->regs[7] >> 4;
    u8* background_rgb = rgb_colors_sg[background_idx];
    u8* foreground_rgb = rgb_colors_sg[foreground_idx];
    int background_col = color(background_rgb[0], background_rgb[1], background_rgb[2]);
    int foreground_col = color(foreground_rgb[0], foreground_rgb[1], foreground_rgb[2]);
    int y = line >> 3;
    int yy = line & 7;
    u16 tilemap_addr = vdp_get_tile_map_addr(vdp);
    u16 tile_data_addr = vdp_get_tile_data_addr(vdp, 2);
    u8* tilemap = &vdp->VRAM[tilemap_addr];

    for(int x = 0; x < 8; x++){
        vdp->framebuffer[x + line * SCREEN_WIDTH_SMS] = background_col;
        vdp->framebuffer[(SCREEN_WIDTH_SMS-1-x) + line * SCREEN_WIDTH_SMS] = background_col;
    }

    for(int i = 0; i < 40; i++){
        u8 tile_idx = tilemap[i + y * 40];
        u8* tile_data = &vdp->VRAM[tile_data_addr + tile_idx*8];
        u8 row = tile_data[yy];

        for(int x = 0; x < 6; x++){
            u8 pos = 7-x;
            bool color_type = (row & (1 << pos));
            int col = color_type ? foreground_col : background_col;
            vdp->framebuffer[8 + (i*6+x) + line * SCREEN_WIDTH_SMS] = col;
        }
    }
}

static void vdp_render_sprites_legacy(vdp_t* vdp, int line){
    bool collision_array[SCREEN_WIDTH_SMS] = { [0 ... 255] = false };
    bool drawn_pixels[SCREEN_WIDTH_SMS] = { [0 ... 255] = false };

    u8 sprite_size = (vdp->regs[1] & 0b10) ? 16 : 8;
    bool mag_bit = vdp->regs[1] & 0b1;

    u16 sprite_attribute_addr = (vdp->regs[5] & 0x7F) * 0x80;
    u16 sprite_pattern_addr = (vdp->regs[6] & 0b111) * 0x800;
    u8* sprite_attributes = &vdp->VRAM[sprite_attribute_addr];
    u8* sprite_patterns = &vdp->VRAM[sprite_pattern_addr];

    for(int i = 0; i < 32; i++){
        u8* sprite_attribute = &sprite_attributes[i*4];
        int y = sprite_attribute[0];
        if(y == 208)
            return;
        y += 1;
        if(y >= 240)
            y -= 256;
        int x = sprite_attribute[1];
        u8 tile_idx = sprite_attribute[2];
        u8 color_data = sprite_attribute[3] & 0xF;
        if(sprite_attribute[3] & 0x80)
            x -= 32;

        if(line < y || line >= y + (sprite_size << mag_bit))
            continue;

        if(!color_data)
            continue;

        int yy = line - y;
        if(mag_bit)
            yy >>= 1;
        u8* tile_data = &sprite_patterns[sprite_size == 8 ? tile_idx*8 : (tile_idx & 252)*8];
        u8* rgb_color = rgb_colors_sg[color_data];
        int col = color(rgb_color[0], rgb_color[1], rgb_color[2]);

        for(int xx = 0; xx < sprite_size; xx++){
            u8 row = tile_data[xx < 8 ? yy : yy+16];
            u8 pos = 7 - (xx & 7);
            bool color_type = (row & (1 << pos));

            if(!mag_bit) {
                vdp_put_sprite_pixel(x+xx, line, vdp, collision_array, drawn_pixels, color_type, col);
            } else {
                vdp_put_sprite_pixel(x+xx*2,   line, vdp, collision_array, drawn_pixels, color_type, col);
                vdp_put_sprite_pixel(x+xx*2+1, line, vdp, collision_array, drawn_pixels, color_type, col);
            }   
        }
    }    
}

static void vdp_render_line_backdrop(vdp_t* vdp, int y){
    u8 backdrop_idx = vdp->regs[7] & 0xF;
    u8* rgb_color = rgb_colors_sg[backdrop_idx];
    int col = color(rgb_color[0], rgb_color[1], rgb_color[2]);
    for(int x = 0; x < SCREEN_WIDTH_SMS; x++)
        vdp->framebuffer[x + y * SCREEN_WIDTH_SMS] = col;
}

static u8 vdp_sms_get_tile_color_index(vdp_t* vdp, u8 x, u8 y, u16 tile_attr){
    int tile_idx = tile_attr & 0x1FF;
    u8* tile_data = &vdp->VRAM[tile_idx * 32];
    bool sprite_palette = tile_attr & (1 << 11);
    bool flipX = tile_attr & (1 << 9);
    bool flipY = tile_attr & (1 << 10);
    if(flipX)
        x = 7 - x;
    if(flipY)
        y = 7 - y;

    u8 out = 0;
    for(int i = 0; i < 4; i++){
        u8 bitmask = 1 << (7 - x);
        bool bit = tile_data[i + y*4] & bitmask; 
        out |= bit << i;
    }
    return out | (sprite_palette * 0x10);
}

static u8 vdp_sms_get_sprite_color_index(vdp_t* vdp, u8 x, u8 y, u8* tile_data){
    u8 out = 0;
    for(int i = 0; i < 4; i++){
        u8 bitmask = 1 << (7 - x);
        bool bit = tile_data[i + y*4] & bitmask; 
        out |= bit << i;
    }
    return out | 0x10;
}

static int sms_get_color(vdp_t* vdp, int col_idx){
    u8 rgb222 = vdp->CRAM[col_idx] & 0x3F;
    u8* rgb = rgb_colors_sms[rgb222];
    int col = color(rgb[0], rgb[1], rgb[2]); 
    return col;
}

static int gg_get_color(vdp_t* vdp, int col_idx){
    u8 r = vdp->CRAM[col_idx*2] & 0xF;
    u8 g = vdp->CRAM[col_idx*2] >> 4;
    u8 b = vdp->CRAM[col_idx*2+1] & 0xF;
    r |= (r << 4);
    g |= (g << 4);
    b |= (b << 4);
    int col = color(r, g, b); 
    return col;
}

static int vdp_get_color(vdp_t* vdp, int col_idx){
    if(vdp->cram_size == CRAM_SIZE_SMS)
        return sms_get_color(vdp, col_idx);
    return gg_get_color(vdp, col_idx);
}

static void vdp_render_tiles_mode_4(vdp_t* vdp, int y, bool* drawn_pixels){
    u16 tilemap_addr = ((vdp->regs[2] >> 1) & 0b111) * 0x800;
    u16* tilemap = (u16*)(&vdp->VRAM[tilemap_addr]);
    bool h_scroll_lock = vdp->regs[0] & (1 << 6); 
    bool v_scroll_lock = vdp->regs[0] & (1 << 7);
    int screenY = y;
    int startX = vdp->regs[0] & (1 << 5) ? 8 : 0;
    y = (y + vdp->scroll_y) % 224;
    int ty = y >> 3;
    int py = y & 7;

    if(startX){
        int backdrop_col = vdp_get_color(vdp, 0x10 | (vdp->regs[7] & 0xF));
        for(int i = 0; i < 8; i++)
            vdp->framebuffer[i + screenY * SCREEN_WIDTH_SMS] = backdrop_col;
    }

    SMS_RENDER_TILES_LINE(startX, 8*24);

    if(v_scroll_lock){
        ty = screenY >> 3;
        py = screenY & 7;
    }

    SMS_RENDER_TILES_LINE(8*24, SCREEN_WIDTH_SMS);
}

static void vdp_render_sprites_mode_4(vdp_t* vdp, int y, bool* drawn_pixels){
    bool collision_array[SCREEN_WIDTH_SMS] = { [0 ... 255] = false };

    u16 sprite_table_addr = ((vdp->regs[5] >> 1) & 0x3F) << 8;
    u8* sprite_table = &vdp->VRAM[sprite_table_addr]; 
    u16 sprite_tiles_addr = (vdp->regs[6] & (1 << 2)) << 11;
    u8* sprite_tiles = &vdp->VRAM[sprite_tiles_addr];
    int sprite_height = vdp->regs[1] & 0b10 ? 16 : 8;
    bool hide_left_col = vdp->regs[0] & (1 << 5);
    int shift_x = vdp->regs[0] & (1 << 3) ? -8 : 0;

    for(int i = 0; i < 64; i++){
        int startX = sprite_table[128 | (i << 1)] + shift_x;
        int startY = sprite_table[i];
        if(startY == 208){
            return;
        }
        startY += 1;
        if(startY >= 240)
            startY -= 256;
        if(sprite_table[i] == 208)
            return;
        int py = y - startY;
        if(py < 0 || py >= sprite_height)
            continue;
        u8 sprite_idx = sprite_table[128 | 1 | (i << 1)];
        if(sprite_height == 16)
            sprite_idx = (sprite_idx & ~0b1) | (py >= 8);
        u8* sprite_tile = &sprite_tiles[sprite_idx * 32];
        for(int px = 0; px < 8; px++){
            int offX = startX + px;
            if(hide_left_col && offX < 8)
                continue;
            u8 col_idx = vdp_sms_get_sprite_color_index(vdp, px, py & 7, sprite_tile);
            int col = vdp_get_color(vdp, col_idx);
            vdp_put_sprite_pixel(offX, y, vdp, collision_array, drawn_pixels, col_idx != 0x10, col);
        }
    }
}

void tms80_vdp_render_line(vdp_t* vdp, int y){
    u8 mode = 0;
    mode |= (vdp->regs[1] >> 3) & 0b11;
    mode |= ((vdp->regs[0] >> 1) & 0b1) << 2;

    if(!(vdp->regs[1] & (1 << 6))){
        vdp_render_line_backdrop(vdp, y);
        return;
    }

    if(vdp->regs[0] & (1 << 2)){
        bool drawn_pixels[SCREEN_WIDTH_SMS] = { [0 ... 255] = false };
        vdp_render_tiles_mode_4(vdp, y, drawn_pixels);
        vdp_render_sprites_mode_4(vdp, y, drawn_pixels);
        return;
    }

    if(mode == 2){
        vdp_render_text_legacy(vdp, y);
    } else {
        vdp_render_tiles_legacy(vdp, mode, y);
        vdp_render_sprites_legacy(vdp, y);
    }
}

void tms80_vdp_fire_interrupt(vdp_t* vdp, z80_t* z80, bool is_vblank){
    vdp->status_reg |= (is_vblank << 7);
    
    if(is_vblank && !(vdp->regs[1] & (1 << 5)))
        return;

    if(!is_vblank && !(vdp->regs[0] & (1 << 4)))
        return;


    z80->INTERRUPT_PENDING = true;
}

u8 tms80_vdp_read_status_register(vdp_t* vdp, z80_t* z80){
    u8 out = vdp->status_reg;
    vdp->status_reg &= 0x1F;
    vdp->control_port_flag = 0;
    z80->INTERRUPT_PENDING = false;
    return out;
}

void tms80_vdp_skip_bios(vdp_t* vdp){
    memcpy(vdp->regs, sms_vdp_skip_bios, sizeof(sms_vdp_skip_bios));
}

void tms80_vdp_show_frame(const vdp_t* vdp){
    background(0);
    if(vdp->cram_size == CRAM_SIZE_GG){
        for(int y = 0; y < height; y++)
            for(int x = 0; x < width; x++)
                pixels[x+y*width] = vdp->framebuffer[(x+GG_START_X) + (y+GG_START_Y) * SCREEN_WIDTH_SMS];
    } else {
        for(int i = 0; i < width*height; i++)
            pixels[i] = vdp->framebuffer[i];
    }
    renderPixels();
}