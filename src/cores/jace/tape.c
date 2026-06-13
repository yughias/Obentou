#include "cores/jace/jace.h"
#include <tinyfiledialogs.h>

#include <stdio.h>

#define DATA                  0xFF
#define HEADER                0x00
#define HEADER_PULSES         8192
#define DATA_PULSES           1024
#define PILOT_TONE_T_STATES   2011
#define SYNC_PULSE_1_T_STATES  601
#define SYNC_PULSE_2_T_STATES  791
#define BIT_0_T_STATES         800
#define BIT_1_T_STATES        1590
#define PAUSE_1_T_STATES       903
#define PAUSE_2_T_STATES      4186

static u8 tape_read_u8(tape_t* tape){
    if (tape->tape_pos < tape->tape->size)
        return tape->tape->data[tape->tape_pos++];
    return 0;
}

static u16 tape_read_u16(tape_t* tape){
    u8 lsb = tape_read_u8(tape);
    u8 msb = tape_read_u8(tape);
    return (msb << 8) | lsb;
}

static void tape_stop(tape_t* tape){
    tape->format = NO_TAPE;
}

static void send_signal(jace_t* jace, bool bit){
    jace->ear_in = jace->beeper = bit;
}

static void tape_load_emitter(jace_t* jace){
    tape_t* tape = &jace->tape;
    send_signal(jace, tape->polarity);
    tape->clock_counter++;
    if(tape->clock_counter == tape->pulse_duration){
        tape->clock_counter = 0;
        tape->pulses--;
        tape->polarity ^= 1;
    }
}

static void tape_fetch_bit(tape_t* tape) {
    bool bit = tape->byte & 0x80;
    tape->byte <<= 1;
    tape->bitIdx++;
    
    if (tape->bitIdx == 8) {
        tape->bitIdx = 0;
        tape->block_len--;
        
        if (tape->block_len != 0) {
            if (tape->block_len == 1) {
                tape->byte = tape->checksum;
            } else {
                tape->byte = tape_read_u8(tape);
                tape->checksum ^= tape->byte;
            }
        }
    }
    
    tape->clock_counter = 0;
    tape->pulses = 2;
    if (bit)
        tape->pulse_duration = BIT_1_T_STATES;
    else
        tape->pulse_duration = BIT_0_T_STATES;
}

static void wav_step(jace_t* jace){
    wav_t* wav = &jace->tape.wav;
    size_t idx = jace->tape.clock_counter * wav->sample_rate / 3.25e6;
    idx <<= wav->bytes_per_sample == 2;
    idx <<= wav->channels == 2;
    if(idx >= wav->size)
        return;
    bool sample = wav->data[idx | (wav->bytes_per_sample == 2)] > 0x80;
    jace->ear_in = jace->beeper = sample;
    jace->tape.clock_counter++;
}

static void tape_step(jace_t* jace){
    tape_t* tape = &jace->tape;
    switch(tape->format){
        case TAP_TAPE_LOAD:
        switch(tape->loader_state){
            case BLOCK_START:
            if(tape->tape_pos == tape->tape->size) {
                tape_stop(tape);
                break;
            }
            tape->block_len = tape_read_u16(tape) + 2;
            tape->byte = tape->block_type;
            printf("%d %d\n", tape->block_len, tape->block_type);
            tape->block_type ^= 0xFF;
            tape->checksum = 0;
            tape->bitIdx = 0;
            tape->clock_counter = 0;
            tape->pulse_duration = PILOT_TONE_T_STATES;
            if(tape->byte == DATA) {
                tape->pulses = DATA_PULSES;
            } if(tape->byte == HEADER) {
                tape->pulses = HEADER_PULSES;
            }
            tape->loader_state = PILOT_TONE;
            break;

            case PILOT_TONE:
            if(tape->pulses == 0){
                printf("switching to sync\n");
                tape->loader_state = SYNC_TONE;
                tape->pulses = 2;
            }
            tape_load_emitter(jace);
            break;

            case SYNC_TONE:
            if(tape->pulses == 2){
                tape->pulse_duration = SYNC_PULSE_1_T_STATES;
            }
            if(tape->pulses == 1){
                tape->pulse_duration = SYNC_PULSE_2_T_STATES;
            }
            if(tape->pulses == 0){
                printf("switching to data\n");
                tape->loader_state = DATA_LOAD;
                tape_fetch_bit(tape);
            }
            tape_load_emitter(jace);
            break;

            case DATA_LOAD:
            if(tape->block_len == 0 && tape->pulses == 0){
                printf("switching to pause\n");
                tape->loader_state = PAUSE;
                tape->pulses = 2;
                tape->clock_counter = 0;
                tape->pulse_duration = PAUSE_1_T_STATES;
            }
            if(tape->block_len != 0 && tape->pulses == 0)
                tape_fetch_bit(tape);
            tape_load_emitter(jace);
            break;

            case PAUSE:
            if (tape->pulses == 2)
                tape->pulse_duration = PAUSE_1_T_STATES;
            if (tape->pulses == 1)
                tape->pulse_duration = PAUSE_2_T_STATES;
            if(tape->pulses == 0) {
                printf("switching to block start\n");
                tape->loader_state = BLOCK_START;
            }
            tape_load_emitter(jace);
            break;
        }
        break;

        default:
        break;
    }
}


void jace_tape_step(jace_t* jace){
    if (jace->tape.paused)
        return;
    if (jace->tape.wav.data)
        wav_step(jace);
    else
        tape_step(jace);
}