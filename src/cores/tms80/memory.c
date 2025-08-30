#include "cores/tms80/tms80.h"

#include "types.h"

static u8 gg_internal_regs[] = {0xC0, 0x7F, 0xFF, 0x00, 0xFF, 0x00, 0xFF};

u8 tms80_sg_sc_readMemory(void* ctx, u16 addr){
    tms80_t* tms80 = (tms80_t*)ctx;

    if(addr < 0xC000 && addr < tms80->cartridge_size)
        return tms80->cartridge[addr];

    return tms80->RAM[addr];
}

u8 tms80_sg_sc_mirrored_readMemory(void* ctx, u16 addr){
    tms80_t* tms80 = (tms80_t*)ctx;

    if(addr < 0xC000)
        return tms80->cartridge[addr % tms80->cartridge_size];

    return tms80->RAM[addr];
}

void tms80_sg_sc_writeMemory(void* ctx, u16 addr, u8 byte){
    tms80_t* tms80 = (tms80_t*)ctx;

    tms80->RAM[addr] = byte;
}

void tms80_sg_sc_ram_adapter_writeMemory(void* ctx, u16 addr, u8 byte){
    tms80_t* tms80 = (tms80_t*)ctx;

    if(addr >= 0x2000 && addr < 0x4000){
        tms80->cartridge[addr] = byte;
        return;
    }

    tms80->RAM[addr] = byte;
}

u8 tms80_sms_bios_readMemory(void* ctx, u16 addr){
    tms80_t* tms80 = (tms80_t*)ctx;

    if(addr < 0xC000){
        u8 bank_idx = addr >> 14;
        u32 banked_addr = ((addr & 0x3FFF) + tms80->banks[bank_idx]*0x4000);
        return tms80->bios[banked_addr % tms80->bios_size];
    }

    return tms80->RAM[addr & ((1 << 13) - 1)];
}

void tms80_sms_bios_writeMemory(void* ctx, u16 addr, u8 byte){
    tms80_t* tms80 = (tms80_t*)ctx;

    if(addr == 0xFFFC){
        tms80->ram_bank = byte;
    }
    if(addr >= 0xFFFD && addr <= 0xFFFF)
        tms80->banks[addr - 0xFFFD] = byte;

    if(addr >= 0xC000)
        tms80->RAM[addr & ((1 << 13) - 1)] = byte;
}

u8 tms80_sms_readMemory(void* ctx, u16 addr){
    tms80_t* tms80 = (tms80_t*)ctx;
    
    if(addr < 0xC000){
        u8 bank_idx = addr >> 14;
        if((tms80->ram_bank & (1 << 3)) && addr >= 0x8000)
            return tms80->RAM[(1 << 13) + (addr - 0x8000)];
        if(addr < 1024)
            return tms80->cartridge[addr];
        u32 banked_addr = ((addr & 0x3FFF) + tms80->banks[bank_idx]*0x4000);
        return tms80->cartridge[banked_addr % tms80->cartridge_size];
    }

    return tms80->RAM[addr & ((1 << 13) - 1)];
}

void tms80_sms_writeMemory(void* ctx, u16 addr, u8 byte){
    tms80_t* tms80 = (tms80_t*)ctx;

    if(addr == 0xFFFC){
        tms80->ram_bank = byte;
    }
    if(addr >= 0xFFFD && addr <= 0xFFFF)
        tms80->banks[addr - 0xFFFD] = byte;

    if((tms80->ram_bank & (1 << 3)) && addr >= 0x8000 && addr < 0xC000){
        tms80->RAM[(1 << 13) + (addr - 0x8000)] = byte;
    }

    if(addr >= 0xC000)
        tms80->RAM[addr & ((1 << 13) - 1)] = byte;
}

u8 tms80_readIO(void* ctx, u16 addr){
    tms80_t* tms80 = (tms80_t*)ctx;
    vdp_t* vdp = &tms80->vdp;
    addr &= 0xFF;

    
    if(tms80->type == GG){
        if(!addr)
            return tms80_gg_get_start_button();

        if(addr <= 0x6)
            return gg_internal_regs[addr];
    }

    if(addr >= 0x40 && addr <= 0x7F){
        if(!(addr & 1))
            return tms80_vdp_get_v_counter(vdp);
    }

    if(addr >= 0xC0 && addr <= 0xFF){
        if(addr & 1)
            return tms80_get_keypad_b(tms80);
        else
            return tms80_get_keypad_a(tms80);
    }

    if(addr >= 0x80 && addr <= 0xBF){
        if(addr & 1){
            return tms80_vdp_read_status_register(vdp, &tms80->z80);
        } else
            return tms80_vdp_read_from_data_port(vdp);
    }

    return 0xFF;
}

void tms80_writeIO(void* ctx, u16 addr, u8 byte){
    tms80_t* tms80 = (tms80_t*)ctx;
    vdp_t* vdp = &tms80->vdp;
    sn76489_t* apu = &tms80->apu;
    addr &= 0xFF;

    if(tms80->type == SMS && addr == 0x3E){
        if((byte & (1 << 3))){
            tms80->z80.readMemory = tms80_sms_readMemory;
            tms80->z80.writeMemory = tms80_sms_writeMemory;
        } else if(tms80->bios) {
            tms80->z80.readMemory = tms80_sms_bios_readMemory;
            tms80->z80.writeMemory = tms80_sms_bios_writeMemory;
        }
    }

    if(tms80->has_keyboard && addr == 0xDE)
        tms80->keypad_reg = byte;

    if(addr >= 0x40 && addr <= 0x7F)
        tms80_sn76489_write(apu, byte);

    if(addr >= 0x80 && addr <= 0xBF){
        if(addr & 1){
            tms80_vdp_write_to_control_port(vdp, byte);
        } else
            tms80_vdp_write_to_data_port(vdp, byte);
    }
}