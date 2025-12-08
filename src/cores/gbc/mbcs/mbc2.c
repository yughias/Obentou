#include "cores/gbc/mbc.h"
#include "cores/gbc/memory.h"
#include "cores/gbc/gb.h"

u8 gb_mbc2_4000_7FFF(gb_t* gb, u16 addr){
    mbc_t* mbc = &gb->mbc;
    size_t real_addr = addr;
    real_addr &= (1 << 14) - 1;
    u8 bank = (mbc->REG_2000_3FFF & 0b1111) == 0x00 ? 0x01 : (mbc->REG_2000_3FFF & 0b1111); 
    real_addr |= bank << 14;
    real_addr &= gb->ROM_SIZE - 1;
    return gb->ROM[real_addr];
}

u8 gb_mbc2_ram_read(gb_t* gb, u16 addr){
    mbc_t* mbc = &gb->mbc;
    if((mbc->REG_0000_1FFF & 0x0F) != 0x0A)
        return 0xFF;

    u16 ram_addr = addr & ((1 << 9) - 1);
    return gb->ERAM[ram_addr] | 0xF0;
}

void gb_mbc2_ram_write(gb_t* gb, u16 addr, u8 byte){
    mbc_t* mbc = &gb->mbc;
    if((mbc->REG_0000_1FFF & 0x0F) != 0x0A)
        return;

    u16 ram_addr = addr & ((1 << 9) - 1);
    gb->ERAM[ram_addr] = byte;
}

void gb_mbc2_registers(gb_t* gb, u16 addr, u8 byte){
    mbc_t* mbc = &gb->mbc;
    if(addr < 0x4000){
        if(!(addr & 0x100))
            mbc->REG_0000_1FFF = byte;
        else
            mbc->REG_2000_3FFF = byte;
    }
}