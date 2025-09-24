#include "cores/pce/pce.h"

#include "peripherals/archive.h"

#include <stdio.h>
#include <stdlib.h>

static void pce_write_regs(void* ctx, u16 offset, u8 value);
static u8 pce_read_regs(void* ctx, u16 offset);

void* PCE_init(const archive_t* rom_archive, const archive_t* bios_archive) {
    pce_t* p = malloc(sizeof(pce_t));
    file_t* rom = archive_get_file_by_ext(rom_archive, "pce");
    p->rom = rom->data;
    p->rom_size = rom->size;

    // strip header
    if(p->rom_size & 512){
        p->rom_size &= ~512;
        p->rom += 512;
    }

    h6280_init(&p->cpu);
    p->cpu.ctx = p;
    p->cpu.read = pce_read;
    p->cpu.write = pce_write;
    p->cpu.io_write = pce_write_regs;

    p->vdc.vce = &p->vce;
    p->vdc.ctx = p;

    p->psg.lfsr[0].seed = 1;
    p->psg.lfsr[1].seed = 1;

    h6280_reset(&p->cpu);

    return p;
}

static u8 pce_read_rom(pce_t* p, u8 bank, u16 offset) {
    u16 size = p->rom_size >> 13;
    u32 addr;
    switch(size){
        // 384Kb 
        case 0x30:
        {
            if(bank < 64){
                bank &= 0x1F;
            } else {
                bank &= 0x0F;
                bank |= 0x20;
            }
        }
        break;

        // 512Kb
        case 0x40:
        {
            if(bank < 64){
                bank &= 0x3F;
            } else {
                bank &= 0x1F;
                bank |= 0x20;
            }
        }
        break;

        // SF2
        case 0x140:
        {
            if(bank >= 0x40){
                u16 sf2_bank = (bank - 0x40) + p->sf2;
                return p->rom[((sf2_bank << 13) | offset) % p->rom_size];
            }
        }
        break;

        default:
        break;
    }

    addr = (bank << 13) | offset;
    return p->rom[addr % p->rom_size];
}

u8 pce_read(void* ctx, u32 addr) {
    pce_t* p = (pce_t*)ctx;
    u8 bank = (addr >> 13);
    u16 offset = (addr & 0x1FFF);

    if(bank < 0x80){
        return pce_read_rom(p, bank, offset);
    }

    if(bank == 0xF7)
        return p->cart_ram[offset];
    
    if(bank == 0xF8)
        return p->ram[offset];

    if(bank == 0xFF)
        return pce_read_regs(ctx, offset);

    printf("pce_read: Unhandled read from address (bank %X) 0x%04X, offset: %04X\n", bank, addr, offset);
    exit(EXIT_FAILURE);
}

void pce_write(void* ctx, u32 addr, u8 value) {
    pce_t* p = (pce_t*)ctx;
    h6280_t* h = &p->cpu;
    u8 bank = (addr >> 13);
    u16 offset = (addr & 0x1FFF);

    if(addr >= 0x1FF0 && addr < 0x1FF4){
        switch(addr & 0b11){
            case 0:
            p->sf2 = 0x40;
            break;

            case 1:
            p->sf2 = 0x80;
            break;

            case 2:
            p->sf2 = 0xC0;
            break;

            case 3:
            p->sf2 = 0x100;
            break;
        }
        return;
    }

    if(bank == 0xF7){
        p->cart_ram[offset] = value;
        return;
    }

    if(bank == 0xF8){
        p->ram[offset] = value;
        return;
    }

    if(bank == 0xFF){
        pce_write_regs(ctx, offset, value);
        return;
    }

    h6280_print(h);
    printf("pce_write: Unhandled write to address 0x%04X (bank: %02X) with value 0x%02X\n", addr, bank, value);
    //exit(EXIT_FAILURE);
}

static void pce_write_regs(void* ctx, u16 offset, u8 value){
    pce_t* p = (pce_t*)ctx;
    vce_t* vce = &p->vce;
    vdc_t* vdc = &p->vdc;
    tmr_t* tmr = &p->timer;
    psg_t* psg = &p->psg;
    controller_t* c = &p->controller;

    if(offset < 0x400){
        pce_vdc_write_io(vdc, offset, value);
        return;
    }

    if(offset >= 0x800 && offset < 0xC00){
        pce_psg_write(psg, offset, value);
        return;
    }

    if(offset == 0x1000){
        pce_controller_write(c, value & 0b11);
        return;
    }

    if(offset == 0x1402){
        p->irq_disable = value;
        return;
    }

    if(offset == 0x1403){
        tmr->irq = false;
        return;
    }

    if(offset == 0x0400){
        vce->ctrl = value;
        return;
    }

    if(offset == 0x0402){
        vce->lo_pal = value;
        return;
    }

    if(offset == 0x0403){
        vce->hi_pal = value & 1;
        return;
    }

    if(offset == 0x0404){
        pce_vce_set_col_lo(vce, value);
        return;
    }

    if(offset == 0x0405){
        pce_vce_set_col_hi(vce, value);
        return;
    }

    if(offset == 0x0C00){
        tmr->reload = value & 0x7F;
        return;
    }

    if(offset == 0x0C01){
        bool enabled = value & 1;
        if(!tmr->enabled && enabled)
            tmr->counter = tmr->reload;
        tmr->enabled = enabled;
        return;
    }

    if(offset >= 0x800 && offset < 0x810)
        return;

    printf("pce_write_regs: Unhandled write to address 0x%04X with value 0x%02X\n", offset, value);
}

