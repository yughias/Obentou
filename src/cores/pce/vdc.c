#include "cores/pce/vdc.h"
#include "cores/pce/timings.h"
#include "cores/pce/pce.h"

static u8 tilemap_size_lut[8][2] = {
    {32,  32},  {64,  32},
    {128, 32},  {128, 32},
    {32,  64},  {64,  64},
    {128, 64},  {128, 64}
};

static u8 vdc_inc_lut[4] = {1, 32, 64, 128};

static u16 inline vdc_read_vram(vdc_t* v){
    if((v->regs[MARR] << 1) >= VRAM_SIZE) {
        printf("vdc_read_vram: Invalid VRAM address 0x%04X\n", v->regs[MARR]);
        return 0;
    }
    
    u16 addr = v->regs[MARR] << 1;
    return v->vram[addr] | (v->vram[addr | 1] << 8);
}

static u8 inline vdc_get_increment(vdc_t* v){
    u8 lut_idx = (v->regs[CR] >> 11) & 0b11;
    return vdc_inc_lut[lut_idx];
}

void pce_vdc_get_tilemap_size(const vdc_t* v, u8* w, u8* h){
    u8 lut_idx = (v->regs[MWR] >> 4) & 0b111;
    *w = tilemap_size_lut[lut_idx][0];
    *h = tilemap_size_lut[lut_idx][1];
}

static int get_render_width(const vdc_t* vdc){
    int hdw = vdc->regs[HDR] & 0x7F;
    int render_width = (hdw + 1) << 3;

    return render_width < width ? render_width : width;
}

static int get_start_render_line(const vdc_t* vdc){
    int vsw = vdc->regs[VPR] & 0x1F;
    int vds = vdc->regs[VPR] >> 8;

    return vsw + vds;
}

static int get_end_render_line(const vdc_t* vdc){
    int vsw_vds = get_start_render_line(vdc);
    // int vcr = vdc->regs[VCR] & 0xFF; not used?
    int vdw = vdc->regs[VDW] & 0x1FF;

    return vsw_vds + vdw;
}

static void vdc_dma_vram_vram_transfer(vdc_t* v){
    u16 src = v->regs[SOUR] << 1;
    u16 dst = v->regs[DESR] << 1;
    int len = (v->regs[LENR] + 1) << 1;
    int src_dir = v->regs[DCR] & (1 << 2) ? -1 : +1;
    int dst_dir = v->regs[DCR] & (1 << 3) ? -1 : +1;
    
    for(int i = 0; i < len; i++){
        v->vram[dst] = v->vram[src];
        src += src_dir;
        dst += dst_dir;
    }

    if(v->regs[DCR] & (1 << 1)){
        pce_notify_event((pce_t*)v->ctx, 0, 255, 0);
        v->status |= VDC_STAT_DMA;
        v->irq = true;
    }
}

void pce_vdc_write_reg(vdc_t* v, u8 idx, u8 value, bool hi){
    v->regs[idx] = hi ? (v->regs[idx] & 0x00FF) | (value << 8) : (v->regs[idx] & 0xFF00) | value;

    if(idx == MARR && hi){
        v->read_buffer = vdc_read_vram(v);
        v->regs[MARR] += vdc_get_increment(v);
        return;
    }

    if(idx == VRR_VWR && hi){
        pce_notify_event((pce_t*)v->ctx, 128, 0, 128);
        if((v->regs[MAWR] << 1) >= VRAM_SIZE){
            printf("vdc_write_reg: Invalid VRAM address 0x%04X\n", v->regs[MAWR]);
        } else {
            u16 addr = (v->regs[MAWR] << 1);
            v->vram[addr] = v->regs[VRR_VWR];
            v->vram[addr | 1] = v->regs[VRR_VWR] >> 8;
            v->regs[MAWR] += vdc_get_increment(v);
        }
        return;
    }

    if(idx == DCR){
        v->satb_dma = v->regs[DCR] & (1 << 4);
        return;
    }

    if(idx == BYR){
        // hack
        v->yscroll = (v->regs[BYR] + 1) & 0x1FF;
        return;
    }

    if(idx == SATB){
        v->satb_dma = true;
        return;
    }

    if(idx == LENR && hi){
        vdc_dma_vram_vram_transfer(v);
        return;
    }
}

u8 pce_vdc_read_reg(vdc_t* v, u8 idx, bool hi){
    u8 out = hi ? v->read_buffer >> 8 : v->read_buffer;
    if(hi){
        v->read_buffer = vdc_read_vram(v);
        v->regs[MARR] += vdc_get_increment(v);
    }    

    if(idx != VRR_VWR)
        printf("vdc_read_reg: Unhandled read from address 0x%02X\n", idx);
    return out;
}

