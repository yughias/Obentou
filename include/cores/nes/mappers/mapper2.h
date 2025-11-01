#include "cores/nes/memory.h"

MAPPER_INIT(2, 
    MAPPER_ALLOC(n, u8);
)

MAPPER_CPU_READ(2, 
    if(addr >= 0x8000 && addr < 0xC000){
        u8 reg = *(u8*)n->mapper;
        u32 bank = (reg & 0xF) << 14;
        return cart->prg[(addr - 0x8000 + bank) % cart->prg_size];  
    }
    return cart->prg[addr - 0xC000 + cart->prg_size - (1 << 14)]; 
)

MAPPER_CPU_WRITE(2, 
    if(addr >= 0x8000){
        u8* bank_ptr = (u8*)n->mapper;
        *bank_ptr = byte;
        return;
    }
)

MAPPER_PPU_READ(2,
    if(addr < 0x2000)
        return cart->chr[addr];

    if(addr < 0x3000){
        addr = nes_ppu_get_vram_addr(cart->vram_align, addr, ppu->vram_size);
        return ppu->vram[addr];
    }

    if(addr >= 0x3F00)
        return ppu->palette_ram[addr % PALETTE_RAM_SIZE];
)

MAPPER_PPU_WRITE(2,
    if(addr < 0x2000 && cart->is_chr_ram){
        cart->chr[addr] = byte;
        return;
    }
    
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

