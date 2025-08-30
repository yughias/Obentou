#include "cores/nes/memory.h"

MAPPER_INIT(7, 
    n->mapper = malloc(sizeof(u8));
)

MAPPER_CPU_READ(7, 
    if(addr >= 0x8000){
        u8 reg = *(u8*)n->mapper;
        u32 bank = (reg & 0b111) << 15;
        return cart->prg[(addr - 0x8000 + bank) % cart->prg_size];  
    }
)

MAPPER_CPU_WRITE(7, 
    if(addr >= 0x8000){
        u8* bank_ptr = (u8*)n->mapper;
        *bank_ptr = byte;
        return;
    }
)

MAPPER_PPU_READ(7,
    if(addr < 0x2000)
        return cart->chr[addr];

    if(addr < 0x3000){
        bool nt = (*(u8*)n->mapper) & 0x10;
        addr &= 0x3FF;
        addr |= nt << 10;
        return ppu->vram[addr];
    }

    if(addr >= 0x3F00)
        return ppu->palette_ram[addr % PALETTE_RAM_SIZE];
)

MAPPER_PPU_WRITE(7,
    if(addr < 0x2000 && cart->is_chr_ram){
        cart->chr[addr] = byte;
        return;
    }
    
    if(addr >= 0x2000 && addr < 0x3000){
        bool nt = (*(u8*)n->mapper) & 0x10;
        addr &= 0x3FF;
        addr |= nt << 10;
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