void pce_vdc_write_io(vdc_t* v, u8 offset, u8 value){
    bool a0 = offset & 0b01;
    bool a1 = offset & 0b10;

    if(!a1){
        if(!a0)
            v->selector = value & 0x1F;
    } else {
        pce_vdc_write_reg(v, v->selector, value, a0); 
    }
}

u8 pce_vdc_read_io(vdc_t* v, u8 offset){
    bool a0 = offset & 0b01;
    bool a1 = offset & 0b10;

    if(!a1){
        if(!a0) {
            v->irq = false;
            u8 out = v->status;
            v->status = 0;
            return out;
        } else {
            return 0;
        }
    } else {
        return pce_vdc_read_reg(v, v->selector, a0);
    }
}

bool pce_vdc_vblank_on(const vdc_t* v){
    return v->regs[CR] & (1 << 3);
}

bool pce_vdc_rcr_on(const vdc_t* v){
    return v->regs[CR] & (1 << 2);
}

bool pce_vdc_background_on(const vdc_t* v){
    return v->regs[CR] & (1 << 7);
}

bool pce_vdc_sprites_on(const vdc_t* v){
    return v->regs[CR] & (1 << 6);
}

static void vdc_event_start_line(vdc_t* v){
    int start_render_line = get_start_render_line(v);

    if(v->frame_counter == start_render_line){
        v->display_counter = 0;
        v->yscroll = v->regs[BYR] & 0x1FF;
    }

    if(v->frame_counter == get_end_render_line(v)){
        if(pce_vdc_vblank_on(v)){
            pce_notify_event((pce_t*)v->ctx, 255, 255, 255);
            v->irq = true;
            v->status |= VDC_STAT_VBLANK;
        }
    }

    if(v->frame_counter == get_end_render_line(v) + 3){
        if(v->satb_dma){
            if(!(v->regs[DCR] & (1 << 4)))
                v->satb_dma = false;
            memcpy(v->satb, v->vram + (v->regs[SATB] << 1), sizeof(v->satb));
            v->status |= VDC_STAT_SATB;
            if(v->regs[DCR] & (1 << 0)){
                v->irq = true;
                pce_notify_event((pce_t*)v->ctx, 120, 120, 120);
            }
        }
    }

    v->next_event = END_LINE;
    v->next_event_cycles = CYCLES_PER_SCANLINE - CYCLES_PER_HBLANK;
}

static void vdc_event_end_line(vdc_t* v){
    if(pce_vdc_rcr_on(v) && v->display_counter == ((v->regs[RCR] & 0x3FF) - 64)){
        pce_notify_event((pce_t*)v->ctx, 255, 0, 0);
        v->irq = true;
        v->status |= VDC_STAT_RCR;
    }

    if(v->frame_counter >= TOP_OVERSCAN && v->frame_counter < SCREEN_HEIGHT + TOP_OVERSCAN){
        pce_vdc_render_line(v, v->vce);
    }

    v->display_counter += 1; 
    v->frame_counter = (v->frame_counter + 1) % TOTAL_SCANLINES;

    v->next_event = START_LINE;
    v->next_event_cycles = CYCLES_PER_HBLANK;
}

void pce_vdc_step(vdc_t* v, u32 cycles){
    while(cycles){
        u32 to_substract = cycles > v->next_event_cycles ? v->next_event_cycles : cycles;
        v->next_event_cycles -= to_substract;
        cycles -= to_substract;

        while(!v->next_event_cycles){
            switch(v->next_event){
                case START_LINE:
                vdc_event_start_line(v);
                break;

                case END_LINE:
                vdc_event_end_line(v);
                break;
            }
        }
    } 
}

static u8 get_tile_col_idx(const vdc_t* vdc, u16 tile_idx, u8 x, u8 y){
    u16 base_addr = tile_idx << 5;
    u8 x_mask = 1 << (7-x);
    bool bp0 = vdc->vram[base_addr + y * 2] & x_mask;
    bool bp1 = vdc->vram[base_addr + y * 2 + 1] & x_mask;
    bool bp2 = vdc->vram[base_addr + y * 2 + 16] & x_mask;
    bool bp3 = vdc->vram[base_addr + y * 2 + 17] & x_mask;
    return (bp0 << 0) | (bp1 << 1) | (bp2 << 2) | (bp3 << 3);
}

