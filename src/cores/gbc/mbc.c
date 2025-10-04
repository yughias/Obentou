#include "cores/gbc/mbc.h"
#include "cores/gbc/memory.h"
#include "cores/gbc/gb.h"
#include "cores/gbc/mbcs/all.h"
#include "cores/gbc/info.h"

#include <stdio.h>
#include <string.h>

#define RAM_MAPPER(name) mbc->mapper_A000_BFFF_read =  gb_ ## name ## _read; mbc->mapper_A000_BFFF_write = gb_ ## name ## _write

static u8 gb_no_mapped_address_read(gb_t* gb, u16 addr){ return 0xFF; }
static void gb_no_mapped_address_write(gb_t* gb, u16 addr, u8 byte){}
static u8 gb_no_mbc_address(gb_t* gb, u16 addr){ return gb->ROM[addr]; }
static void gb_mbc_no_write(gb_t* gb, u16 addr, u8 byte){}

static void mbc_standard_registers(gb_t* gb, u16 addr, u8 byte){
    mbc_t* mbc = &gb->mbc;
    if(addr < 0x2000){
        mbc->REG_0000_1FFF = byte;
    } else if(addr < 0x4000) {
        mbc->REG_2000_3FFF = byte;
    } else if(addr < 0x6000) {
        mbc->REG_4000_5FFF = byte;
    } else {
        mbc->REG_6000_7FFF = byte;
    }
}

