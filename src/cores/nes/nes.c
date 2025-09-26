#include "cores/nes/nes.h"
#include "cores/nes/memory.h"
#include "cores/nes/mappers.h"

#include <string.h>

#include "peripherals/archive.h"

typedef void (*mapper_init_func)(nes_t*);

#define GET_IRQ(n) (n->cart_irq || n->apu.frame_irq || n->apu.dmc.irq)

void* NES_init(const archive_t* rom_archive, const archive_t* bios_archive){
    nes_t* nes = malloc(sizeof(nes_t));
    file_t* rom = archive_get_file_by_ext(rom_archive, "nes");
    memset(nes, 0, sizeof(nes_t));
    nes_ines_load(&nes->cart, rom->data, rom->size);
    nes->ppu.vram_size = nes->cart.vram_align == VRAM_4 ? EXTENDED_VRAM_SIZE : BASIC_VRAM_SIZE;
    nes->ppu.vram = malloc(nes->ppu.vram_size);
    m6502_t* cpu = &nes->cpu;
    m6502_init(cpu);
    cpu->ctx = (void*)nes;
    ppu_t* ppu = &nes->ppu;
    ppu->ctx = (void*)nes;
    apu_t* apu = &nes->apu;
    apu->sequencer_mode = 4;
    apu->noise.lfsr = 1;
    apu->dmc.silence_flag = true;
    apu->dmc.empty = true;
    apu->ctx = (void*)nes;

    mapper_t* mapper = &mappers[nes->cart.mapper];
    if(!mapper->supported){
        printf("mapper %d not supported\n", nes->cart.mapper);
        exit(EXIT_FAILURE);
    }

    mapper_init_func mapper_init = (mapper_init_func)mapper->mapper_init;
    (*mapper_init)(nes);
    cpu->read = (m6502_read_func)mapper->cpu_read;
    cpu->write = (m6502_write_func)mapper->cpu_write;
    ppu->read = (ppu_read_func)mapper->ppu_read;
    ppu->write = (ppu_write_func)mapper->ppu_write;

    m6502_reset(cpu);

    return nes;
}

void nes_reset(nes_t* nes){
    m6502_init(&nes->cpu);
    m6502_reset(&nes->cpu);

    if(nes->mapper){
        free(nes->mapper);
        mapper_t* mapper = &mappers[nes->cart.mapper];
        mapper_init_func mapper_init = (mapper_init_func)mapper->mapper_init;
        (*mapper_init)(nes);
    }

    ppu_t* ppu = &nes->ppu;
    ppu_read_func ppu_read = ppu->read;
    ppu_write_func ppu_write = ppu->write;  
    u8* vram = ppu->vram;
    memset(ppu, 0, sizeof(ppu_t));
    ppu->vram = vram;
    ppu->vram_size = nes->cart.vram_align == VRAM_4 ? EXTENDED_VRAM_SIZE : BASIC_VRAM_SIZE;
    ppu->read = ppu_read;
    ppu->write = ppu_write;
    ppu->ctx = (void*)nes;
    apu_t* apu = &nes->apu;
    memset(apu, 0, sizeof(apu_t));
    apu->sequencer_mode = 4;
    apu->noise.lfsr = 1;
    apu->dmc.silence_flag = true;
    apu->dmc.empty = true;
    apu->ctx = (void*)nes;
}

void NES_run_frame(nes_t* nes){
    m6502_t* cpu = &nes->cpu;
    ppu_t* ppu = &nes->ppu;
    
    while(!ppu->end_of_frame){
        if(nes->dma.enabled){
            dma_t* dma = &nes->dma;
            if(dma->read_cycle){
                u16 addr = (dma->addr_hi << 8) | dma->addr_lo;
                dma->data = cpu->read(cpu, addr);
                cpu->cycles += 1;
                dma->read_cycle = false;
                dma->addr_lo += 1;
            } else {
                cpu->write(cpu, 0x2004, dma->data);
                cpu->cycles += 1;
                dma->read_cycle = true;
                dma->enabled =dma->addr_lo;
            }
        } else {
            if(ppu->nmi_pin){
                ppu->nmi_pin = false;
                m6502_nmi(cpu);
            } else if(GET_IRQ(nes) && m6502_interrupt_enabled(cpu)){
                m6502_irq(cpu);
            } else {
                m6502_step(cpu);
            }
        }
    }

    ppu->end_of_frame = false;
}

void nes_sync(nes_t* nes){
    nes_apu_sync(&nes->apu);
    nes_ppu_sync(&nes->ppu);
}

bool NES_detect(const archive_t* rom_archive, const archive_t* bios_archive){
    return archive_get_file_by_ext(rom_archive, "nes");
}

void NES_close(nes_t* nes, const char* sav_path){
    free(nes->mapper);
    if(nes->cart.is_chr_ram)
        free(nes->cart.chr);
    free(nes->cart.prg_ram);
    free(nes->ppu.vram);
}