#include "cores/gbc/gb.h"

static void getInfoHDMA(dma_t*, u16* srcAddr, u16* dstAddr, u16* length);
static void transferBlockHDMA(sm83_t* cpu, u16* srcAddr, u16* dstAddr);

void gb_initHDMA(dma_t* dma){
    memset(dma->HDMA_REGS, 0, sizeof(dma->HDMA_REGS));
    dma->hblank_dma_started = false;
}

void gb_startDMA(gb_t* gb){
    dma_t* dma = &gb->dma;
    u16 hi8 = dma->DMA_REG << 8;
    if(hi8 >> 12 == 0xF)
        hi8 = 0xD000 | (hi8 & 0xFFF);
    for(u8 lo8 = 0x00; lo8 < 0xA0; lo8++){
        gb->OAM[lo8] = gb->cpu.readByte(gb, hi8 | lo8);
    }
}

void gb_startHDMA(gb_t* gb, u8 byte){
    dma_t* dma = &gb->dma;
    if(byte & 0x80){
        dma->hblank_dma_started = true;
        dma->HDMA_REGS[4] = byte & 0x7F;

        if(gb->ppu.mode == HBLANK_MODE && !gb->cpu.HALTED)
            gb_stepHDMA(gb);
    } else {
        if(dma->hblank_dma_started){
            dma->hblank_dma_started = false;
            dma->HDMA_REGS[4] |= 0x80;
        } else {
            dma->HDMA_REGS[4] = byte;

            u16 srcAddr;
            u16 dstAddr;
            u16 length;

            getInfoHDMA(dma, &srcAddr, &dstAddr, &length);

            while(length){
                transferBlockHDMA(&gb->cpu, &srcAddr, &dstAddr);
                length--;
            }

            dma->HDMA_REGS[0] = srcAddr >> 8;
            dma->HDMA_REGS[1] = srcAddr;
            dma->HDMA_REGS[2] = dstAddr >> 8;
            dma->HDMA_REGS[3] = dstAddr;
            dma->HDMA_REGS[4] = 0xFF;
        }
    }   
}

void gb_stepHDMA(gb_t* gb){
    dma_t* dma = &gb->dma;
    u16 srcAddr;
    u16 dstAddr;
    u16 length;

    if(!dma->hblank_dma_started)
        return;

    getInfoHDMA(dma, &srcAddr, &dstAddr, &length);  

    transferBlockHDMA(&gb->cpu, &srcAddr, &dstAddr);
    length--;

    dma->HDMA_REGS[0] = srcAddr >> 8;
    dma->HDMA_REGS[1] = srcAddr;
    dma->HDMA_REGS[2] = dstAddr >> 8;
    dma->HDMA_REGS[3] = dstAddr;

    if(!length){
        dma->hblank_dma_started = false;
        dma->HDMA_REGS[4] = 0xFF;
    } else
        dma->HDMA_REGS[4] = length - 1;
}

static void getInfoHDMA(dma_t* dma, u16* srcAddr, u16* dstAddr, u16* length){
    *srcAddr = ((dma->HDMA_REGS[0] << 8) | (dma->HDMA_REGS[1] & 0xF0));  
    *dstAddr = ((dma->HDMA_REGS[2] & 0x1F) << 8) | (dma->HDMA_REGS[3] & 0xF0) | 0x8000;
    *length =  ((dma->HDMA_REGS[4] & 0x7F) + 1);
}

static void transferBlockHDMA(sm83_t* cpu, u16* srcAddr, u16* dstAddr){
    for(int i = 0; i < 16; i++){
        u8 byte = cpu->readByte(cpu->ctx, *srcAddr);
        cpu->writeByte(cpu->ctx, *dstAddr, byte);
        (*srcAddr) += 1;
        (*dstAddr) += 1;
    }
}