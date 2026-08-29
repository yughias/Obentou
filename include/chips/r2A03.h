#ifndef __R2A03_H__
#define __R2A03_H__

#include "types.h"

#include "utils/serializer.h"

typedef struct r2A03_t r2A03_t;
typedef u8 (*r2A03_read_func)(void*, u16);
typedef void (*r2A03_write_func)(void*, u16, u8);

#define R2A03_STRUCT(X) \
    X(u16, pc, 1, 0) \
    X(u8, s, 1, 0) \
    X(u8, p, 1, 0) \
    X(u8, a, 1, 0) \
    X(u8, x, 1, 0) \
    X(u8, y, 1, 0) \
    X(r2A03_read_func, read, 0, 0) \
    X(r2A03_write_func, write, 0, 0) \
    X(u32, cycles, 1, 0) \
    X(u16, mem_addr, 1, 0) \
    X(u8, op_arg, 1, 0) \
    X(bool, in_mem, 1, 0) \
    X(void*, ctx, 0, 0)

DECLARE_SERIALIZABLE_STRUCT(r2A03, R2A03_STRUCT)

void r2A03_init(r2A03_t* cpu);
void r2A03_reset(r2A03_t* cpu);
void r2A03_nmi(r2A03_t* cpu);
void r2A03_irq(r2A03_t* cpu);
void r2A03_step(r2A03_t* cpu);
void r2A03_print(r2A03_t* cpu);
bool r2A03_interrupt_enabled(r2A03_t* cpu);

#endif