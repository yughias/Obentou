#ifndef __TAPE_H__
#define __TAPE_H__

#include "utils/serializer.h"

#define TRAP_TAPE_ROUTINE_ADDR 0x56B

typedef enum {NO_TAPE, TAP_TAPE_LOAD, TAP_INSTANT_LOAD} TAPE_TYPE;
typedef enum {BLOCK_START, PILOT_TONE, SYNC_TONE, DATA_LOAD, PAUSE} TAP_LOADER_STATE;

typedef struct speccy_t speccy_t;

void speccy_tape_trap_routine(speccy_t* speccy);
void speccy_tape_step(speccy_t* speccy);

#define DECLARE_TAPE_STRUCT(X) \
    X(u8*, tape_buf, 0, 0) \
    X(size_t, tape_size, 0, 0) \
    X(size_t, tape_pos, 1, 0) \
    X(TAPE_TYPE, format, 1, 0) \
    X(TAP_LOADER_STATE, loader_state, 1, 0) \
    X(uint16_t, block_len, 1, 0) \
    X(uint8_t, byte, 1, 0) \
    X(uint8_t, bitIdx, 1, 0) \
    X(size_t, pulses, 1, 0) \
    X(size_t, pulse_duration, 1, 0) \
    X(size_t, clock_counter, 1, 0) \
    X(bool, polarity, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(tape, DECLARE_TAPE_STRUCT);

#endif