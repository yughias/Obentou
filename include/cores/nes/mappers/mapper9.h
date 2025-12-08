#include "cores/nes/memory.h"

typedef struct mmc2_t {
    u8 prg_bank;
    u8 chr_fd[2];
    u8 chr_fe[2];
    u8 latch[2];
} mmc2_t;

MAPPER_INIT(9,
    MAPPER_ALLOC(n, mmc2_t);
    mmc2_t* mmc2 = (mmc2_t*)n->mapper;
    mmc2->latch[0] = 0xFE;
    mmc2->latch[1] = 0xFE;
)

MAPPER_CPU_READ(9, 
    if(addr >= 0x8000 && addr < 0xA000){
        mmc2_t* mmc2 = (mmc2_t*)n->mapper;
        addr -= 0x8000;
        u32 prg_addr = addr + (mmc2->prg_bank << 13);
        return cart->prg[prg_addr % cart->prg_size];  
    }
    addr -= 0xA000;
    return cart->prg[addr - (3 << 13) + cart->prg_size];   
)

MAPPER_CPU_WRITE(9, 
    if(addr >= 0xA000 && addr < 0xB000){
        mmc2_t* mmc2 = (mmc2_t*)n->mapper;
        mmc2->prg_bank = byte & 0xF;
        return;
    }

    if(addr >= 0xB000 && addr < 0xC000){
        mmc2_t* mmc2 = (mmc2_t*)n->mapper;
        mmc2->chr_fd[0] = byte & 0x1F;
        return;
    }
    
    if(addr >= 0xC000 && addr < 0xD000){
        mmc2_t* mmc2 = (mmc2_t*)n->mapper;
        mmc2->chr_fe[0] = byte & 0x1F;
        return;
    }

    if(addr >= 0xD000 && addr < 0xE000){
        mmc2_t* mmc2 = (mmc2_t*)n->mapper;
        mmc2->chr_fd[1] = byte & 0x1F;
        return;
    }
    
    if(addr >= 0xE000 && addr < 0xF000){
        mmc2_t* mmc2 = (mmc2_t*)n->mapper;
        mmc2->chr_fe[1] = byte & 0x1F;
        return;
    }

    if(addr >= 0xF000){
        ines_t* cart = &n->cart;
        cart->vram_align = byte & 1 ? VRAM_V : VRAM_H; 
        return;
    }
)

MAPPER_PPU_READ(9,
    mmc2_t* mmc2 = (mmc2_t*)n->mapper;
    mmc2_t old_mmc2 = *mmc2;

    if(addr == 0xFD8)
        mmc2->latch[0] = 0xFD;
    if(addr == 0xFE8)
        mmc2->latch[0] = 0xFE;
    if(addr >= 0x1FD8 && addr <= 0x1FDF)
        mmc2->latch[1] = 0xFD;
    if(addr >= 0x1FE8 && addr <= 0x1FEF)
        mmc2->latch[1] = 0xFE;

    if(addr < 0x1000){
        u8 bank = old_mmc2.latch[0] == 0xFE ? old_mmc2.chr_fe[0] : old_mmc2.chr_fd[0];
        u32 chr_addr = addr + (bank << 12);
        return cart->chr[chr_addr % cart->chr_size];
    }

    if(addr >= 0x1000 && addr < 0x2000){
        addr -= 0x1000;
        u8 bank = old_mmc2.latch[1] == 0xFE ? old_mmc2.chr_fe[1] : old_mmc2.chr_fd[1];
        u32 chr_addr = addr + (bank << 12);
        return cart->chr[chr_addr % cart->chr_size];
    }

    if(addr < 0x3000){
        addr -= 0x2000;
        addr = nes_ppu_get_vram_addr(cart->vram_align, addr, ppu->vram_size);
        return ppu->vram[addr];
    }

    if(addr >= 0x3F00)
        return ppu->palette_ram[addr % PALETTE_RAM_SIZE];

    return 0;
)

MAPPER_PPU_WRITE(9,
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

