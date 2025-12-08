#include "cores/nes/memory.h"

MAPPER_INIT(0, ;)

MAPPER_CPU_READ(0, 
    return cart->prg[addr % cart->prg_size];   
)

MAPPER_CPU_WRITE(0, ;)

MAPPER_PPU_READ(0,
    if(addr < 0x2000)
        return cart->chr[addr];

    if(addr < 0x3000){
        addr = nes_ppu_get_vram_addr(cart->vram_align, addr, ppu->vram_size);
        return ppu->vram[addr];
    }

    if(addr >= 0x3F00)
        return ppu->palette_ram[addr % PALETTE_RAM_SIZE];

    return 0;
)

MAPPER_PPU_WRITE(0,
    if(addr >= 0x2000 && addr < 0x3000){
        addr = nes_ppu_get_vram_addr(cart->vram_align, addr, ppu->vram_size);
        ppu->vram[addr] = byte;
        return;
    }
    
    if(addr >= 0x3F00 && addr < 0x4000){
        addr %= PALETTE_RAM_SIZE;
        ppu->palette_ram[addr] = byte;
        if(!(addr & 0b11)){
            ppu->palette_ram[addr & 0xEF] = byte;
        }
    }
)

