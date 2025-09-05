#include "cores/gbc/gb.h"
#include "cores/gbc/memory_table.h"

#define READ(name) return name ## _REG
#define WRITE(name) name ## _REG = byte; return

// cgb regs are disable after writing 01 to bit 2-3 and disabling bootrom
#define CGB_MODE(name) if(gb->console_type == CGB_TYPE) { name } else return
#define CGB_MODE_READ(name) CGB_MODE(name) 0xFF
#define CGB_MODE_WRITE(name) CGB_MODE(name)

static u16 megaduckShuffleRegisters(u16);
static u8 megaduckNibbleSwap(u16, uint8_t);

void gb_fillReadTable(readGbFunc* readTable, u8 start, u8 end, readGbFunc callback){
    readTable[start] = callback;
    for(int i = start + 1; i < end; i++)
        readTable[i] = callback;
}

void gb_fillWriteTable(writeGbFunc* writeTable, u8 start, u8 end, writeGbFunc callback){
    writeTable[start] = callback;
    for(int i = start + 1; i < end; i++)
        writeTable[i] = callback;
}

// read / write callbacks

u8 gb_readBootrom(gb_t* gb, u16 address){
    return gb->BOOTROM[address];
}

u8 gb_readVram(gb_t* gb, u16 address){
    return gb->VRAM[(address - 0x8000) + gb->VBK_REG * 0x2000];
}

u8 gb_readMirrorRam(gb_t* gb, u16 address){
    return gb->WRAM[(address - 0xE000)];
}

u8 gb_readWram(gb_t* gb, u16 address){
    u8 bank = 0;
    if(address >= 0xD000){
        address -= 0xD000;
        bank = gb->SVBK_REG ? gb->SVBK_REG : 1;
    }

    return gb->WRAM[(address & (0x2000 - 1)) + bank * 0x1000];
}

u8 gb_readOam(gb_t* gb, u16 address){
    if(address < 0xFEA0)
        return gb->OAM[address - 0xFE00];
    else
        return 0xFF;
}

