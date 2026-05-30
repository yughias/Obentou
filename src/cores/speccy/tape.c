#include "cores/speccy/speccy.h"
#include <tinyfiledialogs.h>

#include <stdio.h>

#define DATA                  0xFF
#define HEADER                0x00
#define HEADER_PULSES         8063
#define DATA_PULSES           3223
#define PILOT_TONE_T_STATES   2168
#define SYNC_PULSE_1_T_STATES  667
#define SYNC_PULSE_2_T_STATES  735
#define BIT_0_T_STATES         855
#define BIT_1_T_STATES        1710
#define PAUSE_DURATION        1000

static u8 tape_read_u8(tape_t* tape){
    if (tape->tape_pos < tape->tape_size)
        return tape->tape_buf[tape->tape_pos++];
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

static void send_signal(speccy_t* speccy, bool bit){
    speccy->ula &= ~(0b10000);
    speccy->ula |= bit << 4;
}

static void tape_load_emitter(speccy_t* speccy){
    tape_t* tape = &speccy->tape;
    send_signal(speccy, tape->polarity);
    tape->clock_counter++;
    if(tape->clock_counter == tape->pulse_duration){
        tape->clock_counter = 0;
        tape->pulses--;
        tape->polarity ^= 1;
    }
}

static void tape_fetch_bit(tape_t* tape){
    bool bit = tape->byte & 0x80;
    tape->byte <<= 1;
    tape->bitIdx++;
    if(tape->bitIdx == 8){
        tape->bitIdx = 0;
        tape->block_len--;
        if(tape->block_len != 0)
            tape->byte = tape_read_u8(tape);
    }
    tape->clock_counter = 0;
    tape->pulses = 2;
    if(bit)
        tape->pulse_duration = BIT_1_T_STATES;
    else
        tape->pulse_duration = BIT_0_T_STATES;
}

void speccy_tape_trap_routine(speccy_t* speccy){
    z80_t* cpu = &speccy->cpu;
    tape_t* tape = &speccy->tape;
    uint8_t blockType = cpu->AF_ >> 8;
    bool isLoad = cpu->AF_ & 1;
    uint16_t address = cpu->IX;
    //uint16_t length = cpu->DE;

    tape->block_len = tape_read_u16(tape);
    u8 tap_block_type = tape_read_u8(tape);

    if(tap_block_type != blockType){
        tape_stop(tape);
        printf("different block type in instant loading!\n");
        return;
    }

    if(isLoad){
        uint8_t checksum = blockType;
        for(size_t n_bytes = 0; n_bytes < tape->block_len - 2; n_bytes++){
            tape->byte = tape_read_u8(tape);
            cpu->writeMemory(speccy, address, tape->byte);
            address += 1;
            checksum ^= tape->byte;
        }
        tape->byte = tape_read_u8(tape);
        if(checksum != tape->byte){
            printf("different checksum in instant loading!\n");
            return;
        }
    }

    if(tape->tape_pos == tape->tape_size){
        tape_stop(tape);
    }

    cpu->AF |= 1;
    cpu->PC = 0x05E2;
}

void speccy_tape_step(speccy_t* speccy){
    tape_t* tape = &speccy->tape;
    switch(tape->format){
        case TAP_TAPE_LOAD:
        switch(tape->loader_state){
            case BLOCK_START:
            if(tape->tape_pos == tape->tape_size)
                tape_stop(tape);
            tape->block_len = tape_read_u16(tape);
            tape->byte = tape_read_u8(tape);
            tape->bitIdx = 0;
            tape->clock_counter = 0;
            tape->pulse_duration = PILOT_TONE_T_STATES;
            tape->polarity = true;
            if(tape->byte == DATA)
                tape->pulses = DATA_PULSES;
            if(tape->byte == HEADER)
                tape->pulses = HEADER_PULSES;
            tape->loader_state = PILOT_TONE;
            break;

            case PILOT_TONE:
            if(tape->pulses == 0){
                tape->loader_state = SYNC_TONE;
                tape->pulses = 2;
            }
            tape_load_emitter(speccy);
            break;

            case SYNC_TONE:
            if(tape->pulses == 2){
                tape->pulse_duration = SYNC_PULSE_1_T_STATES;
            }
            if(tape->pulses == 1){
                tape->pulse_duration = SYNC_PULSE_2_T_STATES;
            }
            if(tape->pulses == 0){
                tape->loader_state = DATA_LOAD;
                tape_fetch_bit(tape);
            }
            tape_load_emitter(speccy);
            break;

            case DATA_LOAD:
            if(tape->block_len == 0 && tape->pulses == 0){
                tape->loader_state = PAUSE;
                tape->pulses = 1;
                tape->clock_counter = 0;
                tape->pulse_duration = PAUSE_DURATION;
            }
            if(tape->block_len != 0 && tape->pulses == 0)
                tape_fetch_bit(tape);
            tape_load_emitter(speccy);
            break;

            case PAUSE:
            if(tape->pulses == 0)
                tape->loader_state = BLOCK_START;
            tape_load_emitter(speccy);
            break;
        }
        break;

        default:
        break;
    }
}