static u8 pce_read_regs(void* ctx, u16 offset){
    pce_t* p = (pce_t*)ctx;
    h6280_t* h = &p->cpu;
    vdc_t* vdc = &p->vdc;
    vce_t* vce = &p->vce;

    if(offset < 0x400){
        return pce_vdc_read_io(vdc, offset);
    }

    if(offset == 0x1000){
        return pce_controller_read(&p->controller);
    }

    if(offset == 0x1402){
        return p->irq_disable;
    }

    if(offset == 0x0404){
        return pce_vce_get_col_lo(vce);
    }

    if(offset == 0x0405){
        return pce_vce_get_col_hi(vce);
    }
    
    printf("pce_read_regs: Unhandled read from address 0x%04X\n", offset);
    printf("pc: %X\n", h->pc);
    return 0xFF;
}

void PCE_run_frame(pce_t* p) {
    h6280_t* h = &p->cpu;
    vdc_t* vdc = &p->vdc;
    vce_t* vce = &p->vce;
    tmr_t* tmr = &p->timer;
    psg_t* psg = &p->psg;

    pce_set_resolution(p);

    while(h->cycles < CYCLES_PER_FRAME){
        u32 old_cycles = h->cycles;
        h6280_step(h);

        bool vdc_disabled = p->irq_disable & 0b010;
        bool tmr_disabled = p->irq_disable & 0b100;

        if(tmr_disabled)
            tmr->irq = false;

        if(h6280_interrupt_enabled(h) && tmr->irq && !tmr_disabled){
            pce_notify_event(p, 255, 255, 0);
            h6280_irq(h, 0xFFFA);
        }
        
        if(h6280_interrupt_enabled(h) && vdc->irq && !vdc_disabled){
            h6280_irq(h, 0xFFF8);
        }

        u32 elapsed = h->cycles - old_cycles;
        
        pce_vdc_step(vdc, elapsed);
        pce_tmr_step(tmr, elapsed);
        pce_psg_step(psg, elapsed);
    }

    h->cycles -= CYCLES_PER_FRAME;

    renderPixels();
}

void pce_notify_event(pce_t* gr, u8 r, u8 p, u8 b){
    int cycles = gr->cpu.cycles % CYCLES_PER_FRAME;
    int x = cycles % CYCLES_PER_SCANLINE;
    int y = cycles / CYCLES_PER_SCANLINE;
    for(int yy = y - 1; yy <= y + 1; yy++){
        for(int xx = x - 1; xx <= x + 1; xx++){
            if(xx < 0 || xx >= CYCLES_PER_SCANLINE || yy < 0 || yy >= TOTAL_SCANLINES)
                continue;
            int col;
            if(xx == x && yy == y)
                col = color(r, p, b);
            else
                col = color(r / 2, p / 2, b / 2);
            gr->event_viewer[xx + yy * CYCLES_PER_SCANLINE] = col;
        }
    }
}

void pce_set_resolution(pce_t* p){
    vce_t* vce = &p->vce;
    u8 ctrl = vce->ctrl & 0b11;
    int w;
    switch(ctrl){
        case 0b00:
        w = 256;
        break;

        case 0b01:
        w = 352;
        break;

        case 0b10:
        case 0b11:
        w = 512;
        break;
    }

    if(width != w){
        size(w, SCREEN_HEIGHT);
    }
}

void pce_draw_events(SDL_Window** win, pce_t* p){
    Uint32 id = SDL_GetWindowID(*win);
    if(!id){
        *win = NULL;
        return;
    }
    SDL_Surface* s = SDL_GetWindowSurface(*win);
    int* pix = s->pixels;
    for(int i = 0; i < sizeof(p->event_viewer) / sizeof(int); i++)
        pix[i] = p->event_viewer[i];
    for(int y = 0; y < s->h; y++)
        pix[y * s->w + (CYCLES_PER_SCANLINE - CYCLES_PER_HBLANK)] = color(45, 45, 45);
    SDL_UpdateWindowSurface(*win);
    memset(p->event_viewer, 0, sizeof(p->event_viewer));
}

void pce_notify_line(pce_t* p, int frame_line, int* line, int w){
    const int max_screen_w = CYCLES_PER_SCANLINE - CYCLES_PER_HBLANK;
    for(int i = 0; i < max_screen_w; i++){
        int x = i * w / max_screen_w;
        int idx = i + frame_line * CYCLES_PER_SCANLINE;
        if(!p->event_viewer[idx])
            p->event_viewer[idx] = line[x];
    }
}

bool PCE_detect(const archive_t* rom_archive){
    return archive_get_file_by_ext(rom_archive, "pce");
}