void pce_vdc_render_background(vdc_t* vdc, const vce_t* vce){
    u8 tw, th;
    pce_vdc_get_tilemap_size(vdc, &tw, &th);
    int w = tw << 3;
    int h = th << 3;

    int bgx = vdc->regs[BXR] & 0x3FF;
    int render_width = get_render_width(vdc);

    int y = vdc->yscroll;
    int screen_line = vdc->frame_counter - TOP_OVERSCAN;
    for(int i = 0; i < render_width; i++){
        int x = (i + bgx) % w;
        u8 tx = x >> 3;
        u8 ty = y >> 3;
        u16 tile_attr_addr = (tx + ty * tw) << 1;
        u16 tile_attr = vdc->vram[tile_attr_addr] | (vdc->vram[tile_attr_addr | 1] << 8);
        u16 tile_idx = tile_attr & ((1 << 12) - 1);
        u8 pal_idx = tile_attr >> 12;
        u8 col_idx = get_tile_col_idx(vdc, tile_idx, x & 7, y & 7);
        int col = pce_vce_get_pal_col(vce, pal_idx, col_idx);
        pixels[i + screen_line * width] = col;
        vdc->bg_buffer[i] = col_idx | (pal_idx << 4);  
    }

    vdc->yscroll = (vdc->yscroll + 1) % h;
}

static u8 vdc_get_sprite_col_idx(vdc_t* v, u16 addr, u8 xs, u8 ys, bool xf, bool yf, u8 x, u8 y){
    u16* data = (u16*)(&v->vram[addr]);
    int real_x = xf ? xs - x - 1 : x;
    int real_y = yf ? ys - y - 1 : y;
    u8 off_x = real_x & 15;
    u8 off_y = real_y & 15;
    u8 tx = real_x >> 4;
    u8 ty = real_y >> 4;
    u8 tile_idx = tx + (ty << 1);
    u16* tile = &data[tile_idx << 6];
    bool bp0 = tile[off_y] & (1 << (15-off_x));
    bool bp1= tile[off_y + 16] & (1 << (15-off_x));
    bool bp2 = tile[off_y + 32] & (1 << (15-off_x));
    bool bp3 = tile[off_y + 48] & (1 << (15-off_x));
    return (bp0 << 0) | (bp1 << 1) | (bp2 << 2) | (bp3 << 3);   
}

void pce_vdc_get_sprite_info(vdc_t* vdc, u8 idx, u16* addr, int* xpos, int* ypos, u8* xs, u8* ys, bool* xf, bool* yf, bool* f, u8* pal){
    u16* sprite_attr = &vdc->satb[idx*4];
    *ypos = sprite_attr[0] & 0x3FF;
    *ypos -= 64;
    *xpos = sprite_attr[1] & 0x3FF;
    *xpos -= 32;
    *addr = sprite_attr[2] << 6;
    *yf = sprite_attr[3] & (1 << 0xF);
    *xf = sprite_attr[3] & (1 << 0xB);
    *ys = ((sprite_attr[3] >> 0xC) & 0b11);
    *f = sprite_attr[3] & (1 << 7);
    switch(*ys){
        case 0b00 : *ys = 16; break;
        case 0b01 : *ys = 32; break;
        default : *ys = 64; 
    }
    *xs = sprite_attr[3] & (1 << 8) ? 32 : 16;
    *pal = sprite_attr[3] & 0x0F; 
}

void pce_vdc_render_sprites(vdc_t* vdc){
    int sprite_count = 0;
    int render_width = get_render_width(vdc);
    int display_line = vdc->display_counter;
    int screen_line = vdc->frame_counter - TOP_OVERSCAN;
    for(int i = 63; i >= 0; i--){
        u8 xs, ys;
        bool xf, yf, f;
        int xpos, ypos;
        u16 addr;
        u8 pal;
        pce_vdc_get_sprite_info(vdc, i, &addr, &xpos, &ypos, &xs, &ys, &xf, &yf, &f, &pal);
        if(ypos > display_line) continue;
        if(ypos + ys <= display_line) continue;
        if(xpos + xs < 0) continue;
        if(xpos >= render_width) continue;
        sprite_count += 1;
        for(int i = xpos < 0 ? 0 : xpos; i < xpos + xs && i < render_width; i++){
            u8 col_idx = vdc_get_sprite_col_idx(vdc, addr, xs, ys, xf, yf, i - xpos, display_line - ypos);
            if(!col_idx)
                continue;
            int col;
            if(!f && (vdc->bg_buffer[i] & 0xF)){
                col = pce_vce_get_pal_col(vdc->vce, vdc->bg_buffer[i] >> 4, vdc->bg_buffer[i] & 0xF);
            } else {
                col = pce_vce_get_pal_col(vdc->vce, pal | 0x10, col_idx);
                if(vdc->sprite_buffer[i]){
                    vdc->status |= VDC_STAT_COLLISION;
                    if(vdc->regs[CR] & 1)
                        vdc->irq = true;
                }
                vdc->sprite_buffer[i] = true;
            }
            pixels[i + screen_line * width] = col;
        }
    }
}

