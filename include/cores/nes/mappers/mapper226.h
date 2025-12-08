#include "cores/nes/memory.h"

typedef struct m226_t {
    u8 reg_8000;
    u8 reg_8001;
} m226_t;

MAPPER_INIT(226, 
    MAPPER_ALLOC(n, m226_t);
    memset(n->mapper, 0, sizeof(m226_t));
)

MAPPER_CPU_READ(226, 
    if(addr >= 0x8000){
        m226_t* m = (m226_t*)n->mapper;
        bool mode = m->reg_8000 & (1 << 5);
        u8 reg = (m->reg_8000 & 0x1F) | ((m->reg_8000 & 0x80) >> 2) | (m->reg_8001 << 6);
        if(!mode)
            reg &= ~1;
        u32 bank = reg << 14;
        addr = mode ? addr & 0x3FFF : addr & 0x7FFF;
        return cart->prg[(addr | bank) % cart->prg_size];  
    }

    return 0;
)

MAPPER_CPU_WRITE(226, 
    m226_t* m = (m226_t*)n->mapper;
    if(addr == 0x8000){
        n->cart.vram_align = byte & (1 << 6) ? VRAM_H : VRAM_V;
        m->reg_8000 = byte;
        return;
    }

    if(addr == 0x8001){
        m->reg_8001 = byte & 1;
        return;
    }
)

MAPPER_PPU_READ(226,
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

MAPPER_PPU_WRITE(226,
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