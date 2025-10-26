#ifndef __M65C02_H__
#define __M65C02_H__

#include "types.h"

#include "utils/serializer.h"

/* we don't need hi acc for this system */
#define W65C02_LO_ACC

typedef struct w65c02_t w65c02_t;
typedef u8 (*w65c02_read_func)(void*, u16);
typedef void (*w65c02_write_func)(void*, u16, u8);

#define W65C02_STRUCT(X) \
    X(u16, pc, 1, 0) \
    X(u8, s, 1, 0) \
    X(u8, p, 1, 0) \
    X(u8, a, 1, 0) \
    X(u8, x, 1, 0) \
    X(u8, y, 1, 0) \
    X(bool, wait, 1, 0) \
    X(bool, stop, 1, 0) \
    X(w65c02_read_func, read, 0, 0) \
    X(w65c02_write_func, write, 0, 0) \
    X(u32, cycles, 1, 0) \
    X(u16, mem_addr, 1, 0) \
    X(u8, op_arg, 1, 0) \
    X(bool, in_mem, 1, 0) \
    X(bool, slow_op, 1, 0) \
    X(void*, ctx, 0, 0)

DECLARE_SERIALIZABLE_STRUCT(w65c02, W65C02_STRUCT);

void w65c02_init(w65c02_t* cpu);
void w65c02_reset(w65c02_t* cpu);
void w65c02_nmi(w65c02_t* cpu);
void w65c02_irq(w65c02_t* cpu);
void w65c02_step(w65c02_t* cpu);
void w65c02_print(w65c02_t* cpu);
bool w65c02_interrupt_enabled(w65c02_t* cpu);

#endif