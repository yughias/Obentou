#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#define CLOCK_PER_FRAME 69888

#include "utils/serializer.h"

#include "cpus/z80.h"
#include "cores/speccy/ula.h"
#include "cores/speccy/audio.h"
#include "cores/speccy/memory.h"
#include "cores/speccy/input.h"
#include "cores/speccy/tape.h"

#define SPECCY_STRUCT(X) \
    X(z80_t, cpu, 1, 1) \
    X(u8, rom, (1 << 14), 0, 0) \
    X(u8, ram, (1 << 16) - (1 << 14), 1, 0) \
    X(bool, half_divider, 1, 0) \
    X(ay_t, ay, 1, 0) \
    X(bool, flash_revert, 1, 0) \
    X(u8, ula, 1, 0) \
    X(tape_t, tape, 1, 1) \
    X(u32, master_clock_counter, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(speccy, SPECCY_STRUCT);

#endif