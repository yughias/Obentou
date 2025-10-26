#ifndef __M6502_H__
#define __M6502_H__

#include "types.h"

#include "utils/serializer.h"

typedef struct m6502_t m6502_t;
typedef u8 (*m6502_read_func)(void*, u16);
typedef void (*m6502_write_func)(void*, u16, u8);

#define M6502_STRUCT(X) \
    X(u16, pc, 1, 0) \
    X(u8, s, 1, 0) \
    X(u8, p, 1, 0) \
    X(u8, a, 1, 0) \
    X(u8, x, 1, 0) \
    X(u8, y, 1, 0) \
    X(m6502_read_func, read, 0, 0) \
    X(m6502_write_func, write, 0, 0) \
    X(u32, cycles, 1, 0) \
    X(u16, mem_addr, 1, 0) \
    X(u8, op_arg, 1, 0) \
    X(bool, in_mem, 1, 0) \
    X(void*, ctx, 0, 0)

DECLARE_SERIALIZABLE_STRUCT(m6502, M6502_STRUCT)

void m6502_init(m6502_t* cpu);
void m6502_reset(m6502_t* cpu);
void m6502_nmi(m6502_t* cpu);
void m6502_irq(m6502_t* cpu);
void m6502_step(m6502_t* cpu);
void m6502_print(m6502_t* cpu);
bool m6502_interrupt_enabled(m6502_t* cpu);

#endif