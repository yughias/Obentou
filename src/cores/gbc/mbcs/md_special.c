#include "cores/gbc/mbc.h"
#include "cores/gbc/memory.h"
#include "cores/gbc/gb.h"

u8 gb_megaduck_special_mapper(gb_t* gb, u16 addr){
    mbc_t* mbc = &gb->mbc;
    size_t real_addr;
    u8 bank = mbc->REG_0000_1FFF & 1;
    real_addr = addr + 0x8000 * bank;
    real_addr &= gb->ROM_SIZE - 1;
    return gb->ROM[real_addr];
}

u8 gb_megaduck_special_registers_read(gb_t* gb, u16 addr){
    mbc_t* mbc = &gb->mbc;
    if(addr == 0xB000)
        return mbc->REG_0000_1FFF;
    
    return 0xFF;
}

void gb_megaduck_special_registers_write(gb_t* gb, u16 addr, u8 byte){
    mbc_t* mbc = &gb->mbc;
    if(addr == 0xB000)
        mbc->REG_0000_1FFF = byte;
}