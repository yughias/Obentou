#ifndef _Z80_H_
#define _Z80_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct z80_t z80_t;
typedef uint8_t (*readFunc)(z80_t*, uint16_t);
typedef void (*writeFunc)(z80_t*, uint16_t, uint8_t);

#define Z80_REG(a, b) \
union { \
    uint16_t a ## b; \
    struct { \
        uint8_t b; \
        uint8_t a; \
    }; \
};

#define Z80_XY_REG(name) \
union { \
    uint16_t name; \
    struct { \
        uint8_t name ## L; \
        uint8_t name ## H; \
    }; \
};

typedef struct z80_t {
    // interrupt vars
    bool     HALTED;
    bool     IFF1, IFF2;
    bool     INTERRUPT_DELAY;
    bool     INTERRUPT_PENDING;
    uint8_t  INTERRUPT_MODE;
    uint8_t INTERRUPT_VECT;

    // 16 bit regs 
    Z80_REG(A, F);
    Z80_REG(B, C);
    Z80_REG(D, E);
    Z80_REG(H, L);

    Z80_XY_REG(IX);
    Z80_XY_REG(IY);

    Z80_REG(W, Z);

    uint16_t AF_;
    uint16_t BC_;
    uint16_t DE_;
    uint16_t HL_;

    uint16_t SP;
    uint16_t PC;

    uint8_t I;
    uint8_t R;

    bool Q;

    uint8_t aux_reg;

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

#endif