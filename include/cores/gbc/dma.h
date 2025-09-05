#ifndef __DMA_H__
#define __DMA_H__

#include "types.h"

#define DMA_ADDR 0xFF46

typedef struct dma_t {
    u8 DMA_REG;
    u8 HDMA_REGS[5];   
    bool hblank_dma_started; 
} dma_t;

typedef struct gb_t gb_t;

void gb_initHDMA(dma_t*);
void gb_startDMA(gb_t* gb);
void gb_startHDMA(gb_t* gb, u8 byte);
void gb_stepHDMA(gb_t*);

#endif