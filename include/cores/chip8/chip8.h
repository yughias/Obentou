#ifndef _CHIP8_H_
#define _CHIP8_H_

#include "types.h"
#include "utils/serializer.h"

#define CYCLES_PER_FRAME 10
#define STACKSIZE        64
#define MEMORY_SIZE      4096
#define CH8_W            64
#define CH8_H            32

#define COLOR_BG color(20, 20, 20)
#define COLOR_FG color(180, 120, 20)

#define CHIP8_STRUCT(X) \
    X(u8, memory, MEMORY_SIZE, 1, 0) \
    X(bool, display, CH8_W*CH8_H, 1, 0) \
    X(u16, PC, 1, 0) \
    X(u16, I, 1, 0) \
    X(u8, V, 16, 1, 0) \
    X(u8, ST, 1, 0) \
    X(u8, DT, 1, 0) \
    X(u16, stack, STACKSIZE, 1, 0) \
    X(size_t, stackIndex, 1, 0) \
    X(bool, keys, 16, 1, 0) \
    X(bool, preKeys, 16, 1, 0) \
    X(bool, buzzer, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(ch8, CHIP8_STRUCT);

#endif