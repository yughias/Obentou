#include "cores/nes/memory.h"

typedef struct mmc3_t {
    u8 sel;
    u8 r[8];
    bool reload_pending;
    bool irq_enabled;
    u8 irq_counter;
    u8 irq_latch;
    bool prev_a12;
    u32 prev_clk;
} mmc3_t;

static void mmc3_check_irq(nes_t* n, bool a12){
    mmc3_t* mmc3 = n->mapper;
    /*
    int scan = n->ppu.cycles / PPU_CYCLES_PER_SCANLINE;
    int c = n->ppu.cycles % PPU_CYCLES_PER_SCANLINE;
    if(scan < SCREEN_HEIGHT && c == 262){
        if(!mmc3->irq_counter || mmc3->reload_pending){
            mmc3->irq_counter = mmc3->irq_latch;
            mmc3->reload_pending = false;
        } else {
            mmc3->irq_counter -= 1;
        }
        if(!mmc3->irq_counter && mmc3->irq_enabled){
            n->irq_pin = true;
        }
    }
    */
    if(!a12){
        if(mmc3->prev_a12){
            mmc3->prev_a12 = false;
            mmc3->prev_clk = n->cpu.cycles;
        }
    } else if(!mmc3->prev_a12){
        u32 elapsed = n->cpu.cycles - mmc3->prev_clk;
        mmc3->prev_a12 = true;
        if(elapsed > 3){
            if(!mmc3->irq_counter || mmc3->reload_pending){
                mmc3->irq_counter = mmc3->irq_latch;
                mmc3->reload_pending = false;
            } else {
                mmc3->irq_counter -= 1;
            }
            if(!mmc3->irq_counter && mmc3->irq_enabled){
                n->cart_irq = true;
            }
        }
    }
}

MAPPER_INIT(4, 
    MAPPER_ALLOC(n, mmc3_t);
    memset(n->mapper, 0x00, sizeof(mmc3_t));
    n->cart.vram_align = VRAM_H;
    mmc3_t* mmc3 = (mmc3_t*)n->mapper;
    mmc3->prev_a12 = true;
)

MAPPER_CPU_READ(4, 
    u32 prg_addr;

    if(addr >= 0x8000){
        mmc3_t* mmc3 = (mmc3_t*)n->mapper;
        bool b0 = (addr >> 13) & 1;
        bool b1 = (addr >> 14) & 1;
        u8 selector = b0 | (b1 << 1);
        if(mmc3->sel & (1 << 6)){
            if(!b0)
                selector = ((!b1) << 1);
        }
        addr &= 0x1FFF;
        switch(selector){
            case 0b00:
            prg_addr = addr + (mmc3->r[6] << 13);
            break;

            case 0b01:
            prg_addr = addr + (mmc3->r[7] << 13);
            break;

            case 0b10:
            prg_addr = addr + cart->prg_size - (1 << 14);
            break;

            case 0b11:
            prg_addr = addr + cart->prg_size - (1 << 13);
            break;
        }

        return cart->prg[prg_addr % cart->prg_size]; 
    }

    return 0x00; 
)

MAPPER_CPU_WRITE(4, 
    if(addr >= 0x8000 && addr < 0xA000){
        mmc3_t* mmc3 = (mmc3_t*)n->mapper;
        if(addr & 1)
            mmc3->r[mmc3->sel & 0b111] = byte;
        else
            mmc3->sel = byte;
        return;
    }

    if(addr >= 0xA000 && addr < 0xC000 && !(addr & 1)){
        cart->vram_align = byte & 1 ? VRAM_V : VRAM_H;
        return;
    }

    if(addr >= 0xC000 && addr < 0xE000){
        mmc3_t* mmc3 = (mmc3_t*)n->mapper;
        if(addr & 1){
            mmc3->irq_counter = 0;
            mmc3->reload_pending = true;
        } else {
            mmc3->irq_latch = byte;
        }
        return;
    }

    if(addr >= 0xE000){
        mmc3_t* mmc3 = (mmc3_t*)n->mapper;
        if(addr & 1){
            mmc3->irq_enabled = true;
        } else {
            mmc3->irq_enabled = false;
            mmc3->prev_a12 = true;
            n->cart_irq = false;
        }
    }
)

MAPPER_PPU_READ(4,
    mmc3_check_irq(n, addr & (1 << 12));
    if(addr < 0x2000){
        mmc3_t* mmc3 = (mmc3_t*)n->mapper;
        if(mmc3->sel & 0x80){
            bool a12 = addr & (1 << 12);
            addr &= ~(1 << 12);
            addr |= (!a12) << 12;
        }
        u8 selector = (addr >> 10) & 0b111;
        u32 chr_addr;
        if(selector < 0b100){
            addr &= 0x7FF;
            bool bank_bit = selector & 0b10;
            chr_addr = addr + ((mmc3->r[bank_bit] & 0xFE) << 10);
        } else {
            addr &= 0x3FF;
            u8 bank_idx = 2 + (selector & 0b11);
            chr_addr = addr + (mmc3->r[bank_idx] << 10);
        }

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

MAPPER_PPU_WRITE(4,
    mmc3_check_irq(n, addr & (1 << 12));

    if(addr < 0x2000){
        mmc3_t* mmc3 = (mmc3_t*)n->mapper;
        if(mmc3->sel & 0x80){
            bool a12 = addr & (1 << 12);
            addr &= ~(1 << 12);
            addr |= (!a12) << 12;
        }
        u8 selector = (addr >> 10) & 0b111;
        u32 chr_addr;
        if(selector < 0b100){
            addr &= 0x7FF;
            bool bank_bit = selector & 0b10;
            chr_addr = addr + ((mmc3->r[bank_bit] & 0xFE) << 10);
        } else {
            addr &= 0x3FF;
            u8 bank_idx = 2 + (selector & 0b11);
            chr_addr = addr + (mmc3->r[bank_idx] << 10);
        }

        cart->chr[chr_addr % cart->chr_size] = byte;
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

