#include "cores/spaceinvaders/spaceinvaders.h"

#include <string.h>

u8 SI_read_memory(void* ctx, u16 addr) {
    spaceinvaders_t* si = ctx;
    addr &= MIRROR_MASK;
    return si->memory[addr];
}

void SI_write_memory(void* ctx, u16 addr, u8 byte) {
    spaceinvaders_t* si = ctx;
    addr &= MIRROR_MASK;
    if (addr < ROM_SIZE)
        return;
    si->memory[addr] = byte;
}

u8 SI_read_IO(void* ctx, u8 addr) {
    spaceinvaders_t* si = ctx;

    if(addr >= 1 && addr <= 2)
        return si->in[addr - 1];

    if(addr == SHIFT_IN)
        return SI_shifter_read(&si->shifter);

    return 0xFF;
}

void SI_write_IO(void* ctx, u8 addr, u8 byte) {
    spaceinvaders_t* si = ctx;

    if(addr == SHIFTAMNT) {
        si->shifter.amnt = byte;
        return;
    }

    if(addr == SHIFT_DATA) {
        SI_shifter_write(&si->shifter, byte);
        return;
    }
}