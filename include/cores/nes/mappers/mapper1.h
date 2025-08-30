#include "cores/nes/memory.h"

static VRAM_ALIGN mmc1_set_vram_align(u8 reg){
    switch(reg & 0b11){
        case 0b00:
        return VRAM_1_LO;

        case 0b01:
        return VRAM_1_HI;

        case 0b10:
        return VRAM_H;

        case 0b11:
        return VRAM_V;
    }
}   

typedef struct mmc1_t {
    u8 ctrl;
    u8 chr[2];
    u8 prg;
    u8 shifter;
    u32 last_write;
} mmc1_t;

MAPPER_INIT(1, 
    n->mapper = malloc(sizeof(mmc1_t));
    mmc1_t* mmc1 = (mmc1_t*)n->mapper;
    mmc1->ctrl = 0x0C;
    mmc1->chr[0] = 0;
    mmc1->chr[1] = 0;
    mmc1->prg = 0;
    mmc1->shifter = 0x10;
    n->cart.vram_align = mmc1_set_vram_align(mmc1->ctrl);
)

MAPPER_CPU_READ(1, 
    mmc1_t* mmc1 = (mmc1_t*)n->mapper;
    u8 mode = (mmc1->ctrl >> 2) & 0b11;
    u32 prg_addr;
    switch(mode){
        case 0b00:
        case 0b01:
        u8 bank = mmc1->prg & 0x1E;
        prg_addr = addr - 0x8000 + (bank << 14);
        break;

        case 0b10:
        if(addr >= 0xC000){
            u8 bank = mmc1->prg;
            prg_addr = addr - 0xC000 + (bank << 14);
        } else {
            addr -= 0x8000;
            prg_addr = addr;
        }
        break;

        case 0b11:
        if(addr < 0xC000){
            u8 bank = mmc1->prg;
            prg_addr = addr - 0x8000 + (bank << 14);
        } else {
            addr -= 0xC000;
            prg_addr = addr - (1 << 14) + cart->prg_size; 
        }
        break;
    }
    return cart->prg[prg_addr % cart->prg_size];   
)

MAPPER_CPU_WRITE(1, 
    if(addr >= 0x8000){
        mmc1_t* mmc1 = (mmc1_t*)n->mapper;
        m6502_t* m = &n->cpu;
        u32 elapsed = m->cycles - mmc1->last_write;
        mmc1->last_write = m->cycles;
        if(elapsed <= 1)
            return;
        bool bit = byte & 1;
        if(byte & 0x80){
            mmc1->ctrl |= 0xC;
            mmc1->shifter = 0x10;
            n->cart.vram_align = mmc1_set_vram_align(mmc1->ctrl);
            return;
        }

        if(mmc1->shifter & 1){
            mmc1->shifter = (mmc1->shifter >> 1) | (bit << 4);
            bool a14 = addr & (1 << 14);
            bool a13 = addr & (1 << 13);
            u8 selector = a13 | (a14 << 1);
            switch(selector){
                case 0b00:
                mmc1->ctrl = mmc1->shifter;
                n->cart.vram_align = mmc1_set_vram_align(mmc1->ctrl);
                break;

                case 0b01:
                mmc1->chr[0] = mmc1->shifter;
                break;

                case 0b10:
                mmc1->chr[1] = mmc1->shifter;
                break;

                case 0b11:
                mmc1->prg = mmc1->shifter & 0xF;
                break;
            }
            mmc1->shifter = 0x10;
        } else {
            mmc1->shifter = (mmc1->shifter >> 1) | (bit << 4);
        }
        return;
    }
)

MAPPER_PPU_READ(1,
    if(addr < 0x2000){
        mmc1_t* mmc1 = (mmc1_t*)n->mapper;
        u32 bank;
        if(mmc1->ctrl & (1 << 4)){
            bool bank_bit = addr & 0x1000;
            bank = mmc1->chr[bank_bit] << 12;
            addr &= 0xFFF;
        } else {
            bank = (mmc1->chr[0] & 0x1E) << 12;
        }
        return cart->chr[(bank + addr) % cart->chr_size];
    }

    if(addr < 0x3000){
        addr = nes_ppu_get_vram_addr(cart->vram_align, addr, ppu->vram_size);
        return ppu->vram[addr];
    }

    if(addr >= 0x3F00)
        return ppu->palette_ram[addr % PALETTE_RAM_SIZE];
)

MAPPER_PPU_WRITE(1,
    if(addr < 0x2000 && cart->is_chr_ram){
        mmc1_t* mmc1 = (mmc1_t*)n->mapper;
        u32 bank;
        if(mmc1->ctrl & (1 << 4)){
            bool bank_bit = addr & 0x1000;
            bank = mmc1->chr[bank_bit] << 12;
            addr &= 0xFFF;
        } else {
            bank = (mmc1->chr[0] & 0x1E) << 13;
        }
        cart->chr[(bank + addr) % cart->chr_size] = byte;
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

