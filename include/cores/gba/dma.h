#ifndef __DMA_H__
#define __DMA_H__

#include "types.h"

typedef struct gba_t gba_t;

typedef enum DMA_IDX {DMA_OFF = -1, DMA_0 = 0, DMA_1 = 1, DMA_2 = 2, DMA_3 = 3} DMA_IDX;

typedef struct dma_t {
    u32 internal_source;
    u32 internal_dest;
    u32 bus;
    u32 DMASAD;
    u32 DMADAD;
    u32 DMACNT;
} dma_t;

void gba_trigger_dma(gba_t* gba, int i);
void gba_transfer_dma(gba_t* gba, int i);
void gba_update_hblank_dma(gba_t* gba);
void gba_update_vblank_dma(gba_t* gba);

#endif