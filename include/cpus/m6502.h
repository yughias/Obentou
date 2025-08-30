#ifndef __M6502_H__
#define __M6502_H__

#include "types.h"

typedef struct m6502_t m6502_t;
typedef u8 (*m6502_read_func)(void*, u16);
typedef void (*m6502_write_func)(void*, u16, u8);

typedef struct m6502_t {
    u16 pc;
    u8 s;
    u8 p;
    u8 a;
    u8 x;
    u8 y;

    m6502_read_func read;
    m6502_write_func write;

    u32 cycles;

    u16 mem_addr;
    u8 op_arg;
    bool in_mem;

    void* ctx;
} m6502_t;

void m6502_init(m6502_t* cpu);
void m6502_reset(m6502_t* cpu);
void m6502_nmi(m6502_t* cpu);
void m6502_irq(m6502_t* cpu);
void m6502_step(m6502_t* cpu);
void m6502_print(m6502_t* cpu);
bool m6502_interrupt_enabled(m6502_t* cpu);

#endif