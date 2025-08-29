#include "cores/pce/tmr.h"

void pce_tmr_step(tmr_t* tmr, u32 elapsed) {
    if(!tmr->enabled)
        return;
    tmr->prescaler -= elapsed;
    while(tmr->prescaler < 0) {
        tmr->prescaler += 1024;
        if(tmr->counter)
            tmr->counter--;
        else {
            tmr->irq = true;
            tmr->counter = tmr->reload;
        }
    }
}