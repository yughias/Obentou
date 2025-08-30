#ifndef __DMA_H__
#define __DMA_H__

#include "types.h"

typedef struct dma_t {
    bool enabled;
    u8 addr_hi;
    u8 addr_lo;
    bool read_cycle;
    u8 data;
} dma_t;

#endif