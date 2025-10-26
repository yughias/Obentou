#ifndef _Z80_H_
#define _Z80_H_

#include "types.h"

typedef struct z80_t z80_t;
typedef u8 (*readFunc)(void*, u16);
typedef void (*writeFunc)(void*, u16, u8);

#define Z80_REG(a, b) \
union { \
    u16 a ## b; \
    struct { \
        u8 b; \
        u8 a; \
    }; \
};

#define Z80_XY_REG(name) \
union { \
    u16 name; \
    struct { \
        u8 name ## L; \
        u8 name ## H; \
    }; \
};

typedef struct z80_t {
    // interrupt vars
    bool     HALTED;
    bool     IFF1, IFF2;
    bool     INTERRUPT_DELAY;
    bool     INTERRUPT_PENDING;
    u8  INTERRUPT_MODE;
    u8 INTERRUPT_VECT;

    // 16 bit regs 
    Z80_REG(A, F);
    Z80_REG(B, C);
    Z80_REG(D, E);
    Z80_REG(H, L);

    Z80_XY_REG(IX);
    Z80_XY_REG(IY);

    Z80_REG(W, Z);

    u16 AF_;
    u16 BC_;
    u16 DE_;
    u16 HL_;

    u16 SP;
    u16 PC;

    u8 I;
    u8 R;

    bool Q;

    u8 aux_reg;

    readFunc readMemory;
    writeFunc writeMemory;

    readFunc readIO;
    writeFunc writeIO;

    uint64_t cycles;

    void* ctx;
} z80_t;

void z80_init(z80_t*);
void z80_print(z80_t*);
void z80_step(z80_t*);
void z80_nmi(z80_t*);

typedef struct byte_vec_t byte_vec_t;
void serialize_z80_t(z80_t*, byte_vec_t*);
u8* deserialize_z80_t(z80_t*, u8*);

#endif