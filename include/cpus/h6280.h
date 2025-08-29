#ifndef __H6280_H__
#define __H6280_H__

#include "types.h"

typedef struct h6280_t h6280_t;
typedef u8 (*h6280_read_func)(void*, u32);
typedef void (*h6280_write_func)(void*, u32, u8);
typedef void (*h6280_io_func)(void*, u16, u8);

typedef struct h6280_t {
    u16 pc;
    u8 s;
    u8 p;
    u8 a;
    u8 x;
    u8 y;

    u8 mpr[8];

    bool wait;
    bool stop;
    bool hi_speed;
    bool irq_delay;

    h6280_read_func read;
    h6280_write_func write;
    h6280_io_func io_write;

    u32 cycles;

    u16 mem_addr;
    u8 op_arg;
    bool in_mem;
    bool slow_op;

    void* ctx;
} h6280_t;

void h6280_init(h6280_t* cpu);
void h6280_reset(h6280_t* cpu);
void h6280_nmi(h6280_t* cpu);
void h6280_irq(h6280_t* cpu, u16 vector);
void h6280_step(h6280_t* cpu);
void h6280_print(h6280_t* cpu);
void h6280_print_next_op(h6280_t* cpu);
bool h6280_interrupt_enabled(h6280_t* cpu);

#endif