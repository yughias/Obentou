#ifndef __TIMER_H__
#define __TIMER_H__

#include "types.h"

#include "utils/serializer.h"

#include "cores/gba/scheduler.h"

typedef struct gba_t gba_t;

#define GBA_TMR_STRUCT(X) \
    X(u64, started_clock, 1, 0) \
    X(u32, started_value, 1, 0) \
    X(u32, counter, 1, 0) \
    X(u8, speed_shift, 1, 0) \
    X(u32, TMCNT, 1, 0) \
    X(gba_scheduler_ptr_t, scheduled_event, 1, 1)
    
DECLARE_SERIALIZABLE_STRUCT(gba_tmr, GBA_TMR_STRUCT);

void gba_timer_trigger(gba_t* gba, int i);
void gba_timer_update_counter(gba_t* gba, int i);
void gba_timer_deschedule(gba_t* gba, int i);
void gba_timer_disable_cascade_mode(gba_t* gba, int i);

void gba_timer_event_overflow(gba_t* gba, u32 i);

#endif