void gb_detectConsoleAndMbc(gb_t* gb){
    mbc_t* mbc = &gb->mbc;
    mbc->rom_write = mbc_standard_registers;
    mbc->mapper_0000_3FFF = gb_no_mbc_address;
    mbc->mapper_4000_7FFF = gb_no_mbc_address;
    RAM_MAPPER(no_mapped_address);
    mbc->REG_0000_1FFF = 0x00;
    mbc->REG_2000_3FFF = 0x00;
    mbc->REG_4000_5FFF = 0x00;
    mbc->REG_6000_7FFF = 0x00;
    mbc->REG_2000_2FFF = 0x00;
    mbc->REG_3000_3FFF = 0x00;
    mbc->hasBattery = false;
    mbc->hasRtc = false;
    mbc->hasCamera = false;
    mbc->mbcAlreadyWritten = false; 

    gb->console_type = CGB_TYPE;

    if(gb->noCart)
        return;

    // TODO
    //if(config_force_dmg_when_possible &&gb->ROM[0x143] != 0xC0)
    //    gb->console_type = DMG_TYPE;

    // MBC for megaduck
    if(!gb_containNintendoLogo(gb->ROM)){
        gb->console_type = MEGADUCK_TYPE;

        // default 32k megaduck rom
        if(gb->ROM_SIZE == 1 << 15)
            return;

        u16 checksum = gb_calculateRomChecksum(gb->ROM, gb->ROM_SIZE);

        if(
            checksum == SULEIMAN_TREASURE_CHECKSUM ||
            checksum == PUPPET_KNIGHT_CHECKSUM
        ){
            mbc->mapper_0000_3FFF = gb_megaduck_special_mapper;
            mbc->mapper_4000_7FFF = gb_megaduck_special_mapper;
            RAM_MAPPER(megaduck_special_registers);
            mbc->rom_write = gb_mbc_no_write;
            return;
        }

        // megaduck mapper with register in 0x0001
        mbc->mapper_0000_3FFF = gb_no_mbc_address;
        mbc->mapper_4000_7FFF = gb_megaduck_standard_mapper;
        mbc->rom_write =  gb_megaduck_standard_registers;
        return;
    }

    if(gb_detectM161(gb->ROM)){
        printf("M161 DETECTED!\n");
        mbc->rom_write = gb_m161_registers;
        mbc->mapper_0000_3FFF = gb_m161_rom;
        mbc->mapper_4000_7FFF = gb_m161_rom;
        return;
    }

    if(gb_detectMMM01(gb->ROM, gb->ROM_SIZE)){
        printf("MMM01 DETECTED!\n");
        mbc->REG_3000_3FFF = 0x01;
        mbc->rom_write = gb_mmm01_registers;
        mbc->mapper_0000_3FFF = gb_mmm01_0000_3FFF;
        mbc->mapper_4000_7FFF = gb_mmm01_4000_7FFF;
        RAM_MAPPER(mmm01_ram);
        return;
    }

    if(gb_detectMBC1M(gb->ROM, gb->ROM_SIZE)){
        printf("MBC1M DETECTED\n");
        mbc->mapper_0000_3FFF = gb_mbc1m_0000_3FFF;
        mbc->mapper_4000_7FFF = gb_mbc1m_4000_7FFF;
        return;
    }

    switch(gb->ROM[0x147]){
        case 0x01:
        mbc->mapper_0000_3FFF = gb_mbc1_0000_3FFF;
        mbc->mapper_4000_7FFF = gb_mbc1_4000_7FFF;
        printf("MBC1 ON!\n");
        break;

        case 0x03:
        mbc->mapper_0000_3FFF = gb_mbc1_0000_3FFF;
        mbc->mapper_4000_7FFF = gb_mbc1_4000_7FFF;
        RAM_MAPPER(mbc1_ram);
        mbc->hasBattery = true;
        printf("MBC1 WITH RAM AND BATTERY ON!\n");
        break;

        case 0x05:
        mbc->rom_write = gb_mbc2_registers;
        mbc->REG_2000_3FFF = 0x01;
        mbc->mapper_0000_3FFF = gb_no_mbc_address;
        mbc->mapper_4000_7FFF = gb_mbc2_4000_7FFF;
        printf("MBC2 ONLY ON!\n");
        break;

        case 0x06:
        mbc->rom_write = gb_mbc2_registers;
        mbc->REG_2000_3FFF = 0x01;
        mbc->mapper_0000_3FFF = gb_no_mbc_address;
        mbc->mapper_4000_7FFF = gb_mbc2_4000_7FFF;
        RAM_MAPPER(mbc2_ram);
        mbc->hasBattery = true;
        printf("MBC2 WITH RAM AND BATTERY ON!\n");
        break;

        case 0x0F:
        case 0x10:
        mbc->hasRtc = true;
        mbc->data = gb_allocRtc();
        case 0x11:
        case 0x12:
        case 0x13:
        mbc->rom_write = gb_mbc3_registers;
        mbc->mapper_0000_3FFF = gb_no_mbc_address;
        mbc->mapper_4000_7FFF = gb_mbc3_4000_7FFF;
        RAM_MAPPER(mbc3_ram);
        mbc->hasBattery = true;
        printf("MBC3 WITH RAM AND BATTERY ON!\n");
        break;

        case 0x19:
        mbc->rom_write = gb_mbc5_registers;
        mbc->REG_2000_2FFF = 0x01;
        mbc->mapper_0000_3FFF = gb_no_mbc_address;
        mbc->mapper_4000_7FFF = gb_mbc5_4000_7FFF;
        mbc->hasBattery = false;
        printf("MBC5 ONLY ON!\n");
        break;

        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        mbc->hasRumble = gb->ROM[0x147] >= 0x1C;
        mbc->rom_write = gb_mbc5_registers;
        mbc->REG_2000_2FFF = 0x01;
        mbc->mapper_0000_3FFF = gb_no_mbc_address;
        mbc->mapper_4000_7FFF = gb_mbc5_4000_7FFF;
        RAM_MAPPER(mbc5_ram);
        mbc->hasBattery = true;
        printf("MBC5 WITH RAM ON!\n");
        break;

        case 0x22:
        mbc->hasBattery = true;
        mbc->rom_write = gb_mbc5_registers;
        mbc->REG_2000_2FFF = 0x01;
        mbc->mapper_0000_3FFF = gb_no_mbc_address;
        mbc->mapper_4000_7FFF = gb_mbc5_4000_7FFF;
        RAM_MAPPER(mbc7_ram);
        mbc->data = gb_mbc7_alloc();
        printf("MBC7 ON!\n");
        break;

        case 0xFC:
        mbc->hasBattery = true;
        mbc->hasCamera = true;
        mbc->REG_2000_3FFF = 0x1;
        mbc->mapper_0000_3FFF = gb_no_mbc_address;
        mbc->mapper_4000_7FFF = gb_mbc3_4000_7FFF;
        RAM_MAPPER(mbc_cam_ram);
        gb_mbc_cam_init(gb);
        printf("GAMEBOY CAMERA ON!\n");
        break;

        default:
        break;
    }
}

bool gb_detectM161(const u8* buf){
    if(!strncmp((const char*)&buf[0x134], "TETRIS SET", strlen("TETRIS SET")))
        if(buf[0x14D] == 0x3F && buf[0x14E] == 0x4C && buf[0x14F] == 0xB7)
            return true;
    
    return false;
}

bool gb_detectMMM01(const u8* buf, size_t size){
    if(size == (1 << 15)){
        return false;
    }

    if(gb_containNintendoLogo(buf + size - (1 << 15)))
        return true;
    return false;
}

bool gb_detectMBC1M(const u8* buf, size_t size){
    int nintendo_logo_size = 48; 
    int nintendo_logo_position = 0x10 << 14;
    if(size < nintendo_logo_position + 0x100 + nintendo_logo_size)
        return false;

    if(gb_containNintendoLogo(buf + nintendo_logo_position))
        return true;

    return false;
}

bool gb_containNintendoLogo(const u8* buffer){
    const u8 nintendo_logo[48] = {
        0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 
        0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
        0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 
        0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
        0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 
        0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
    };

    bool out = memcmp(&buffer[0x104], nintendo_logo, sizeof(nintendo_logo)); 
    

    return !out;
}