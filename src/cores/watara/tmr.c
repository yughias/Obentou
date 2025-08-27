#include "cores/watara/tmr.h"

void watara_tmr_tick(tmr_t* tmr, u8 ctrl, int cycles){
    if(!tmr->ctr)
        return;
    tmr->divider += cycles;
    int scaler = ctrl & 0x10 ? 16384 : 256;
    while(tmr->divider >= scaler){
        tmr->divider -= scaler;
        if(tmr->ctr)
            tmr->ctr -= 1;
    }

    if(!tmr->ctr)
        tmr->irq = true;
}