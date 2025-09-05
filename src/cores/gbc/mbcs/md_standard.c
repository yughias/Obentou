#include "cores/gbc/mbc.h"
#include "cores/gbc/memory.h"
#include "cores/gbc/gb.h"

u8 gb_megaduck_standard_mapper(gb_t* gb, u16 addr){
    mbc_t* mbc = &gb->mbc;
    size_t real_addr;
    addr -= 0x4000;
    u8 bank = mbc->REG_0000_1FFF ? mbc->REG_0000_1FFF : 1;
    real_addr = addr + 0x4000 * bank;
    real_addr &= gb->ROM_SIZE - 1;
    return gb->ROM[real_addr];
}

void gb_megaduck_standard_registers(gb_t* gb, u16 addr, u8 byte){
    mbc_t* mbc = &gb->mbc;
    if(addr == 1)
        mbc->REG_0000_1FFF = byte;
}