#ifndef __TMR_H__
#define __TMR_H__

#include "types.h"

typedef struct tmr_t {
    bool irq;
    u8 ctr;
    int divider;
} tmr_t;

void watara_tmr_tick(tmr_t* tmr, u8 ctrl, int cycles);

#endif