#include "cores/gbc/gb.h"

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

void* GBC_init(const char* filename){
    gb_t* gb = malloc(sizeof(gb_t));
    memset(gb, 0, sizeof(gb_t));
    sm83_t* cpu = &gb->cpu;
    cpu->readByte = gb_readByte;
    cpu->writeByte = gb_writeByte;
    cpu->tickSystem = tickHardware;
    cpu->ctx = gb;
    initCPU(cpu);
    gb_initMemory(gb, filename);
    gb_initAudio(&gb->apu);
    gb_initHDMA(&gb->dma);
    gb_initSerial();
    gb_initJoypad();
    gb_initLcdcMasks(gb);
    gb_initColorPalette(gb);
    gb->ppu.workingBufferPtr = gb->ppu.workingBuffer;
    gb->ppu.renderBufferPtr = gb->ppu.renderBuffer;
    gb_renderLcdOff(&gb->ppu);

    return gb;
}

bool GBC_detect(const char* filename){
    bool out = false;
    out |= SDL_strcasestr(filename, ".gb") != NULL;
    out |= SDL_strcasestr(filename, ".gbc") != NULL;
    out |= SDL_strcasestr(filename, ".megaduck") != NULL;

    if(SDL_strcasestr(filename, ".bin") != NULL){
        FILE* fptr = fopen(filename, "rb");
        size_t size = 0;
        if(fptr){
            fseek(fptr, 0, SEEK_END);
            size = ftell(fptr);
            fclose(fptr);
        }
        out |= size == (1 << 15);
        out |= size == (1 << 16);
        out |= size == (1 << 17);
    }
    return out;
}