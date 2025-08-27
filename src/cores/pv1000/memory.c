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

uint8_t pv1000_readMemory(z80_t* z80, uint16_t addr){
    pv1000_t* pv1000 = z80->ctx;

    return pv1000->memory[addr];
}

void pv1000_writeMemory(z80_t* z80, uint16_t addr, uint8_t byte){
    pv1000_t* pv1000 = z80->ctx;

    if(addr < 0x8000)
        return;

    pv1000->memory[addr] = byte;
}

uint8_t pv1000_readIO(z80_t* z80, uint16_t addr){
    pv1000_t* pv1000 = z80->ctx;
    addr &= 0xFF;

    if(addr == 0xFC){
        uint8_t status = pv1000->status; 
        pv1000->status &= ~0b1;
        return status;
    }

    if(addr == 0xFD){
        return pv1000_controller_read(&pv1000->controller);
    }

    return 0x00;
}

void pv1000_writeIO(z80_t* z80, uint16_t addr, uint8_t byte){
    pv1000_t* pv1000 = z80->ctx;
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