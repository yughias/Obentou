#ifndef __TAPE_H__
#define __TAPE_H__

#include "utils/serializer.h"
#include "utils/wav.h"
#include "utils/file.h"

// TODO FIX CRASH ON LOADSTATE

typedef enum {NO_TAPE, TAP_TAPE_LOAD} TAPE_TYPE;
typedef enum {BLOCK_START, PILOT_TONE, SYNC_TONE, DATA_LOAD, PAUSE} TAP_LOADER_STATE;

typedef struct jace_t jace_t;
void jace_tape_step(jace_t* speccy);

#define DECLARE_TAPE_STRUCT(X) \
    X(wav_t, wav, 0, 0) \
    X(file_t*, tape, 0, 0) \
    X(size_t, tape_pos, 1, 0) \
    X(TAPE_TYPE, format, 1, 0) \
    X(TAP_LOADER_STATE, loader_state, 1, 0) \
    X(u16, block_len, 1, 0) \
    X(u8, byte, 1, 0) \
    X(u8, bitIdx, 1, 0) \
    X(size_t, pulses, 1, 0) \
    X(size_t, pulse_duration, 1, 0) \
    X(size_t, clock_counter, 1, 0) \
    X(u8, block_type, 1, 0) \
    X(u8, checksum, 1, 0) \
    X(bool, polarity, 1, 0) \
    X(bool, paused, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(tape, DECLARE_TAPE_STRUCT);

#endif