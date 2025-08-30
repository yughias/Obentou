#include "cores/pv1000/memory.h"
#include "cores/pv1000/pv1000.h"

#define WRITE_TO_FREQ_REG(reg_addr, idx) \
if(addr == reg_addr){ \
    byte &= 0x3F; \
    if(byte == 0x3F) \
        psg->square_freq[idx] = 0; \
    else \
        psg->square_freq[idx] = CALCULATE_SQUARE_FREQ(byte); \
}

u8 pv1000_readMemory(void* ctx, u16 addr){
    pv1000_t* pv1000 = (pv1000_t*)ctx;
    return pv1000->memory[addr];
}

void pv1000_writeMemory(void* ctx, u16 addr, u8 byte){
    pv1000_t* pv1000 = (pv1000_t*)ctx;
    if(addr < 0x8000)
        return;

    pv1000->memory[addr] = byte;
}

u8 pv1000_readIO(void* ctx, u16 addr){
    pv1000_t* pv1000 = (pv1000_t*)ctx;
    addr &= 0xFF;

    if(addr == 0xFC){
        u8 status = pv1000->status; 
        pv1000->status &= ~0b1;
        return status;
    }

    if(addr == 0xFD){
        return pv1000_controller_read(&pv1000->controller);
    }

    return 0x00;
}

void pv1000_writeIO(void* ctx, u16 addr, u8 byte){
    pv1000_t* pv1000 = (pv1000_t*)ctx;
    vdp_t* vdp = &pv1000->vdp;
    psg_t* psg = &pv1000->psg;

    addr &= 0xFF;

    WRITE_TO_FREQ_REG(0xF8, 0);
    WRITE_TO_FREQ_REG(0xF9, 1);
    WRITE_TO_FREQ_REG(0xFA, 2);

    if(addr == 0xFB){
        byte &= 0b11;
        psg->mixer = byte & 0b1;
        psg->enabled = byte >> 1;
    }

    if(addr == 0xFD){
        pv1000->controller.selected_matrix = byte;
        pv1000->status |= 2;
    }

    if(addr == 0xFE){
        vdp->ram_tile_addr = ((byte >> 4) << 12) | (0b11 << 10);
    }

    if(addr == 0xFF){
        vdp->all_tiles_in_rom = (byte >> 4) & 1;
        vdp->rom_tile_addr = ((byte >> 5) & 0b111) * 0x2000;
    }
}