u8 gb_readIO(gb_t* gb, u16 address){
    static const void* read_io[0x100] = {
        &&r_00, &&r_01, &&r_02, &&r_03, &&r_04, &&r_05, &&r_06, &&r_07, &&r_08, &&r_09, &&r_0A, &&r_0B, &&r_0C, &&r_0D, &&r_0E, &&r_0F,
        &&r_10, &&r_11, &&r_12, &&r_13, &&r_14, &&r_15, &&r_16, &&r_17, &&r_18, &&r_19, &&r_1A, &&r_1B, &&r_1C, &&r_1D, &&r_1E, &&r_1F,
        &&r_20, &&r_21, &&r_22, &&r_23, &&r_24, &&r_25, &&r_26, &&r_27, &&r_28, &&r_29, &&r_2A, &&r_2B, &&r_2C, &&r_2D, &&r_2E, &&r_2F,
        &&r_30, &&r_31, &&r_32, &&r_33, &&r_34, &&r_35, &&r_36, &&r_37, &&r_38, &&r_39, &&r_3A, &&r_3B, &&r_3C, &&r_3D, &&r_3E, &&r_3F,
        &&r_40, &&r_41, &&r_42, &&r_43, &&r_44, &&r_45, &&r_46, &&r_47, &&r_48, &&r_49, &&r_4A, &&r_4B, &&r_4C, &&r_4D, &&r_4E, &&r_4F,
        &&r_50, &&r_51, &&r_52, &&r_53, &&r_54, &&r_55, &&r_56, &&r_57, &&r_58, &&r_59, &&r_5A, &&r_5B, &&r_5C, &&r_5D, &&r_5E, &&r_5F,
        &&r_60, &&r_61, &&r_62, &&r_63, &&r_64, &&r_65, &&r_66, &&r_67, &&r_68, &&r_69, &&r_6A, &&r_6B, &&r_6C, &&r_6D, &&r_6E, &&r_6F,
        &&r_70, &&r_71, &&r_72, &&r_73, &&r_74, &&r_75, &&r_76, &&r_77, &&r_78, &&r_79, &&r_7A, &&r_7B, &&r_7C, &&r_7D, &&r_7E, &&r_7F,
        &&r_80, &&r_81, &&r_82, &&r_83, &&r_84, &&r_85, &&r_86, &&r_87, &&r_88, &&r_89, &&r_8A, &&r_8B, &&r_8C, &&r_8D, &&r_8E, &&r_8F,
        &&r_90, &&r_91, &&r_92, &&r_93, &&r_94, &&r_95, &&r_96, &&r_97, &&r_98, &&r_99, &&r_9A, &&r_9B, &&r_9C, &&r_9D, &&r_9E, &&r_9F,
        &&r_A0, &&r_A1, &&r_A2, &&r_A3, &&r_A4, &&r_A5, &&r_A6, &&r_A7, &&r_A8, &&r_A9, &&r_AA, &&r_AB, &&r_AC, &&r_AD, &&r_AE, &&r_AF,
        &&r_B0, &&r_B1, &&r_B2, &&r_B3, &&r_B4, &&r_B5, &&r_B6, &&r_B7, &&r_B8, &&r_B9, &&r_BA, &&r_BB, &&r_BC, &&r_BD, &&r_BE, &&r_BF,
        &&r_C0, &&r_C1, &&r_C2, &&r_C3, &&r_C4, &&r_C5, &&r_C6, &&r_C7, &&r_C8, &&r_C9, &&r_CA, &&r_CB, &&r_CC, &&r_CD, &&r_CE, &&r_CF,
        &&r_D0, &&r_D1, &&r_D2, &&r_D3, &&r_D4, &&r_D5, &&r_D6, &&r_D7, &&r_D8, &&r_D9, &&r_DA, &&r_DB, &&r_DC, &&r_DD, &&r_DE, &&r_DF,
        &&r_E0, &&r_E1, &&r_E2, &&r_E3, &&r_E4, &&r_E5, &&r_E6, &&r_E7, &&r_E8, &&r_E9, &&r_EA, &&r_EB, &&r_EC, &&r_ED, &&r_EE, &&r_EF,
        &&r_F0, &&r_F1, &&r_F2, &&r_F3, &&r_F4, &&r_F5, &&r_F6, &&r_F7, &&r_F8, &&r_F9, &&r_FA, &&r_FB, &&r_FC, &&r_FD, &&r_FE, &&r_FF,
    };

    if(gb->console_type == MEGADUCK_TYPE)
        address = megaduckShuffleRegisters(address);

    gb_timer_t* tmr = &gb->timer;
    sm83_t* cpu = &gb->cpu;
    dma_t* dma = &gb->dma;
    joypad_t* joy = &gb->joypad;
    ppu_t* ppu = &gb->ppu;
    apu_t* apu = &gb->apu;
    serial_t* serial = &gb->serial;
    
    goto *(read_io[address & 0xFF]);

    {
        r_00: return gb_getJoypadRegister(joy);
        r_01: READ(serial->SB);
        r_02: return serial->SC_REG | 0b01111110; // fix mask for CGB
        r_03: return 0xFF;
        r_04: return tmr->div;
        r_05: READ(tmr->TIMA);
        r_06: READ(tmr->TMA);
        r_07: tmr->TAC_REG |= 0b11111000; return tmr->TAC_REG;
        r_08: r_09: r_0A: r_0B: r_0C: r_0D: r_0E: return 0xFF;
        r_0F: cpu->IF |= 0b11100000; return cpu->IF;
        r_10: return gb_getNR10(apu);
        r_11: return gb_getNR11(apu);
        r_12: READ(apu->NR12);
        r_13: return 0xFF;
        r_14: return gb_getNR14(apu);
        r_15: return 0xFF;
        r_16: return gb_getNR21(apu);
        r_17: READ(apu->NR22);
        r_18: return 0xFF;
        r_19: return gb_getNR24(apu);
        r_1A: return gb_getNR30(apu);
        r_1B: return 0xFF;
        r_1C: return gb_getNR32(apu);
        r_1D: return 0xFF;
        r_1E: return gb_getNR34(apu);
        r_1F: r_20: return 0xFF;
        r_21: READ(apu->NR42);
        r_22: READ(apu->NR43);
        r_23: return gb_getNR44(apu);
        r_24: READ(apu->NR50);
        r_25: READ(apu->NR51);
        r_26: return gb_getNR52(apu);
        r_27: r_28: r_29: r_2A: r_2B: r_2C: r_2D: r_2E: r_2F: return 0xFF;
        r_30: r_31: r_32: r_33: r_34: r_35: r_36: r_37:
        r_38: r_39: r_3A: r_3B: r_3C: r_3D: r_3E: r_3F: return apu->WAVE_RAM[gb_getWaveRamAddress(apu, address - 0xFF30)];
        r_40: READ(ppu->LCDC);
        r_41: return gb_getStatRegister(ppu);
        r_42: READ(ppu->SCY);
        r_43: READ(ppu->SCX);
        r_44: READ(ppu->LY);
        r_45: READ(ppu->LYC);
        r_46: READ(dma->DMA);
        r_47: READ(ppu->BGP);
        r_48: READ(ppu->OBP0);
        r_49: READ(ppu->OBP1);
        r_4A: READ(ppu->WY);
        r_4B: READ(ppu->WX);
        r_4C: CGB_MODE_READ( READ(gb->KEY0); );
        r_4D: CGB_MODE_READ( return gb->KEY1_REG | 0b01111110;);
        r_4E: return 0XFF;
        r_4F: CGB_MODE_READ( return gb->VBK_REG | 0xFE; );
        r_50: r_51: r_52: r_53: r_54: return 0xFF;
        r_55: CGB_MODE_READ( return dma->HDMA_REGS[4]; );
        r_56: r_57:
        r_58: r_59: r_5A: r_5B: r_5C: r_5D: r_5E: r_5F:
        r_60: r_61: r_62: r_63: r_64: r_65: r_66: r_67: return 0xFF;
        r_68: CGB_MODE_READ( READ(ppu->BCPS); ); 
        r_69: CGB_MODE_READ( return gb_readCRAM(&ppu->BCPS_REG, gb->BGP_CRAM); );
        r_6A: CGB_MODE_READ( READ(ppu->OCPS); );
        r_6B: CGB_MODE_READ( return gb_readCRAM(&ppu->OCPS_REG, gb->OBP_CRAM); );
        r_6C: r_6D: r_6E: r_6F: return 0xFF;
        r_70: CGB_MODE_READ( return gb->SVBK_REG | 0xF8; );
        r_71: r_72: r_73: r_74: r_75: r_76: r_77:
        r_78: r_79: r_7A: r_7B: r_7C: r_7D: r_7E: r_7F: return 0xFF;
        r_80: r_81: r_82: r_83: r_84: r_85: r_86: r_87:
        r_88: r_89: r_8A: r_8B: r_8C: r_8D: r_8E: r_8F:
        r_90: r_91: r_92: r_93: r_94: r_95: r_96: r_97:
        r_98: r_99: r_9A: r_9B: r_9C: r_9D: r_9E: r_9F:
        r_A0: r_A1: r_A2: r_A3: r_A4: r_A5: r_A6: r_A7:
        r_A8: r_A9: r_AA: r_AB: r_AC: r_AD: r_AE: r_AF:
        r_B0: r_B1: r_B2: r_B3: r_B4: r_B5: r_B6: r_B7:
        r_B8: r_B9: r_BA: r_BB: r_BC: r_BD: r_BE: r_BF:
        r_C0: r_C1: r_C2: r_C3: r_C4: r_C5: r_C6: r_C7:
        r_C8: r_C9: r_CA: r_CB: r_CC: r_CD: r_CE: r_CF:
        r_D0: r_D1: r_D2: r_D3: r_D4: r_D5: r_D6: r_D7:
        r_D8: r_D9: r_DA: r_DB: r_DC: r_DD: r_DE: r_DF:
        r_E0: r_E1: r_E2: r_E3: r_E4: r_E5: r_E6: r_E7:
        r_E8: r_E9: r_EA: r_EB: r_EC: r_ED: r_EE: r_EF:
        r_F0: r_F1: r_F2: r_F3: r_F4: r_F5: r_F6: r_F7:
        r_F8: r_F9: r_FA: r_FB: r_FC: r_FD: r_FE: return gb->HRAM[address - 0xFF80];
        r_FF: return cpu->IE;
    }
}

