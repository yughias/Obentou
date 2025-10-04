#include "cores/pv1000/pv1000.h"
#include "cores/pv1000/memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/archive.h"

void* PV1000_init(const archive_t* rom_archive, const archive_t* bios_archive){
    pv1000_t* pv1000 = malloc(sizeof(pv1000_t));
    memset(pv1000, 0x00, sizeof(pv1000_t));
    
    z80_t* z80 = &pv1000->z80;
    z80->readMemory = pv1000_readMemory;
    z80->writeMemory = pv1000_writeMemory;
    z80->readIO = pv1000_readIO;
    z80->writeIO = pv1000_writeIO;
    z80->ctx = pv1000;
    pv1000->psg.updateRate = 1.0f / 44100.0f;

    
    memset(pv1000->memory, 0xFF, 0x1000);
    file_t* f = archive_get_file_by_ext(rom_archive, "pv");
    if(!f)
        f = archive_get_file_by_ext(rom_archive, "bin");
    memcpy(pv1000->memory, f->data, f->size);
    
    z80_init(z80);

    return pv1000;
}

void PV1000_run_frame(pv1000_t* pv1000){
    z80_t* z80 = &pv1000->z80;
    vdp_t* vdp = &pv1000->vdp;

    int next_interrupt = 196;

    while(z80->cycles < CYCLES_PER_FRAME){
        u32 old_cycles = z80->cycles;

        z80_step(z80);

        int old_lines = old_cycles / CYCLES_PER_LINE;
        int lines = z80->cycles / CYCLES_PER_LINE;

        if(old_lines != lines && lines == next_interrupt){
            if(next_interrupt == 196)
                pv1000->status |= 0b1;
            if(next_interrupt != 256)
                next_interrupt += 4;
            z80->INTERRUPT_PENDING = true;
        }
    }

    z80->cycles -= CYCLES_PER_FRAME;

    pv1000_vdp_render(vdp, pv1000->memory);
}

bool PV1000_detect(const archive_t* rom_archive, const archive_t* bios_archive){
    if(archive_get_file_by_ext(rom_archive, "pv"))
        return true;
    
    file_t* f;
    if(f = archive_get_file_by_ext(rom_archive, "bin")){
        size_t size = f->size;
        if(size == (1 << 13) || size == (1 << 14))
            return true;
    }
    
    return false;
}