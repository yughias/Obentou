/*
 * Address map:
 *   0x0000–0x3FFF  program ROM (or Ms Pac-Man aux ROM overlay)
 *   0x4000–0x4FFF  work / video RAM
 *   0x5000–0x50FF  memory-mapped I/O registers
 *   0x8000–0x8FFF  Ms Pac-Man high aux ROM (AUX_INSTALLED only)
 */

#include "cores/pacman/memory.h"
#include "cores/pacman/pacman.h"

static u8 maketrax_read_port_2(const pacman_t* p, u16 addr) {
    const maketrax_regs_t* m = &p->maketrax;

    const uint8_t protdata[0x1e] = { // table at $ebd (odd entries)
        0x00, 0xc0, 0x00, 0x40, 0xc0, 0x40, 0x00, 0xc0, 0x00, 0x40, 0x00, 0xc0, 0x00, 0x40, 0xc0, 0x40,
        0x00, 0xc0, 0x00, 0x40, 0x00, 0xc0, 0x00, 0x40, 0xc0, 0x40, 0x00, 0xc0, 0x00, 0x40
    };

    uint8_t data = p->DIP_SWITCH_SETTINGS & 0x3f;

    if (m->disable_protection == 0)
        return protdata[m->offset] | data;

    switch (addr - 0x5080)
    {
        case 0x01:
        case 0x04:
            data |= 0x40; break;
        case 0x05:
        case 0x0e: // korosuke
        case 0x10: // korosuke
            data |= 0xc0; break;
        default:
            data &= 0x3f; break;
    }

    return data;
}

static u8 maketrax_read_port_3(const pacman_t* p, u16 addr) {
    const maketrax_regs_t* m = &p->maketrax;

    const uint8_t protdata[0x1e] = { // table at $ebd (even entries)
            0x1f, 0x3f, 0x2f, 0x2f, 0x0f, 0x0f, 0x0f, 0x3f, 0x0f, 0x0f, 0x1c, 0x3c, 0x2c, 0x2c, 0x0c, 0x0c,
            0x0c, 0x3c, 0x0c, 0x0c, 0x11, 0x31, 0x21, 0x21, 0x01, 0x01, 0x01, 0x31, 0x01, 0x01
    };

    if (m->disable_protection == 0)
        return protdata[m->offset];

    switch (addr - 0x50C0)
    {
        case 0x00:
            return 0x1f;
        case 0x09:
            return 0x30;
        case 0x0c:
            return 0x00;
        default:
            return 0x20;
    }
}

static void maketrax_protection_write(maketrax_regs_t* m, u8 val) {
    if (val == 0) // disable protection / reset?
    {
        m->counter = 0;
        m->offset = 0;
        m->disable_protection = 1;
        return;
    }

    if (val == 1)
    {
        m->disable_protection = 0;

        m->counter++;
        if (m->counter == 0x3c)
        {
            m->counter = 0;
            m->offset++;

            if (m->offset == 0x1e)
                m->offset = 0;
        }
    }
}

u8 pacman_read_memory(void* ctx, u16 addr)
{
    const pacman_t* p = (pacman_t*)ctx;

    if(p->type == PACMAN_TYPE_MAKETRAX && addr >= 0x5080 && addr < 0x50C0) {
        return maketrax_read_port_2(p, addr);
    }

    if(p->type == PACMAN_TYPE_MAKETRAX && addr >= 0x50C0 && addr < 0x5100) {
        return maketrax_read_port_3(p, addr);
    }

    /* Ms Pac-Man auxiliary board overlays the lower 16 KiB of ROM */
    if (p->AUX_ROM && p->AUX_ENABLED) {
        if (addr < PACMAN_ROM_SIZE)
            return p->AUX_ROM[addr];
    }

    if (p->ROM_HIGH && addr >= 0x8000)
        return p->ROM_HIGH[addr & 0x7FFF];  
        
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

    if (p->type == PACMAN_TYPE_MAKETRAX && addr == 0x5004) {
        maketrax_protection_write(&p->maketrax, val);
        return;
    }

    if (p->type == PACMAN_TYPE_JRPACMAN) {
        if (addr == 0x5070) {
            p->jrpacman.palettebank = val & 0x1;
            return;
        }

        if (addr == 0x5071) {
            p->jrpacman.colorbank = val & 0x1;
            return;
        }

        if (addr == 0x5073) {
            p->jrpacman.bgpriority = val & 0x1;
            return;
        }

        if (addr == 0x5074) {
            p->jrpacman.tilebank = val & 0x1;
            return;
        }

        if (addr == 0x5075) {
            p->jrpacman.spritebank = val & 0x1;
            return;
        }

        if (addr == 0x5080) {
            p->jrpacman.scroll = val;
            return;
        }       
    }

    if (addr < PACMAN_ROM_SIZE)
        return;

    if (addr < 0x5000) {
        p->RAM[addr - PACMAN_ROM_SIZE] = val;
        return;
    }

    switch (addr) {
        case 0x5000: p->VBLANK_ENABLED = val;          return;
        case 0x5001: p->SOUND_ENABLED  = val;          return;
        case 0x5002: p->AUX_ENABLED    |= (val & 0x1); return;
        case 0x5003: p->FLIP_SCREEN    = val;          return;
        case 0x5004: p->P1_LAMP        = val;          return;
        case 0x5005: p->P2_LAMP        = val;          return;
        case 0x5006: p->COIN_LOCKOUT   = val;          return;
        case 0x5007: p->COIN_COUNTER   = val;          return;
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
