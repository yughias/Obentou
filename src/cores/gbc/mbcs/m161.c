#include "cores/gbc/mbc.h"
#include "cores/gbc/memory.h"
#include "cores/gbc/gb.h"

u8 gb_m161_rom(gb_t* gb, u16 addr){
    mbc_t* mbc = &gb->mbc;
    size_t real_addr = addr | (mbc->REG_0000_1FFF << 15);
    return gb->ROM[real_addr];
}

void gb_m161_registers(gb_t* gb, u16 addr, u8 byte){
    mbc_t* mbc = &gb->mbc;
    if(!mbc->mbcAlreadyWritten){
        mbc->mbcAlreadyWritten = true;
        mbc->REG_0000_1FFF = byte;
        mbc->REG_0000_1FFF &= (gb->ROM_SIZE - 1) >> 15;
    }
}