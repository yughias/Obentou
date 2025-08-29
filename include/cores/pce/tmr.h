#ifndef __TMR_H__
#define __TMR_H__

#include "types.h"

typedef struct tmr_t {
    u8 counter;
    u8 reload;
    bool enabled;
    int prescaler;
    bool irq;
} tmr_t;

void pce_tmr_step(tmr_t* tmr, u32 elapsed);

#endif