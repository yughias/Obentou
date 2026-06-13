#ifndef __JACE_H__
#define __JACE_H__

#include "types.h"

#include "utils/serializer.h"

#include "cores/jace/tape.h"
#include "cpus/z80.h"

#define MAX_RAM_SIZE ((1 << 15) | (1 << 14))

#define JACE_STRUCT(X) \
    X(z80_t, cpu, 1, 1) \
    X(u8*, rom, 0, 0) \
    X(u8, vram, 0x400, 1, 0) \
    X(u8, cram, 0x400, 1, 0) \
    X(u8, ram, 0x400, 1, 0) \
    X(u8, expansion_ram, 0x8000, 1, 0) \
    X(bool, beeper, 1, 0) \
    X(u8, ear_in, 1, 0) \
    X(tape_t, tape, 1, 1)

DECLARE_SERIALIZABLE_STRUCT(jace, JACE_STRUCT);

#endif