void pce_vdc_render_line(vdc_t* vdc, const vce_t* vce){
    memset(vdc->bg_buffer, 0, sizeof(vdc->bg_buffer));
    memset(vdc->sprite_buffer, 0, sizeof(vdc->sprite_buffer));
    
    int screen_line = vdc->frame_counter - TOP_OVERSCAN;
    int render_width = get_render_width(vdc);
    int start_offset = get_start_render_line(vdc);
    int end_offset =  get_end_render_line(vdc);

    if(vdc->frame_counter >= start_offset && vdc->frame_counter < end_offset){
        if(pce_vdc_background_on(vdc)){
            pce_vdc_render_background(vdc, vce);
        } else {
            for(int i = 0; i < render_width; i++)
                pixels[i + screen_line * width] = pce_vce_get_pal_col(vce, 0, 0);
        }

        if(pce_vdc_sprites_on(vdc))
            pce_vdc_render_sprites(vdc);
    } else {
        for(int i = 0; i < render_width; i++){
            pixels[i + screen_line * width] = pce_vce_get_overscan_col(vce);
        }
    }

    for(int i = render_width; i < width; i++)
        pixels[i + screen_line * width] = pce_vce_get_overscan_col(vce);

    pce_notify_line((pce_t*)vdc->ctx, vdc->frame_counter, &pixels[screen_line * width], width);
}   


static void vdc_get_tile_rgb(vdc_t* v, u16 tile_idx, u8 pal_idx, int* tile_rgb){
    for(int y = 0; y < 8; y++){
        for(int x = 0; x < 8; x++){
            u8 col = get_tile_col_idx(v, tile_idx, x, y);
            tile_rgb[y * 8 + x] = pce_vce_get_pal_col(v->vce, pal_idx, col);
        }
    }
}

void pce_vdc_draw_tilemap(SDL_Window** win, vdc_t* v){
    Uint32 id = SDL_GetWindowID(*win);
    if(!id){
        *win = NULL;
        return;
    }
    u8 w, h;
    pce_vdc_get_tilemap_size(v, &w, &h);
    char title[64];
    sprintf(title, "VDC: %dx%d", w, h);
    SDL_SetWindowTitle(*win, title);
    SDL_SetWindowSize(*win, w*8, h*8);
    SDL_Surface* s = SDL_GetWindowSurface(*win);
    int black = color(0, 0, 0);
    int* pixels = (int*)s->pixels;
    SDL_FillSurfaceRect(s, NULL, black);

    for(int ty = 0; ty < h; ty++){
        for(int tx = 0; tx < w; tx++){
            u16 tile_attr_addr = (tx + ty * w) << 1;
            u16 tile_attr = v->vram[tile_attr_addr] | (v->vram[tile_attr_addr | 1] << 8);
            u16 tile_idx = tile_attr & ((1 << 12) - 1);
            u8 pal_idx = tile_attr >> 12;
            int tile_rgb[64];
            vdc_get_tile_rgb(v, tile_idx, pal_idx, tile_rgb);
            for(int py = 0; py < 8; py++){
                for(int px = 0; px < 8; px++){
                    pixels[(tx*8+px) + (ty*8+py) * s->w] = tile_rgb[px + py * 8];
                }
            }
        }
    }

    SDL_UpdateWindowSurface(*win);
}

void pce_vdc_get_sprite_rgb(vdc_t* v, u16 addr, u8 xs, u8 ys, bool xf, bool yf, u8 pal, int* sprite_rgb){
    for(int y = 0; y < ys; y++){
        for(int x = 0; x < xs; x++){
            u8 col_idx = vdc_get_sprite_col_idx(v, addr, xs, ys, xf, yf, x, y);   
            sprite_rgb[x + y * xs] = pce_vce_get_pal_col(v->vce, pal | 16, col_idx);
        }
    }
}

void pce_vdc_draw_sprites(SDL_Window** win, vdc_t* v){
    Uint32 id = SDL_GetWindowID(*win);
    if(!id){
        *win = NULL;
        return;
    }
    SDL_Surface* s = SDL_GetWindowSurface(*win);
    int black = color(0, 0, 0);
    int* pixels = (int*)s->pixels;
    SDL_FillSurfaceRect(s, NULL, black);

    u16* sat = (u16*)(&v->satb); 

    for(int y = 0; y < 8; y++){
        for(int x = 0; x < 8; x++){
            int idx = x + y * 8;
            u16 addr = sat[idx];
            u8 xs, ys, pal;
            bool xf, yf, f;
            int xpos, ypos;
            pce_vdc_get_sprite_info(v, idx, &addr, &xpos, &ypos, &xs, &ys, &xf, &yf, &f, &pal);
            int sprite_rgb[32*64];
            pce_vdc_get_sprite_rgb(v, addr, xs, ys, xf, yf, pal, sprite_rgb);
            for(int py = 0; py < ys; py++){
                for(int px = 0; px < xs; px++){
                    pixels[(x*32+px) + (y*64+py) * s->w] = sprite_rgb[px + py * xs];
                }
            }
        }
    }

    SDL_UpdateWindowSurface(*win);
}