u8 gb_readCRAM(u8* idx_reg, u8* cram){
    u8 addr = (*idx_reg) & 0b111111;
    return cram[addr];
}

// write callbacks

void gb_writeVram(gb_t* gb, u16 address, u8 byte){
    gb->VRAM[(address - 0x8000) + gb->VBK_REG * 0x2000] = byte;
}

void gb_writeWram(gb_t* gb, u16 address, u8 byte){
    u8 bank = 0;
    if(address >= 0xD000){
        address -= 0xD000;
        bank = gb->SVBK_REG ? gb->SVBK_REG : 1;
    }
    gb->WRAM[(address & (0x2000 - 1)) + bank * 0x1000] = byte;
}

void gb_writeMirrorRam(gb_t* gb, u16 address, u8 byte){
    gb->WRAM[(address - 0xE000)] = byte;
}

void gb_writeOam(gb_t* gb, u16 address, u8 byte){
    if(address < 0xFEA0)
        gb->OAM[address - 0xFE00] = byte;
}

void gb_writeIO(gb_t* gb, u16 address, u8 byte){
    static const void* write_io[0x100] = {
        &&w_00, &&w_01, &&w_02, &&w_03, &&w_04, &&w_05, &&w_06, &&w_07, &&w_08, &&w_09, &&w_0A, &&w_0B, &&w_0C, &&w_0D, &&w_0E, &&w_0F,
        &&w_10, &&w_11, &&w_12, &&w_13, &&w_14, &&w_15, &&w_16, &&w_17, &&w_18, &&w_19, &&w_1A, &&w_1B, &&w_1C, &&w_1D, &&w_1E, &&w_1F,
        &&w_20, &&w_21, &&w_22, &&w_23, &&w_24, &&w_25, &&w_26, &&w_27, &&w_28, &&w_29, &&w_2A, &&w_2B, &&w_2C, &&w_2D, &&w_2E, &&w_2F,
        &&w_30, &&w_31, &&w_32, &&w_33, &&w_34, &&w_35, &&w_36, &&w_37, &&w_38, &&w_39, &&w_3A, &&w_3B, &&w_3C, &&w_3D, &&w_3E, &&w_3F,
        &&w_40, &&w_41, &&w_42, &&w_43, &&w_44, &&w_45, &&w_46, &&w_47, &&w_48, &&w_49, &&w_4A, &&w_4B, &&w_4C, &&w_4D, &&w_4E, &&w_4F,
        &&w_50, &&w_51, &&w_52, &&w_53, &&w_54, &&w_55, &&w_56, &&w_57, &&w_58, &&w_59, &&w_5A, &&w_5B, &&w_5C, &&w_5D, &&w_5E, &&w_5F,
        &&w_60, &&w_61, &&w_62, &&w_63, &&w_64, &&w_65, &&w_66, &&w_67, &&w_68, &&w_69, &&w_6A, &&w_6B, &&w_6C, &&w_6D, &&w_6E, &&w_6F,
        &&w_70, &&w_71, &&w_72, &&w_73, &&w_74, &&w_75, &&w_76, &&w_77, &&w_78, &&w_79, &&w_7A, &&w_7B, &&w_7C, &&w_7D, &&w_7E, &&w_7F,
        &&w_80, &&w_81, &&w_82, &&w_83, &&w_84, &&w_85, &&w_86, &&w_87, &&w_88, &&w_89, &&w_8A, &&w_8B, &&w_8C, &&w_8D, &&w_8E, &&w_8F,
        &&w_90, &&w_91, &&w_92, &&w_93, &&w_94, &&w_95, &&w_96, &&w_97, &&w_98, &&w_99, &&w_9A, &&w_9B, &&w_9C, &&w_9D, &&w_9E, &&w_9F,
        &&w_A0, &&w_A1, &&w_A2, &&w_A3, &&w_A4, &&w_A5, &&w_A6, &&w_A7, &&w_A8, &&w_A9, &&w_AA, &&w_AB, &&w_AC, &&w_AD, &&w_AE, &&w_AF,
        &&w_B0, &&w_B1, &&w_B2, &&w_B3, &&w_B4, &&w_B5, &&w_B6, &&w_B7, &&w_B8, &&w_B9, &&w_BA, &&w_BB, &&w_BC, &&w_BD, &&w_BE, &&w_BF,
        &&w_C0, &&w_C1, &&w_C2, &&w_C3, &&w_C4, &&w_C5, &&w_C6, &&w_C7, &&w_C8, &&w_C9, &&w_CA, &&w_CB, &&w_CC, &&w_CD, &&w_CE, &&w_CF,
        &&w_D0, &&w_D1, &&w_D2, &&w_D3, &&w_D4, &&w_D5, &&w_D6, &&w_D7, &&w_D8, &&w_D9, &&w_DA, &&w_DB, &&w_DC, &&w_DD, &&w_DE, &&w_DF,
        &&w_E0, &&w_E1, &&w_E2, &&w_E3, &&w_E4, &&w_E5, &&w_E6, &&w_E7, &&w_E8, &&w_E9, &&w_EA, &&w_EB, &&w_EC, &&w_ED, &&w_EE, &&w_EF,
        &&w_F0, &&w_F1, &&w_F2, &&w_F3, &&w_F4, &&w_F5, &&w_F6, &&w_F7, &&w_F8, &&w_F9, &&w_FA, &&w_FB, &&w_FC, &&w_FD, &&w_FE, &&w_FF,
    };

    if(gb->console_type == MEGADUCK_TYPE){
        address = megaduckShuffleRegisters(address);
        byte = megaduckNibbleSwap(address, byte);
    }

    sm83_t* cpu = &gb->cpu;
    gb_timer_t* tmr = &gb->timer;
    dma_t* dma = &gb->dma;
    joypad_t* joy = &gb->joypad;
    serial_t* serial = &gb->serial;
    ppu_t* ppu = &gb->ppu;
    apu_t* apu = &gb->apu;

    goto *(write_io[address & 0xFF]);

    {
        w_00: WRITE(joy->JOYP);
        w_01: WRITE(serial->SB);
        w_02: WRITE(serial->SC);
        w_03: return;
        w_04: tmr->counter = 0x00; return;
        w_05: 
                if(!tmr->ignore_write)
                    tmr->TIMA_REG = byte;
                tmr->delay = 0;
                return;
        w_06: 
                tmr->TMA_REG = byte;
                if(tmr->ignore_write)
                    tmr->TIMA_REG = tmr->TMA_REG;
                return;
        w_07: WRITE(tmr->TAC);
        w_08: w_09: w_0A: w_0B: w_0C: w_0D: w_0E: return;
        w_0F: cpu->IF = byte; return;
        w_10: WRITE(apu->NR10);
        w_11: apu->NR11_REG = byte; gb_setChannel1Timer(apu); return;
        w_12: apu->NR12_REG = byte; gb_checkDAC1(apu); return;
        w_13: WRITE(apu->NR13);
        w_14: apu->NR14_REG = byte; gb_triggerChannel1(apu); return;
        w_15: return;
        w_16: apu->NR21_REG = byte; gb_setChannel2Timer(apu); return;
        w_17: apu->NR22_REG = byte; gb_checkDAC2(apu); return;
        w_18: WRITE(apu->NR23);
        w_19: apu->NR24_REG = byte; gb_triggerChannel2(apu); return;
        w_1A: apu->NR30_REG = byte; gb_checkDAC3(apu); return;
        w_1B: apu->NR31_REG = byte; gb_setChannel3Timer(apu); return;
        w_1C: WRITE(apu->NR32);
        w_1D: WRITE(apu->NR33);
        w_1E: apu->NR34_REG = byte; gb_triggerChannel3(apu); return;
        w_1F: return;
        w_20: apu->NR41_REG = byte; gb_setChannel4Timer(apu); return;
        w_21: apu->NR42_REG = byte; gb_checkDAC4(apu); return;
        w_22: WRITE(apu->NR43);
        w_23: apu->NR44_REG = byte; gb_triggerChannel4(apu); return;
        w_24: WRITE(apu->NR50);
        w_25: WRITE(apu->NR51);
        w_26: WRITE(apu->NR52);
        w_27: w_28: w_29: w_2A: w_2B: w_2C: w_2D: w_2E: w_2F: return;
        w_30: w_31: w_32: w_33: w_34: w_35: w_36: w_37:
        w_38: w_39: w_3A: w_3B: w_3C: w_3D: w_3E: w_3F: apu->WAVE_RAM[gb_getWaveRamAddress(apu, address - 0xFF30)] = byte; return;
        w_40: WRITE(ppu->LCDC);
        w_41:
                if(gb->console_type != CGB_TYPE){
                    if(!ppu->stat_irq && ppu->mode != OAM_SCAN_MODE)
                        cpu->IF |= STAT_IRQ;
                    ppu->stat_irq = true;
                }
                ppu->STAT_REG = byte & 0b01111000;
                return;
        w_42: WRITE(ppu->SCY);
        w_43: WRITE(ppu->SCX);
        w_44: return;
        w_45: WRITE(ppu->LYC);
        w_46: dma->DMA_REG = byte; gb_startDMA(gb); return;
        w_47: WRITE(ppu->BGP);
        w_48: WRITE(ppu->OBP0);
        w_49: WRITE(ppu->OBP1);
        w_4A: WRITE(ppu->WY);
        w_4B: WRITE(ppu->WX);
        w_4C: CGB_MODE_WRITE( WRITE(gb->KEY0); );
        w_4D: CGB_MODE_WRITE( if(byte & 1) gb->KEY1_REG |= 0x80; return; );
        w_4E: return;
        w_4F: CGB_MODE_WRITE( gb->VBK_REG = byte & 1; return; );
        w_50: 
                gb_fillReadTable(gb->readTable, 0x00, 0x09, gb->mbc.mapper_0000_3FFF);
                if((gb->KEY0_REG >> 2) == 0b01)
                    gb->console_type = DMG_ON_CGB_TYPE;
                return;
        w_51: CGB_MODE_WRITE( dma->HDMA_REGS[0] = byte; return; );
        w_52: CGB_MODE_WRITE( dma->HDMA_REGS[1] = byte; return; );
        w_53: CGB_MODE_WRITE( dma->HDMA_REGS[2] = byte; return; );
        w_54: CGB_MODE_WRITE( dma->HDMA_REGS[3] = byte; return; );
        w_55: CGB_MODE_WRITE( gb_startHDMA(gb, byte); return; );
        w_56: w_57:
        w_58: w_59: w_5A: w_5B: w_5C: w_5D: w_5E: w_5F:
        w_60: w_61: w_62: w_63: w_64: w_65: w_66: w_67: return;
        w_68: CGB_MODE_WRITE( WRITE(ppu->BCPS); );
        w_69: CGB_MODE_WRITE( gb_writeCRAM(&ppu->BCPS_REG, byte, gb->BGP_CRAM); return; );
        w_6A: CGB_MODE_WRITE( WRITE(ppu->OCPS); );
        w_6B: CGB_MODE_WRITE( gb_writeCRAM(&ppu->OCPS_REG, byte, gb->OBP_CRAM); return; );
        w_6C: w_6D: w_6E: w_6F: return;
        w_70: CGB_MODE_WRITE( gb->SVBK_REG = byte & 0b111; return; );
        w_71: w_72: w_73: w_74: w_75: w_76: w_77:
        w_78: w_79: w_7A: w_7B: w_7C: w_7D: w_7E: w_7F: return;
        w_80: w_81: w_82: w_83: w_84: w_85: w_86: w_87:
        w_88: w_89: w_8A: w_8B: w_8C: w_8D: w_8E: w_8F:
        w_90: w_91: w_92: w_93: w_94: w_95: w_96: w_97:
        w_98: w_99: w_9A: w_9B: w_9C: w_9D: w_9E: w_9F:
        w_A0: w_A1: w_A2: w_A3: w_A4: w_A5: w_A6: w_A7:
        w_A8: w_A9: w_AA: w_AB: w_AC: w_AD: w_AE: w_AF:
        w_B0: w_B1: w_B2: w_B3: w_B4: w_B5: w_B6: w_B7:
        w_B8: w_B9: w_BA: w_BB: w_BC: w_BD: w_BE: w_BF:
        w_C0: w_C1: w_C2: w_C3: w_C4: w_C5: w_C6: w_C7:
        w_C8: w_C9: w_CA: w_CB: w_CC: w_CD: w_CE: w_CF:
        w_D0: w_D1: w_D2: w_D3: w_D4: w_D5: w_D6: w_D7:
        w_D8: w_D9: w_DA: w_DB: w_DC: w_DD: w_DE: w_DF:
        w_E0: w_E1: w_E2: w_E3: w_E4: w_E5: w_E6: w_E7:
        w_E8: w_E9: w_EA: w_EB: w_EC: w_ED: w_EE: w_EF:
        w_F0: w_F1: w_F2: w_F3: w_F4: w_F5: w_F6: w_F7:
        w_F8: w_F9: w_FA: w_FB: w_FC: w_FD: w_FE: gb->HRAM[address - 0xFF80] = byte; return;
        w_FF: cpu->IE = byte; return;
    }
}

