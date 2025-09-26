#include "cores/gbc/gb.h"

#include "peripherals/archive.h"

#include <stdio.h>
#include <stdlib.h>

void GBC_run_frame(gb_t* gb){
    sm83_t* cpu = &gb->cpu;
    while(cpu->cycles < CYCLES_PER_FRAME){
        #ifdef DEBUG
        infoCPU(cpu);
        #endif
        stepCPU(cpu);
    }
    gb->startFrame_clock += cpu->cycles;
    cpu->cycles -= CYCLES_PER_FRAME;

    // copy framebuffer to pixels
    memcpy(pixels, gb->ppu.renderBufferPtr, sizeof(int)*LCD_WIDTH*LCD_HEIGHT);
    renderPixels();
}

static void tickHardware(void* ctx, int ticks){
    gb_t* gb = (gb_t*)ctx;
    apu_t* apu = &gb->apu;
    bool fast_mode = gb->KEY1_REG & 0x80;

    if(fast_mode)
        ticks /= 2;
       
    gb->cpu.cycles += ticks;

    for(int i = 0; i < ticks; i++){
        gb_updateSerial(gb);
        gb_emulateApu(apu);
        gb_convertAudio(apu);
        gb_updatePPU(gb);
        gb_updateTimer(gb);
    }
    
    if(fast_mode){
        for(int i = 0; i < ticks; i++){
            gb_updateSerial(gb);
            gb_updateTimer(gb);
        }
    }
}

void* GBC_init(const archive_t* rom_archive, const archive_t* bios_archive){
    gb_t* gb = malloc(sizeof(gb_t));
    memset(gb, 0, sizeof(gb_t));
    sm83_t* cpu = &gb->cpu;
    cpu->readByte = gb_readByte;
    cpu->writeByte = gb_writeByte;
    cpu->tickSystem = tickHardware;
    cpu->ctx = gb;
    initCPU(cpu);
    gb_initMemory(gb, rom_archive, bios_archive);
    gb_initAudio(&gb->apu);
    gb_initHDMA(&gb->dma);
    gb_initSerial();
    gb_initLcdcMasks(gb);
    gb_initColorPalette(gb);
    gb->ppu.workingBufferPtr = gb->ppu.workingBuffer;
    gb->ppu.renderBufferPtr = gb->ppu.renderBuffer;
    gb_renderLcdOff(&gb->ppu);

    return gb;
}

bool GBC_detect(const archive_t* rom_archive, const archive_t* bios_archive){
    bool out = false;
    out |= archive_get_file_by_ext(rom_archive, "gb") != NULL;
    out |= archive_get_file_by_ext(rom_archive, "gbc") != NULL;
    out |= archive_get_file_by_ext(rom_archive, "megaduck") != NULL;

    file_t* f;
    if(f = archive_get_file_by_ext(rom_archive, "bin")){
        size_t size = f->size;
        out |= size == (1 << 15);
        out |= size == (1 << 16);
        out |= size == (1 << 17);
    }
    return out;
}

void GBC_close(gb_t* gb, const char* sav_path){
    gb_saveSav(gb, sav_path);
    free(gb->mbc.data);
}