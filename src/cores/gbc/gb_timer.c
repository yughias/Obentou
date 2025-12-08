#include "cores/gbc/gb_timer.h"
#include "cores/gbc/gb.h"

#define RESET_DELAY 4

static bool isNegativeEdge(bool, bool);

void gb_updateTimer(gb_t* gb){
    gb_timer_t* tmr = &gb->timer;
    tmr->counter++;
    tmr->ignore_write = false;

    if(tmr->delay && !(--tmr->delay)){
        gb->cpu.IF |= TIMER_IRQ;
        tmr->TIMA_REG = tmr->TMA_REG;
        tmr->ignore_write = true;
    }

    u16 timer_masks[4] = {1 << 9, 1 << 3, 1 << 5, 1 << 7};
    u16 timer_mask = timer_masks[tmr->TAC_REG & TIMER_CLOCK_MASK];
    bool new_state = (tmr->counter & timer_mask) && (tmr->TAC_REG & TIMER_ENABLE_MASK);
    if(isNegativeEdge(tmr->old_state, new_state)){
        if(tmr->TIMA_REG == 0xFF){
            tmr->TIMA_REG = 0x00;
            tmr->delay = RESET_DELAY;
        } else {
            tmr->TIMA_REG++;
        }
    }

    tmr->old_state = new_state;
}

bool isNegativeEdge(bool old_state, bool new_state){
    return old_state && !new_state;
}