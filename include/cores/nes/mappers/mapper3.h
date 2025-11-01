#include "cores/nes/memory.h"

MAPPER_INIT(3,
    MAPPER_ALLOC(n, u8);
)

MAPPER_CPU_READ(3, 
    return cart->prg[addr % cart->prg_size];   
)

MAPPER_CPU_WRITE(3, 
    if(addr >= 0x8000){
        u8* bank_ptr = (u8*)n->mapper;
        *bank_ptr = byte;
        return;
    }
)

MAPPER_PPU_READ(3,
    if(addr < 0x2000){
        u8 reg = *(u8*)n->mapper;
        u32 bank = (reg & 0xF) * (1 << 13);
        return cart->chr[(addr + bank) % cart->chr_size];
    }

    if(addr < 0x3000){
        addr = nes_ppu_get_vram_addr(cart->vram_align, addr, ppu->vram_size);
        return ppu->vram[addr];
    }

    if(addr >= 0x3F00)
        return ppu->palette_ram[addr % PALETTE_RAM_SIZE];
)

MAPPER_PPU_WRITE(3,
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