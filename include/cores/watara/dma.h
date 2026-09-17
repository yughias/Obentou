#ifndef __DMA_H__
#define __DMA_H__

#include "types.h"

typedef u8 (*dma_read)(void* ctx, u16 addr);
typedef void (*dma_write)(void* ctx, u16 addr, u8 byte);

typedef struct dma_t {
    u8 cbus_lo;
    u8 cbus_hi;
    u8 vbus_lo;
    u8 vbus_hi;
    u8 len;

    dma_read read;
    dma_write write;

    void* ctx;
} dma_t; 

void watara_dma_trigger(dma_t* dma);

#endif