/*
 * Address map:
 *   0x0000–0x3FFF  program ROM (or Ms Pac-Man aux ROM overlay)
 *   0x4000–0x4FFF  work / video RAM
 *   0x5000–0x50FF  memory-mapped I/O registers
 *   0x8000–0x8FFF  Ms Pac-Man high aux ROM (AUX_INSTALLED only)
 */

#include "cores/pacman/memory.h"
#include "cores/pacman/pacman.h"

u8 pacman_read_memory(void* ctx, u16 addr)
{
    pacman_t* p = (pacman_t*)ctx;

    /* Ms Pac-Man auxiliary board overlays the lower 16 KiB of ROM */
    if (p->AUX_INSTALLED && p->AUX_ENABLED) {
        if (addr < PACMAN_ROM_SIZE)
            return p->AUX_ROM_LOW[addr];
        if (addr >= 0x8000 && addr < 0x8800)
            return p->AUX_ROM_HIGH[addr - 0x8000];
        if (addr >= 0x8800)
            return p->AUX_ROM_HIGH[(addr & 0xFFF) + 0x800];
    }

    if (addr < PACMAN_ROM_SIZE)
        return p->ROM[addr];

    if (addr < 0x5000)
        return p->RAM[addr - PACMAN_ROM_SIZE];

    if (addr >= 0x5000 && addr < 0x5040) return p->IN0;
    if (addr >= 0x5040 && addr < 0x5080) return p->IN1;
    if (addr >= 0x5080)                  return p->DIP_SWITCH_SETTINGS;

    return 0xFF;
}

void pacman_write_memory(void* ctx, u16 addr, u8 val)
{
    pacman_t* p = (pacman_t*)ctx;
    addr &= 0x7FFF;

    if (addr < PACMAN_ROM_SIZE)
        return;

    if (addr < 0x5000) {
        p->RAM[addr - PACMAN_ROM_SIZE] = val;
        return;
    }

    switch (addr) {
        case 0x5000: p->VBLANK_ENABLED = val;                                return;
        case 0x5001: p->SOUND_ENABLED  = val;                                return;
        case 0x5002: p->AUX_ENABLED    |= (p->AUX_INSTALLED && (val & 0x1)); return;
        case 0x5003: p->FLIP_SCREEN    = val;                                return;
        case 0x5004: p->P1_LAMP        = val;                                return;
        case 0x5005: p->P2_LAMP        = val;                                return;
        case 0x5006: p->COIN_LOCKOUT   = val;                                return;
        case 0x5007: p->COIN_COUNTER   = val;                                return;
    }

    if (addr >= 0x5040 && addr <= 0x5045) { p->SOUND_VOICE1[addr - 0x5040] = val; return; }
    if (addr >= 0x5046 && addr <= 0x504A) { p->SOUND_VOICE2[addr - 0x5046] = val; return; }
    if (addr >= 0x504B && addr <= 0x504F) { p->SOUND_VOICE3[addr - 0x504B] = val; return; }
    if (addr >= 0x5050 && addr <= 0x5054) { p->VOICE1_FREQ[addr - 0x5050]  = val; return; }
    if (addr == 0x5055)                   { p->VOICE1_VOLUME               = val; return; }
    if (addr >= 0x5056 && addr <= 0x505A) { p->VOICE2_FREQ_VOL[addr - 0x5056] = val; return; }
    if (addr >= 0x505B && addr <= 0x505F) { p->VOICE3_FREQ_VOL[addr - 0x505B] = val; return; }
    if (addr >= 0x5060 && addr <= 0x506F) { p->SPRITE_COORDS[addr - 0x5060] = val; return; }
    if (addr >= 0x50C0 && addr <= 0x50FF) { p->WATCHDOG_RESET = val; return; }
}

u8 pacman_read_io(void* ctx, u16 addr)
{
    return ((pacman_t*)ctx)->IO;
}

void pacman_write_io(void* ctx, u16 addr, u8 val)
{
    ((pacman_t*)ctx)->IO = val;
}
