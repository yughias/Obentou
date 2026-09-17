#include "cores/watara/dma.h"

void watara_dma_trigger(dma_t* dma){
    bool dma_dir = dma->vbus_hi & 0x40;
    u16 cbus_addr = dma->cbus_lo | (dma->cbus_hi << 8);
    u16 vbus_addr = dma->vbus_lo | ((dma->vbus_hi & 0x1F) << 8);
    vbus_addr += 0x4000;
    
    u16 len = (dma->len ? dma->len : 0x100) << 4;

    for(int i = 0; i < len; i++){
        if (dma_dir) {
            u8 byte = dma->read(dma->ctx, cbus_addr++);
            dma->write(dma->ctx, vbus_addr++, byte);
        } else {
            u8 byte = dma->read(dma->ctx, vbus_addr++);
            dma->write(dma->ctx, cbus_addr++, byte);
        }
    }

    dma->len = 0;
    dma->cbus_lo = cbus_addr & 0xFF;
    dma->cbus_hi = cbus_addr >> 8;
    dma->vbus_lo = vbus_addr & 0xFF;
    dma->vbus_hi = (dma->vbus_hi & 0xE0) | ((vbus_addr >> 8) & 0x1F);
}