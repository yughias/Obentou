#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "cpus/m6502.h"
#include "cores/nes/ppu.h"
#include "cores/nes/apu.h"

#define RAM_SIZE (1 << 11)

#define CPU_COMMON_READ \
nes_t* n = (nes_t*)ctx; \
ppu_t* ppu = &n->ppu; \
ines_t* cart = &n->cart; \
joypad_t* joy = &n->joypad; \
nes_sync(n); \
if(addr < 0x2000){ \
    return n->ram[addr % RAM_SIZE]; \
} \
if(addr == 0x2002){ \
    u8 out = ppu->status; \
    ppu->status &= 0x7F; \
    ppu->w = false; \
    return out; \
} \
if(addr == 0x2004){ \
    return ppu->oam[ppu->oam_addr]; \
} \
if(addr == 0x2007){ \
    u8 out = ppu->data; \
    ppu->data = ppu->read(ppu, ppu->v); \
    nes_ppu_inc_addr(ppu); \
    return out; \
} \
if(addr == 0x4015) \
    return nes_apu_get_status(&n->apu); \
if(addr == 0x4016){ \
    return nes_joypad_read_1(joy); \
} \
if(addr == 0x4017){ \
    return nes_joypad_read_2(joy); \
} \
if(cart->prg_ram_size && addr >= 0x6000 && addr < 0x8000){ \
    return cart->prg_ram[addr - 0x6000]; \
} \

#define CPU_COMMON_WRITE \
nes_t* n = (nes_t*)ctx; \
ppu_t* ppu = &n->ppu; \
joypad_t* joy = &n->joypad; \
ines_t* cart = &n->cart; \
nes_sync(n); \
if(addr < 0x2000){ \
    n->ram[addr % RAM_SIZE] = byte; \
    return;  \
} \
if(addr == 0x2000){ \
    ppu->ctrl = byte; \
    ppu->t = (ppu->t & (~(0b11 << 10))) | ((byte & 0b11) << 10); \
    return; \
} \
if(addr == 0x2001){ \
    ppu->mask = byte; \
    return; \
} \
if(addr == 0x2003){ \
    ppu->oam_addr = byte; \
    return; \
} \
if(addr == 0x2004){ \
    ppu->oam[ppu->oam_addr] = byte; \
    ppu->oam_addr += 1; \
    return; \
} \
if(addr == 0x2005){ \
    if(ppu->w) { \
        ppu->t &= ~(0x1F << 5); \
        ppu->t &= ~(0b111 << 12); \
        ppu->t |= (byte & 0b111) << 12; \
        ppu->t |= (byte >> 3) << 5; \
        ppu->w = false; \
    } else { \
        ppu->t = (ppu->t & (~0x1F)) | (byte >> 3); \
        ppu->x = byte & 7; \
        ppu->w = true; \
    } \
    return; \
} \
if(addr == 0x2006){ \
    if(ppu->w) { \
        ppu->t &= 0xFF00; \
        ppu->t |= byte; \
        ppu->v = ppu->t; \
        ppu->w = false; \
    } else { \
        ppu->t &= 0xFF; \
        ppu->t |= (byte & 0x3F) << 8; \
        ppu->w = true; \
    } \
    ppu->read(ppu, ppu->v); \
    return; \
} \
if(addr == 0x2007){ \
    ppu->write(ppu, ppu->v, byte); \
    nes_ppu_inc_addr(ppu); \
    return; \
} \
if(addr == 0x4000){ \
    nes_apu_write_pulse_0(&n->apu.pulses[0], byte); \
    return; \
} \
if(addr == 0x4001){ \
    nes_apu_write_pulse_1(&n->apu.pulses[0], byte); \
    return; \
} \
if(addr == 0x4002){ \
    nes_apu_write_pulse_2(&n->apu.pulses[0], byte); \
    return; \
} \
if(addr == 0x4003){ \
    nes_apu_write_pulse_3(&n->apu.pulses[0], byte); \
    return; \
} \
if(addr == 0x4004){ \
    nes_apu_write_pulse_0(&n->apu.pulses[1], byte); \
    return; \
} \
if(addr == 0x4005){ \
    nes_apu_write_pulse_1(&n->apu.pulses[1], byte); \
    return; \
} \
if(addr == 0x4006){ \
    nes_apu_write_pulse_2(&n->apu.pulses[1], byte); \
    return; \
} \
if(addr == 0x4007){ \
    nes_apu_write_pulse_3(&n->apu.pulses[1], byte); \
    return; \
} \
if(addr == 0x4008){ \
    nes_apu_write_triangle_0(&n->apu.triangle, byte); \
    return; \
} \
if(addr == 0x400A){ \
    nes_apu_write_triangle_1(&n->apu.triangle, byte); \
    return; \
} \
if(addr == 0x400B){ \
    nes_apu_write_triangle_2(&n->apu.triangle, byte); \
    return; \
} \
if(addr == 0x400C){ \
    nes_apu_write_noise_0(&n->apu.noise, byte); \
    return; \
} \
if(addr == 0x400E){ \
    nes_apu_write_noise_1(&n->apu.noise, byte); \
    return; \
} \
if(addr == 0x400F){ \
    nes_apu_write_noise_2(&n->apu.noise, byte); \
    return; \
} \
if(addr == 0x4010){ \
    nes_apu_write_dmc_0(&n->apu.dmc, byte); \
    return; \
} \
if(addr == 0x4011){ \
    nes_apu_write_dmc_1(&n->apu.dmc, byte); \
    return; \
} \
if(addr == 0x4012){ \
    nes_apu_write_dmc_2(&n->apu.dmc, byte); \
    return; \
} \
if(addr == 0x4013){ \
    nes_apu_write_dmc_3(&n->apu.dmc, byte); \
    return; \
} \
if(addr == 0x4014){ \
    n->dma.enabled = true; \
    n->dma.addr_hi = byte; \
    n->dma.read_cycle = true; \
    return; \
} \
if(addr == 0x4015){ \
    nes_apu_write_control(&n->apu, byte); \
    return; \
} \
if(addr == 0x4016){ \
    nes_joypad_write(joy, byte); \
    return; \
} \
if(addr == 0x4017){ \
    nes_apu_write_mi_reg(&n->apu, byte); \
    return; \
} \
if(cart->prg_ram_size && addr >= 0x6000 && addr < 0x8000){ \
    cart->prg_ram[addr - 0x6000] = byte; \
    return; \
}

#define PPU_COMMON_PTR \
nes_t* n = (nes_t*)ppu->ctx; \
ines_t* cart = &n->cart \

#define MAPPER_INIT(X, func) static void mapper ## X ## _init(nes_t* n) { func }
#define MAPPER_CPU_READ(X, func) static u8 mapper ## X ## _cpu_read(void* ctx, u16 addr) { CPU_COMMON_READ; func }
#define MAPPER_CPU_WRITE(X, func) static void mapper ## X ## _cpu_write(void* ctx, u16 addr, u8 byte) { CPU_COMMON_WRITE; func }
#define MAPPER_PPU_READ(X, func) static u8 mapper ## X ## _ppu_read(ppu_t* ppu, u16 addr) { PPU_COMMON_PTR; func }
#define MAPPER_PPU_WRITE(X, func) static void mapper ## X ## _ppu_write(ppu_t* ppu, u16 addr, u8 byte) { PPU_COMMON_PTR; func }


#endif