u16 megaduckShuffleRegisters(u16 address){
    static const u8 megaduck_conversion_table[256] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x40, 0x41, 0x42, 0x43, 0x48, 0x49, 0x4A, 0x4B, 0x44, 0x45, 0x46, 0x47, 0x1C, 0x1D, 0x1E, 0x1F,
        0x10, 0x12, 0x11, 0x13, 0x14, 0x16, 0x26, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1E, 0x1D, 0x2F,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
        0x20, 0x22, 0x21, 0x23, 0x24, 0x26, 0x25, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
        0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
        0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
        0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
        0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    };

    return (0xFF << 8) | megaduck_conversion_table[address & 0xFF]; 
}

void gb_writeCRAM(u8* idx_reg, u8 byte, u8* cram){
    u8 addr = (*idx_reg) & 0b111111;
    bool auto_inc = (*idx_reg) >> 7;
    cram[addr] = byte;
    if(auto_inc){
        addr = (addr + 1) % CRAM_SIZE;
        *idx_reg = (1 << 7) | addr;
    }
}

u8 megaduckNibbleSwap(u16 address, u8 byte){
    if(
        address == NR12_ADDR || address == NR22_ADDR ||
        address == NR42_ADDR || address == NR43_ADDR
    )
        return (byte << 4) | (byte >> 4);

    if(address == NR32_ADDR)
        return ((~byte) + 0x20) & 0x60;

    return byte;
}