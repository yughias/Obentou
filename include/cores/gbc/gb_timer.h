#ifndef __GB_TIMER_H__
#define __GB_TIMER_H__

#include "types.h"

typedef struct gb_t gb_t;

typedef struct {
    u8 TIMA_REG;
    u8 TMA_REG;
    u8 TAC_REG;
    
    union {
        struct {
            u8 internal;
            u8 div;
        };
        u16 counter;
    };
    bool old_state;
    bool ignore_write;
    u8 delay;
} gb_timer_t;

#define TIMER_ENABLE_MASK         0b00000100
#define TIMER_CLOCK_MASK          0b00000011

#define TIMA_ADDR 0xFF05
#define TMA_ADDR 0xFF06
#define TAC_ADDR 0xFF07
#define DIV_ADDR 0xFF04

void gb_updateTimer(gb_t*);

#endif