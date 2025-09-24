#include "cores/watara/watara.h"
#include "peripherals/controls.h"
#include "peripherals/archive.h"

#include "SDL_MAINLOOP.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WATARA_STEP(func) \
int cycles = cpu->cycles; \
func (cpu); \
int elapsed = cpu->cycles - cycles; \
watara_tmr_tick(tmr, w->ctrl, elapsed); \
watara_apu_step(apu, elapsed); \
watara_apu_push_sample(apu, elapsed); \
if(w65c02_interrupt_enabled(cpu) && ((tmr->irq | (apu->ch3.irq << 1)) & w->ctrl)) \
    w65c02_irq(cpu);

static u8 watara_get_controller();

static u8 watara_read(void* ctx, u16 addr){
    watara_t* w = (watara_t*)ctx;

    if(addr < 0x2000)
        return w->ram[addr];

    if(addr == 0x2020){
        return watara_get_controller();
    }

    if(addr == 0x2024){
        w->tmr.irq = false;
        return 0xFF;
    }

    if(addr == 0x2025){
        w->apu.ch3.irq = false;
        return 0xFF;
    }

    if(addr == 0x2027){
        u8 irq_status = 0;
        irq_status |= w->tmr.irq | (w->apu.ch3.irq << 1);
        return irq_status;
    }

    if(addr >= 0x4000 && addr < 0x6000)
        return w->lcd.vram[addr - 0x4000];

    if(addr >= 0x8000){
        u8 bank = addr & (1 << 14) ? 0b111 : (w->apu.ch3.dma_active ? (w->apu.ch3.ctrl >> 4) & 0b111 :w->ctrl >> 5);
        u32 rom_addr = addr & ((1 << 14) - 1);
        rom_addr |= bank << 14;
        return w->rom[rom_addr % w->rom_size];
    }

    //printf("invalid read %X\n", addr);
    //exit(EXIT_FAILURE);
    return 0xFF;
}

static void watara_write(void* ctx, u16 addr, u8 byte){
    watara_t* w = (watara_t*)ctx;

    if(addr < 0x2000){
        w->ram[addr] = byte;
        return;
    }

    if(addr >= 0x4000 && addr < 0x6000){
        w->lcd.vram[addr - 0x4000] = byte;
        return;
    }

    if(addr == 0x2000){
        w->lcd.x_size = byte;
        return;
    }

    
    if(addr == 0x2001){
        w->lcd.y_size = byte;
        return;
    }

    
    if(addr == 0x2002){
        w->lcd.x_scroll = byte;
        return;
    }
    
    if(addr == 0x2003){
        w->lcd.y_scroll = byte;
        return;
    }

    if(addr == 0x2008){
        w->dma.src_lo = byte;
        return;
    }

    if(addr == 0x2009){
        w->dma.src_hi = byte;
        return;
    }

    if(addr == 0x200A){
        w->dma.dst_lo = byte;
        return;
    }

    if(addr == 0x200B){
        w->dma.dst_hi = byte;
        return;
    }

    if(addr == 0x200C){
        w->dma.len = byte;
        return;
    }

    if(addr == 0x200D){
        if(byte & 0x80)
            watara_dma_trigger(&w->dma);
        return;
    }

    if(addr)

    if(addr == 0x2010){
        w->apu.waves[0].flow = byte;
        return;
    }

    if(addr == 0x2011){
        w->apu.waves[0].fhi = byte;
        return;
    }
    
    if(addr == 0x2012){
        w->apu.waves[0].vol_duty = byte;
        return;
    }

    if(addr == 0x2013){
        w->apu.waves[0].length = byte;
        return;
    }
    
    if(addr == 0x2014){
        w->apu.waves[1].flow = byte;
        return;
    }

    if(addr == 0x2015){
        w->apu.waves[1].fhi = byte;
        return;
    }
    
    if(addr == 0x2016){
        w->apu.waves[1].vol_duty = byte;
        return;
    }

    if(addr == 0x2017){
        w->apu.waves[1].length = byte;
        return;
    }

    if(addr == 0x2018){
        w->apu.ch3.addr_lo = byte;
        return;
    }

    if(addr == 0x2019){
        w->apu.ch3.addr_hi = byte;
        return;
    }

    if(addr == 0x201A){
        w->apu.ch3.length = byte;
        return;
    }

    if(addr == 0x201B){
        w->apu.ch3.ctrl = byte;
        return;
    }

    if(addr == 0x201C){
        w->apu.ch3.trigger = true;
        return;
    }

    if(addr == 0x2021){
        w->ddr = byte & 0xF;
        return;
    }

    if(addr == 0x2022){
        w->link = byte & 0xF;
        return;
    }

    if(addr == 0x2023){
        w->tmr.ctr = byte;
        w->tmr.divider = 0;
        return;
    }

    if(addr == 0x2026){
        w->ctrl = byte;
        return;
    }

    if(addr == 0x2028){
        w->apu.noise.freq_vol = byte;
        return;
    }

    if(addr == 0x2029){
        w->apu.noise.length = byte;
        return;
    }

    if(addr == 0x202A){
        w->apu.noise.lfsr = 0x7FFF;
        w->apu.noise.ctrl = byte;
        return;
    }

    //printf("invalid write %02X:%X\n", addr, byte);
    //exit(EXIT_FAILURE);
}

void* WATARA_init(const archive_t* rom_archive, const archive_t* bios_archive){
    watara_t* w = (watara_t*)malloc(sizeof(watara_t));
    memset(w, 0, sizeof(watara_t));

    file_t* f = archive_get_file_by_ext(rom_archive, "watara");
    if(!f)
        f = archive_get_file_by_ext(rom_archive, "sv");
    w->rom = f->data;
    w->rom_size = f->size;

    w->cpu.ctx = (void*)w;
    w->cpu.read = watara_read;
    w->cpu.write = watara_write;
    w65c02_init(&w->cpu);
    w65c02_reset(&w->cpu);

    w->apu.noise.lfsr = 0x7FFF;
    w->apu.read = watara_read;
    w->apu.ctx = (void*)w;

    w->dma.read = watara_read;
    w->dma.write = watara_write;
    w->dma.ctx = (void*)w;

    return w;
}

void WATARA_run_frame(watara_t* w){
    w65c02_t* cpu = &w->cpu;
    tmr_t* tmr = &w->tmr;
    apu_t* apu = &w->apu;

    while(cpu->cycles < CYCLES_BEFORE_NMI){
        WATARA_STEP(w65c02_step);
    }

    cpu->cycles -= CYCLES_BEFORE_NMI;

    if(w->ctrl & 1){
        WATARA_STEP(w65c02_nmi);
    }

    watara_screen_update(&w->lcd);
}

static u8 watara_get_controller(){
    const control_t controls[8] = {
        CONTROL_WATARA_RIGHT, CONTROL_WATARA_LEFT, CONTROL_WATARA_DOWN, CONTROL_WATARA_UP,
        CONTROL_WATARA_B, CONTROL_WATARA_A, CONTROL_WATARA_SELECT, CONTROL_WATARA_START
    };

    u8 out = 0xFF;
    for(int i = 0; i < 8; i++)
        out &= ~(controls_pressed(controls[i]) << i);

    return out;
}

bool WATARA_detect(const archive_t* rom_archive){
    return archive_get_file_by_ext(rom_archive, "watara") || archive_get_file_by_ext(rom_archive, "sv");
}