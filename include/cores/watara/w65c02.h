#ifndef __M65C02_H__
#define __M65C02_H__

#include "types.h"

/* we don't need hi acc for this system */
#define W65C02_LO_ACC

typedef struct w65c02_t w65c02_t;
typedef u8 (*w65c02_read_func)(void*, u16);
typedef void (*w65c02_write_func)(void*, u16, u8);

typedef struct w65c02_t {
    u16 pc;
    u8 s;
    u8 p;
    u8 a;
    u8 x;
    u8 y;

    bool wait;
    bool stop;

    w65c02_read_func read;
    w65c02_write_func write;

    u32 cycles;

    u16 mem_addr;
    u8 op_arg;
    bool in_mem;
    bool slow_op;

    void* ctx;
} w65c02_t;

void w65c02_init(w65c02_t* cpu);
void w65c02_reset(w65c02_t* cpu);
void w65c02_nmi(w65c02_t* cpu);
void w65c02_irq(w65c02_t* cpu);
void w65c02_step(w65c02_t* cpu);
void w65c02_print(w65c02_t* cpu);
bool w65c02_interrupt_enabled(w65c02_t* cpu);

#endif