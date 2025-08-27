#include "cores/watara/dma.h"

void watara_dma_trigger(dma_t* dma){
    u16 src = dma->src_lo | (dma->src_hi << 8);
    u16 dst = dma->dst_lo | (dma->dst_hi << 8);
    u16 len = (dma->len ? dma->len : 0x100) << 4;

    for(int i = 0; i < len; i++){
        u8 byte = dma->read(dma->ctx, src++);
        dma->write(dma->ctx, dst++, byte);
    }

    dma->len = 0;
    dma->src_lo = src & 0xFF;
    dma->src_hi = src >> 8;
    dma->dst_lo = dst & 0xFF;
    dma->dst_hi = dst >> 8;
}