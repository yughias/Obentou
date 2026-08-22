static void write_io_8(arm7tdmi_t* cpu, u32 addr, u8 val) {
    gba_t* gba = (gba_t*)cpu->master;
    ppu_t* ppu = &gba->ppu;
    apu_t* apu = &gba->apu;

    switch(addr) {
        
        case 0x0 ... 0x1:
        ((u8*)&ppu->DISPCNT)[addr - 0x0] = val;
        return;
        
        case 0x4 ... 0x5:
        ((u8*)&ppu->DISPSTAT)[addr - 0x4] = val;
        if (addr == 0x5)
            gba_ppu_check_vcount(gba);
        return;
        
        case 0x8 ... 0xF:
        if (addr == 0x9 || addr == 0xB)
            val &= 0xDF;
        ((u8*)&ppu->BGCNT[(addr - 0x8) / 2])[addr & 1] = val;
        return;
        
        case 0x10 ... 0x1F:
        {
            int bg = (addr - 0x10) / 4;
            if ((addr & 2) == 0)
                ((u8*)&ppu->BGHOFS[bg])[addr & 1] = val;
            else
                ((u8*)&ppu->BGVOFS[bg])[addr & 1] = val;
        }
        return;
        
        case 0x20 ... 0x27:
        ((u8*)&ppu->BGP[(addr - 0x20) / 2])[addr & 1] = val;
        return;
        
        case 0x28 ... 0x2F:
        case 0x38 ... 0x3F:
        {
            int bg = (addr >= 0x38) ? 1 : 0;
            int base = (addr >= 0x38) ? 0x38 : 0x28;
            if (addr - base < 4) {
                ((u8*)&ppu->BGX[bg])[addr - base] = val;
                ppu->INTERNAL_BGX[bg] = ppu->BGX[bg];
            } else {
                ((u8*)&ppu->BGY[bg])[addr - base - 4] = val;
                ppu->INTERNAL_BGY[bg] = ppu->BGY[bg];
            }
        }
        return;
        
        case 0x30 ... 0x37:
        ((u8*)&ppu->BGP[4 + (addr - 0x30) / 2])[addr & 1] = val;
        return;
        
        case 0x40 ... 0x47:
        if (addr < 0x44)
            ((u8*)&ppu->WINH[(addr - 0x40) / 2])[addr & 1] = val;
        else
            ((u8*)&ppu->WINV[(addr - 0x44) / 2])[addr & 1] = val;
        return;
        
        case 0x48 ... 0x4B:
        if (addr < 0x4A)
            ((u8*)&ppu->WININ)[addr - 0x48] = val & 0x3F;
        else
            ((u8*)&ppu->WINOUT)[addr - 0x4A] = val & 0x3F;
        return;
        
        case 0x50 ... 0x55:
        if (addr == 0x50)
            ((u8*)&ppu->BLDCNT)[0] = val;
        else if (addr == 0x51)
            ((u8*)&ppu->BLDCNT)[1] = val & 0x3F;
        else if (addr < 0x54)
            ((u8*)&ppu->BLDALPHA)[addr - 0x52] = val & 0x1F;
        else
            ((u8*)&ppu->BLDY)[addr - 0x54] = val;
        return;
        
        // apu
        case 0x60 ... 0x61:
        ((u8*)&apu->SOUND1CNT_L)[addr - 0x60] = val;
        return;
        
        case 0x62 ... 0x63:
        ((u8*)&apu->SOUND1CNT_H)[addr - 0x62] = val;
        gba_update_SOUND12CNT_duty(gba, apu->SOUND1CNT_H, 0);
        return;
        
        case 0x64 ... 0x65:
        ((u8*)&apu->SOUND1CNT_X)[addr - 0x64] = val;
        gba_update_SOUND12CNT_freq(gba, apu->SOUND1CNT_H, &apu->SOUND1CNT_X, 0);
        return;
        
        case 0x68 ... 0x69:
        ((u8*)&apu->SOUND2CNT_L)[addr - 0x68] = val;
        gba_update_SOUND12CNT_duty(gba, apu->SOUND2CNT_L, 1);
        return;
        
        case 0x6C ... 0x6D:
        ((u8*)&apu->SOUND2CNT_H)[addr - 0x6C] = val;
        gba_update_SOUND12CNT_freq(gba, apu->SOUND2CNT_L, &apu->SOUND2CNT_H, 1);
        return;
        
        case 0x70 ... 0x71:
        ((u8*)&apu->SOUND3CNT_L)[addr - 0x70] = val;
        gba_update_SOUND3CNT_L(gba);
        return;
        
        case 0x72 ... 0x73:
        ((u8*)&apu->SOUND3CNT_H)[addr - 0x72] = val;
        gba_update_SOUND3CNT_H(gba);
        return;
        
        case 0x74 ... 0x75:
        ((u8*)&apu->SOUND3CNT_X)[addr - 0x74] = val;
        gba_update_SOUND3CNT_X(gba);
        return;
        
        case 0x78 ... 0x79:
        ((u8*)&apu->SOUND4CNT_L)[addr - 0x78] = val;
        gba_update_SOUND4CNT_L(gba);
        return;
        
        case 0x7C ... 0x7D:
        ((u8*)&apu->SOUND4CNT_H)[addr - 0x7C] = val;
        gba_update_SOUND4CNT_H(gba);
        return;
        
        case 0x80 ... 0x81:
        ((u8*)&apu->SOUNDCNT_L)[addr - 0x80] = (addr == 0x80) ? (val & 0x77) : val;
        gba_update_SOUNDCNT_L(apu);
        return;
        
        case 0x82 ... 0x83:
        ((u8*)&apu->SOUNDCNT_H)[addr - 0x82] = val;
        gba_update_SOUNDCNT_H(apu);
        return;
        
        case 0x84:
        apu->SOUNDCNT_X = val;
        return;
        
        case 0x88 ... 0x89:
        ((u8*)&apu->SOUNDBIAS)[addr - 0x88] = val;
        return;
        
        case 0x90 ... 0x9F:
        gba_write_wave_ram(gba, addr - 0x90, val);
        return;
        
        case 0xA0 ... 0xA7:
        gba_push_into_fifo(&apu->fifo[(addr < 0xA4) ? 0 : 1], val);
        return;
        
        // dma
        
        case 0xB0 ... 0xDF:
        {
            int dma = (addr - 0xB0) / 12;
            int offset = (addr - 0xB0) % 12;
            
            if (offset < 4) {
                ((u8*)&gba->dmas[dma].DMASAD)[offset] = val;
            } else if (offset < 8) {
                ((u8*)&gba->dmas[dma].DMADAD)[offset - 4] = val;
            } else {
                int cnt_byte = offset - 8;
                u8 write_val = val;
                
                // specific hardware masks on the upper byte of DMACNT
                if (cnt_byte == 2)
                    write_val &= 0xE0;
                else if (cnt_byte == 3 && dma != 3)
                    write_val &= 0xF7; 
                
                bool old_trigger = gba->dmas[dma].DMACNT >> 31;
                ((u8*)&gba->dmas[dma].DMACNT)[cnt_byte] = write_val;
                bool new_trigger = gba->dmas[dma].DMACNT >> 31;
                
                if (!old_trigger && new_trigger)
                    gba_trigger_dma(gba, dma);
            }
        }
        return;

        // timers
        
        case 0x100 ... 0x10F:
        {
            int tmr = (addr - 0x100) / 4;
            int byte_idx = (addr - 0x100) % 4;
            gba_tmr_t* timer = &gba->timers[tmr];
            u32 old_TMCNT = timer->TMCNT;
            
            ((u8*)&timer->TMCNT)[byte_idx] = val;
            
            bool old_enabled = (old_TMCNT >> 16) & 0x80;
            bool old_cascade = (old_TMCNT >> 16) & 0b100;
            bool new_enabled = (timer->TMCNT >> 16) & 0x80;
            bool new_cascade = ((timer->TMCNT >> 16) & 0b100);
            
            if (!old_enabled && new_enabled)
                gba_timer_trigger(gba, tmr);
            else if ((old_enabled && !new_enabled) || (new_enabled && !old_cascade && new_cascade))
                gba_timer_deschedule(gba, tmr);
            else if (new_enabled && old_cascade && !new_cascade)
                gba_timer_disable_cascade_mode(gba, tmr);
            
        }
        return;

        
        case 0x134 ... 0x135:
        ((u8*)&gba->RCNT)[addr - 0x134] = val;
        return;

        // irqs and waitstates
        
        case 0x200 ... 0x201:
        ((u8*)&gba->IE)[addr - 0x200] = val;
        gba_check_interrupts(gba);
        return;
        
        case 0x202 ... 0x203:
        ((u8*)&gba->IF)[addr - 0x202] &= ~(val);
        return;
        
        case 0x204 ... 0x205:
        ((u8*)&gba->WAITCNT)[addr - 0x204] = val;
        gba_update_waitstates(&gba->gamepak, cpu, gba->WAITCNT);
        return;
        
        case 0x208 ... 0x209:
        ((u8*)&gba->IME)[addr - 0x208] = val;
        gba_check_interrupts(gba);
        return;

        // misc
        
        case 0x300:
        gba->POSTFLG = val;
        return;
        
        case 0x301:
        gba->HALTCNT = true;
        return;
            
        default:
        return;
    }
}
