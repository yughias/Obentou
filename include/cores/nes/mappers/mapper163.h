#include "cores/nes/memory.h"

typedef struct fc001_t {
    u8 reg_5000;
    u8 reg_5200;
    u8 reg_5300;
    bool feedback_e;
    bool feedback_f;
    bool a12;
    bool last_a13;
} fc001_t;

static void fc001_check_a13(fc001_t* fc, u16 addr){
    bool a13 = addr & (1 << 13);
    if(!fc->last_a13 && a13){
        if(fc->reg_5000 & 0x80){
            fc->a12 = addr & (1 << 9);
        }
    }
    fc->last_a13 = a13;
}

MAPPER_INIT(163, 
    MAPPER_ALLOC(n, fc001_t);
    memset(n->mapper, 0, sizeof(fc001_t));
)

MAPPER_CPU_READ(163, 
    fc001_t* fc = (fc001_t*)n->mapper;
    if(addr == 0x5500 || addr == 0x5501){
        return fc->feedback_f << 2;
    }

    if(addr >= 0x8000){
        u32 bank = ((fc->reg_5000 & 0xF) << 15) | ((fc->reg_5200 & 0b11) << 19);
        if(!(fc->reg_5300 & (1 << 2))){
            bank |= 0b11 << 15;
        }
        addr -= 0x8000;
        return cart->prg[(addr | bank) % cart->prg_size]; 
    }
    return 0xFF;
)

MAPPER_CPU_WRITE(163,
    fc001_t* fc = (fc001_t*)n->mapper; 
    if(addr == 0x5000){
        fc->reg_5000 = byte;
        return;
    }

    if(addr == 0x5100 || addr == 0x5101){
        bool a = addr & 1;
        bool e = byte & (1 << 0);
        bool f = byte & (1 << 2);
        fc->feedback_e = e;
        if(a){
            fc->feedback_f ^= e;
        } else {
            fc->feedback_f = f;
        }
    }

    if(addr == 0x5200){
        fc->reg_5200 = byte;
        return;
    }

    if(addr == 0x5300){
        fc->reg_5300 = byte;
        return;
    }
)

MAPPER_PPU_READ(163,
    fc001_t* fc = (fc001_t*)n->mapper;
    fc001_check_a13(fc, addr);

    if(addr < 0x2000){
        u32 real_addr = addr | (fc->a12 << 12);
        return cart->chr[real_addr % cart->chr_size];
    }

    if(addr < 0x3000){
        addr = nes_ppu_get_vram_addr(cart->vram_align, addr, ppu->vram_size);
        return ppu->vram[addr];
    }

    if(addr >= 0x3F00)
        return ppu->palette_ram[addr % PALETTE_RAM_SIZE];
)

MAPPER_PPU_WRITE(163,
    fc001_t* fc = (fc001_t*)n->mapper;
    fc001_check_a13(fc, addr);

    if(addr < 0x2000 && cart->is_chr_ram){
        u32 real_addr = addr | (fc->a12 << 12);
        cart->chr[real_addr % cart->chr_size] = byte;
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

