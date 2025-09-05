#include "cores/gbc/gb.h"
#include "cores/gbc/info.h"
#include "cores/gbc/bootrom_skip.h"
#include "cores/gbc/memory_table.h"
#include "cores/gbc/mbcs/mbc3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void gb_initMemory(gb_t* gb, const char* romName){
    gb_loadRom(gb, romName);
    ppu_t* ppu = &gb->ppu;
    ppu->LY_REG = 0;
    ppu->STAT_REG = 0;
    ppu->mode = OAM_SCAN_MODE;
    ppu->lyc_compare = false;
    ppu->internal_ly = 0;
    ppu->stat_irq = false;
    ppu->BCPS_REG = 0x00;
    ppu->OCPS_REG = 0x00;
    gb->joypad.JOYP_REG = 0xFF;
    gb->SVBK_REG = 0x00;
    gb->VBK_REG = 0x00;
    gb->KEY0_REG = 0x00;
    gb->KEY1_REG = 0x00;
    gb->dma.DMA_REG = 0x00;
    gb_timer_t* tmr = &gb->timer;
    tmr->counter = 0x00;
    tmr->old_state = false;
    tmr->delay = 0x00;
    tmr->ignore_write = false;
    tmr->TIMA_REG = 0;
    tmr->TMA_REG = 0;
    tmr->TAC_REG = 0xF8;

    memset(gb->OAM, 0, OAM_SIZE);
    memset(gb->WRAM, 0, WRAM_SIZE);
    memset(gb->VRAM, 0, VRAM_SIZE);

    gb->ERAM_SIZE = gb_getRamSize(gb->ROM);
    mbc_t* mbc = &gb->mbc;
    gb_detectConsoleAndMbc(gb);
    // TODO
    //if(mbc->hasBattery)
    //    loadSav(gb, savName);
    //if(mbc->hasRtc)
    //    loadRtc(mbc->data, savName);

    readGbFunc* readTable = gb->readTable;
    gb_fillReadTable(readTable, 0x00, 0x40, mbc->mapper_0000_3FFF);
    gb_fillReadTable(readTable, 0x40, 0x80, mbc->mapper_4000_7FFF);
    gb_fillReadTable(readTable, 0x80, 0xA0, gb_readVram);
    gb_fillReadTable(readTable, 0xA0, 0xC0, mbc->mapper_A000_BFFF_read);
    gb_fillReadTable(readTable, 0xC0, 0xE0, gb_readWram);
    gb_fillReadTable(readTable, 0xE0, 0xFE, gb_readMirrorRam);
    gb_fillReadTable(readTable, 0xFE, 0xFE, gb_readOam);
    gb_fillReadTable(readTable, 0xFF, 0xFF, gb_readIO);

    writeGbFunc* writeTable = gb->writeTable;
    gb_fillWriteTable(writeTable, 0x00, 0x80, mbc->rom_write);
    gb_fillWriteTable(writeTable, 0x80, 0xA0, gb_writeVram);
    gb_fillWriteTable(writeTable, 0xA0, 0xC0, mbc->mapper_A000_BFFF_write);
    gb_fillWriteTable(writeTable, 0xC0, 0xE0, gb_writeWram);
    gb_fillWriteTable(writeTable, 0xE0, 0xFE, gb_writeMirrorRam);
    gb_fillWriteTable(writeTable, 0xFE, 0xFE, gb_writeOam);
    gb_fillWriteTable(writeTable, 0xFF, 0xFF, gb_writeIO);

    char bootromName[FILENAME_MAX];
    char iniName[FILENAME_MAX];
    getAbsoluteDir(bootromName);
    // TODO
    if(gb->console_type != MEGADUCK_TYPE){
        if(gb->console_type == CGB_TYPE){
            strcat(bootromName, "data/cgb_boot.bin");
            if(!gb_loadBootRom(gb, bootromName))
                gb_skipCgbBootrom(gb);
        } else {
            strcat(bootromName, "data/dmg_boot.bin");
            if(!gb_loadBootRom(gb, bootromName))
                gb_skipDmgBootrom(gb);
        }
    }
}

void gb_freeMemory(gb_t* gb){
    free(gb->ROM);
    free(gb->BOOTROM);
    free(gb->mbc.data);
}

u8 gb_readByte(void* ctx, u16 address){
    gb_t* gb = (gb_t*)ctx;
    return (*gb->readTable[address >> 8])((gb_t*)ctx, address);
}

void gb_writeByte(void* ctx, u16 address, u8 byte){
    gb_t* gb = (gb_t*)ctx;
    (*gb->writeTable[address >> 8])((gb_t*)ctx, address, byte);
}

void gb_loadRom(gb_t* gb, const char* filename){
    FILE* fptr = fopen(filename, "rb");
    if(!fptr){
        printf("Failed to open ROM file\n");
        exit(EXIT_FAILURE);
    }
    fseek(fptr, 0, SEEK_END);
    gb->ROM_SIZE = ftell(fptr);
    rewind(fptr);

    gb->ROM = (u8*)malloc(gb->ROM_SIZE);
    fread(gb->ROM, 1, gb->ROM_SIZE, fptr);
    fclose(fptr);
}

bool gb_loadBootRom(gb_t* gb, const char* filename){
    FILE* fptr = fopen(filename, "rb");
    if(!fptr)
        return false;

    fseek(fptr, 0, SEEK_END);
    gb->BOOTROM_SIZE = ftell(fptr);
    rewind(fptr);

    gb->BOOTROM = malloc(gb->BOOTROM_SIZE);

    fread(gb->BOOTROM, 1, gb->BOOTROM_SIZE, fptr);
    fclose(fptr);
    readGbFunc* readTable = gb->readTable;
    gb_fillReadTable(readTable, 0x00, 0x00, gb_readBootrom);
    gb_fillReadTable(readTable, 0x02, 0x09, gb_readBootrom);
    return true;
}

void gb_loadSav(gb_t* gb, const char* filename){
    FILE* fptr = fopen(filename, "rb");
    if(!fptr)
        return;
    fread(gb->ERAM, 1, gb->ERAM_SIZE, fptr);
    fclose(fptr);
}

void gb_saveSav(gb_t* gb, const char* filename){
    FILE* fptr = fopen(filename, "wb");
    if(!fptr)
        return;
    fwrite(gb->ERAM, 1, gb->ERAM_SIZE, fptr);
    fclose(fptr);
}