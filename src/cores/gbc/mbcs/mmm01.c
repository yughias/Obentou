#include "cores/gbc/mbc.h"
#include "cores/gbc/memory.h"
#include "cores/gbc/gb.h"

u8 gb_mmm01_0000_3FFF(gb_t* gb, u16 addr){
    mbc_t* mbc = &gb->mbc;
    size_t real_addr = addr;
    
    if(mbc->mbcAlreadyWritten){
        real_addr += mbc->REG_2000_2FFF << 14;
    } else {
        real_addr += gb->ROM_SIZE - (1 << 15);
    }

    real_addr &= gb->ROM_SIZE - 1;
    return gb->ROM[real_addr];
}

u8 gb_mmm01_4000_7FFF(gb_t* gb, u16 addr){
    mbc_t* mbc = &gb->mbc;
    size_t real_addr = addr;
    
     if(mbc->mbcAlreadyWritten){
        real_addr &= 0x3FFF;
        real_addr += mbc->REG_2000_2FFF << 14;
        real_addr += mbc->REG_3000_3FFF << 14;
    } else {
        real_addr += gb->ROM_SIZE - (1 << 15);
    }

    real_addr &= gb->ROM_SIZE - 1;
    return gb->ROM[real_addr];
}

u8 gb_mmm01_ram_read(gb_t* gb, u16 addr){
    mbc_t* mbc = &gb->mbc;
    if((mbc->REG_0000_1FFF & 0b1111) != 0xA)
        return 0xFF;

    size_t real_addr = (mbc->REG_4000_5FFF << 13) + (addr & 0x1FFF);
    real_addr &= gb->ERAM_SIZE - 1;
    return gb->ERAM[real_addr];
}

void gb_mmm01_ram_write(gb_t* gb, u16 addr, u8 byte){
    mbc_t* mbc = &gb->mbc;
    if((mbc->REG_0000_1FFF & 0b1111) != 0xA)
        return;

    size_t real_addr = (mbc->REG_4000_5FFF << 13) + (addr & 0x1FFF);
    real_addr &= gb->ERAM_SIZE - 1;
    gb->ERAM[real_addr] = byte;
}

void gb_mmm01_registers(gb_t* gb, u16 addr, u8 byte){
    mbc_t* mbc = &gb->mbc;
    if(addr < 0x2000){
        // MBC_0000_1FFF : ram_enable
        if(!mbc->mbcAlreadyWritten)
            mbc->mbcAlreadyWritten = true;
        else 
            mbc->REG_0000_1FFF = byte;
    } else if(addr < 0x4000){
        // MBC_2000_2FFF : rom_base
        // MBC_3000_3FFF : rom_select
        if(!mbc->mbcAlreadyWritten){
            mbc->REG_2000_2FFF = byte & 0x3F;
        } else {
            mbc->REG_3000_3FFF = byte;
        }
    } else if(addr < 0x6000){
        // MBC_4000_5FFF : ram_select
        if(mbc->mbcAlreadyWritten){
            mbc->REG_4000_5FFF = byte;
        }
    }
}
