#ifndef _I8080_H_
#define _I8080_H_

#include "types.h"

#define I8080_REG(reg, a, b) \
union { \
    u16 reg ## _16; \
    struct { \
        u8 b ## _8; \
        u8 a ## _8; \
    }; \
};

typedef u8 (*readMemPtr)(void* ctx, u16 addr);
typedef void (*writeMemPtr)(void* ctx, u16 addr, u8 byte);
typedef u8 (*readIOPtr)(void* ctx, u8 addr);
typedef void (*writeIOPtr)(void* ctx, u8 addr, u8 byte);

typedef struct i8080_t {
    bool STOPPED;
    bool INTERRUPT_ENABLED;

    I8080_REG(PSW, A, F);
    I8080_REG(B, B, C);
    I8080_REG(D, D, E);
    I8080_REG(H, H, L);

    u16 SP;
    u16 PC;

    u64 cycles;

    readMemPtr readMem;
    writeMemPtr writeMem;
    readIOPtr readIO;
    writeIOPtr writeIO;
    void* ctx;
} i8080_t;

void i8080_initCPU(i8080_t* cpu, void* ctx, readMemPtr readMem, writeMemPtr writeMem, readIOPtr readIO, writeIOPtr writeIO);
void i8080_stepCPU(i8080_t* cpu);
void i8080_generateInterrupt(i8080_t* cpu, u8 val);

typedef struct byte_vec_t byte_vec_t;
void serialize_i8080_t(i8080_t*, byte_vec_t*);
u8* deserialize_i8080_t(i8080_t*, u8*, u8*);

#endif
