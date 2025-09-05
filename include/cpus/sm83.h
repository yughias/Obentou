#ifndef _SM83_H_
#define _SM83_H_

#include "types.h"

#define IF_ADDR 0xFF0F
#define IE_ADDR 0xFFFF

#define VBLANK_IRQ                0b00000001
#define STAT_IRQ                  0b00000010
#define TIMER_IRQ                 0b00000100
#define SERIAL_IRQ                0b00001000
#define JOYPAD_IRQ                0b00010000

typedef u8 (*readFunc)(void*, u16);
typedef void (*writeFunc)(void*, u16, u8);
typedef void (*tickFunc)(void*, int);

typedef struct sm83_t {
    // interrupt vars
    bool    HALTED;
    bool    IME;
    bool    EI_DELAY;
    bool    HALT_BUG; 
    u8 IE;
    u8 IF;

    // registers
    union {
        u16 AF;
        struct {
            union {
                u8 F;
                struct {
                    u8 UNUSED_FLAG : 4;
                    bool C_FLAG : 1;
                    bool H_FLAG : 1;
                    bool N_FLAG : 1;
                    bool Z_FLAG : 1;
                };
            };
            u8 A;
        };
    };

    union {
        u16 BC;
        struct {
            u8 C;
            u8 B;
        };
    };

    union {
        u16 DE;
        struct {
            u8 E;
            u8 D;
        };
    };

    union {
        u16 HL;
        struct {
            u8 L;
            u8 H;
        };
    };

    u16 SP;
    u16 PC;

    // busFunc
    readFunc readByte;
    writeFunc writeByte;

    // tickFunc: used to update system and cpu cycles
    tickFunc tickSystem;

    uint64_t cycles;

    void* ctx;
} sm83_t;

void initCPU(sm83_t*);
void infoCPU(sm83_t*);
void stepCPU(sm83_t*);

#endif