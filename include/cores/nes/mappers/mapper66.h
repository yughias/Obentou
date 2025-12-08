#include "cores/nes/memory.h"

MAPPER_INIT(66, 
    MAPPER_ALLOC(n, u8);
)

MAPPER_CPU_READ(66, 
    u8 bank_reg = *(u8*)n->mapper;
    u8 bank = (bank_reg >> 4) & 0b11;
    u32 prg_addr = addr & 0x7FFF;
    prg_addr |= bank << 15;
    return cart->prg[prg_addr % cart->prg_size];   
)

MAPPER_CPU_WRITE(66, 
    if(addr >= 0x8000){
        u8* bank_ptr = (u8*)n->mapper;
        *bank_ptr = byte;
        return;
    }    
)

MAPPER_PPU_READ(66,
    if(addr < 0x2000){
        u8 bank_reg = *(u8*)n->mapper;
        u8 bank = bank_reg & 0b11;
        u32 chr_addr = addr;
        chr_addr |= bank << 13;
        return cart->chr[chr_addr % cart->chr_size];
    }

    if(addr < 0x3000){
        addr = nes_ppu_get_vram_addr(cart->vram_align, addr, ppu->vram_size);
        return ppu->vram[addr];
    }

    if(addr >= 0x3F00)
        return ppu->palette_ram[addr % PALETTE_RAM_SIZE];

    return 0;
)

MAPPER_PPU_WRITE(66,
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

