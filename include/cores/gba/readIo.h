static u8 read_io_8(arm7tdmi_t* cpu, u32 addr) {
    gba_t* gba = (gba_t*)cpu->master;
    ppu_t* ppu = &gba->ppu;
    apu_t* apu = &gba->apu;

    switch(addr) {
        
        case 0x0 ... 0x1:
        return ((u8*)&ppu->DISPCNT)[addr - 0x0];
        
        case 0x4 ... 0x5:
        gba_ppu_compose_dispstat(ppu);
        return ((u8*)&ppu->DISPSTAT)[addr - 0x4];
        
        case 0x6 ... 0x7:
        return ((u8*)&ppu->VCOUNT)[addr - 0x6];
        
        case 0x8 ... 0xF:
        return ((u8*)&ppu->BGCNT[(addr - 0x8) / 2])[addr & 1];
        
        case 0x48 ... 0x4B:
        if (addr < 0x4A)
            return ((u8*)&ppu->WININ)[addr - 0x48];
        return ((u8*)&ppu->WINOUT)[addr - 0x4A];
        
        case 0x50 ... 0x53:
        if (addr < 0x52)
            return ((u8*)&ppu->BLDCNT)[addr - 0x50];
        return ((u8*)&ppu->BLDALPHA)[addr - 0x52];
        
        // apu reads
        case 0x60 ... 0x61:
        {
            u32 tmp = apu->SOUND1CNT_L & 0x7F;
            return ((u8*)&tmp)[addr - 0x60];
        }
        
        case 0x62 ... 0x63:
        {
            u32 tmp = apu->SOUND1CNT_H & 0xFFC0;
            return ((u8*)&tmp)[addr - 0x62];
        }
        
        case 0x64 ... 0x67:
        {
            u32 tmp = apu->SOUND1CNT_X & 0x4000;
            return ((u8*)&tmp)[addr - 0x64];
        }
        
        case 0x68 ... 0x6B:
        {
            u32 tmp = apu->SOUND2CNT_L & 0xFFC0;
            return ((u8*)&tmp)[addr - 0x68];
        }
        
        case 0x6C ... 0x6F:
        {
            u32 tmp = apu->SOUND2CNT_H & 0x4000;
            return ((u8*)&tmp)[addr - 0x6C];
        }
        
        case 0x70 ... 0x71:
        {
            u32 tmp = apu->SOUND3CNT_L & 0x00E0;
            return ((u8*)&tmp)[addr - 0x70];
        }
        
        case 0x72 ... 0x73:
        {
            u32 tmp = apu->SOUND3CNT_H & 0xE000;
            return ((u8*)&tmp)[addr - 0x72];
        }
        
        case 0x74 ... 0x77:
        {
            u32 tmp = apu->SOUND3CNT_X & 0x4000;
            return ((u8*)&tmp)[addr - 0x74];
        }
        
        case 0x78 ... 0x7B:
        {
            u32 tmp = apu->SOUND4CNT_L & 0xFF00;
            return ((u8*)&tmp)[addr - 0x78];
        }
        
        case 0x7C ... 0x7F:
        {
            u32 tmp = apu->SOUND4CNT_H & 0x40FF;
            return ((u8*)&tmp)[addr - 0x7C];
        }
        
        case 0x80 ... 0x81:
        return ((u8*)&apu->SOUNDCNT_L)[addr - 0x80];
        
        case 0x82 ... 0x83:
        return ((u8*)&apu->SOUNDCNT_H)[addr - 0x82];
        
        case 0x84:
        return gba_get_SOUNDCNT_X(apu);
        
        case 0x85 ... 0x87:
        return 0x00;
        
        case 0x88 ... 0x89:
        return ((u8*)&apu->SOUNDBIAS)[addr - 0x88];
        
        case 0x8A ... 0x8B:
        return 0x00;
        
        case 0x90 ... 0x9F:
        return gba_read_wave_ram(gba, addr - 0x90);
        
        // dma reads
        case 0xB8 ... 0xBB:
        case 0xC4 ... 0xC7:
        case 0xD0 ... 0xD3:
        case 0xDC ... 0xDF:
        {
            int dma_idx = (addr - 0xB0) / 12;
            int byte_idx = (addr - 0xB0) % 12;
            if (byte_idx < 10)
                return 0;
            return ((u8*)&gba->dmas[dma_idx].DMACNT)[byte_idx - 8];
        }
        
        // timer reads
        case 0x100 ... 0x10F:
        {
            int tmr = (addr - 0x100) / 4;
            int byte_idx = (addr - 0x100) % 4;
            if (byte_idx < 2) {
                gba_timer_update_counter(gba, tmr);
                return ((u8*)&gba->timers[tmr].counter)[byte_idx];
            }
            return ((u8*)&gba->timers[tmr].TMCNT)[byte_idx];
        }
        
        // misc
        case 0x130 ... 0x131:
        return (gba_keypad_read() >> ((addr - 0x130) * 8)) & 0xFF;
        
        case 0x132 ... 0x133:
        return ((u8*)&gba->KEYCNT)[addr - 0x132];
        
        case 0x134 ... 0x135:
        return ((u8*)&gba->RCNT)[addr - 0x134];
        
        case 0x136 ... 0x137:
        case 0x142 ... 0x143:
        case 0x15A ... 0x15B:
        return 0;
        
        // irqs and waitstates
        case 0x200 ... 0x201:
        return ((u8*)&gba->IE)[addr - 0x200];
        
        case 0x202 ... 0x203:
        return ((u8*)&gba->IF)[addr - 0x202];
        
        case 0x204 ... 0x205:
        return ((u8*)&gba->WAITCNT)[addr - 0x204];
        
        case 0x206 ... 0x207:
        return 0;
        
        case 0x208 ... 0x209:
        return ((u8*)&gba->IME)[addr - 0x208];
        
        case 0x20A ... 0x20B:
        return 0;
        
        case 0x300:
        return gba->POSTFLG;
        
        case 0x302 ... 0x303:
        return 0;
        
        default:
        return readOpenBus(cpu) >> ((addr & 0b11) << 3);
    }
}
