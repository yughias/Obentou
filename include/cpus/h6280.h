#ifndef __H6280_H__
#define __H6280_H__

#include "types.h"

#include "utils/serializer.h"

typedef struct h6280_t h6280_t;
typedef u8 (*h6280_read_func)(void*, u32);
typedef void (*h6280_write_func)(void*, u32, u8);
typedef void (*h6280_io_func)(void*, u16, u8);

#define H6280_STRUCT(X) \
    X(u16, pc, 1, 0) \
    X(u8, s, 1, 0) \
    X(u8, p, 1, 0) \
    X(u8, a, 1, 0) \
    X(u8, x, 1, 0) \
    X(u8, y, 1, 0) \
    X(u8, mpr, 8, 1, 0) \
    X(bool, wait, 1, 0) \
    X(bool, stop, 1, 0) \
    X(bool, hi_speed, 1, 0) \
    X(bool, irq_delay, 1, 0) \
    X(h6280_read_func, read, 0, 0) \
    X(h6280_write_func, write, 0, 0) \
    X(h6280_io_func, io_write, 0, 0) \
    X(u32, cycles, 1, 0) \
    X(u16, mem_addr, 1, 0) \
    X(u8, op_arg, 1, 0) \
    X(bool, in_mem, 1, 0) \
    X(bool, slow_op, 1, 0) \
    X(void*, ctx, 0, 0)

DECLARE_SERIALIZABLE_STRUCT(h6280, H6280_STRUCT)

void h6280_init(h6280_t* cpu);
void h6280_reset(h6280_t* cpu);
void h6280_nmi(h6280_t* cpu);
void h6280_irq(h6280_t* cpu, u16 vector);
void h6280_step(h6280_t* cpu);
void h6280_print(h6280_t* cpu);
void h6280_print_next_op(h6280_t* cpu);
bool h6280_interrupt_enabled(h6280_t* cpu);

#endif