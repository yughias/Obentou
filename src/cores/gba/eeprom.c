#include "cores/gba/eeprom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void eeprom_reset_state(eeprom_t* eeprom, EEPROM_STATE new_state){
    eeprom->bit_counter = 0;
    eeprom->state = new_state;
}

void gba_eeprom_setup_memory(gba_gamepak_t* gamepak, size_t size){
    gamepak->type = GAMEPAK_EEPROM;
    gamepak->savMemorySize = size;
    gamepak->savMemory = malloc(size);
    gamepak->internalData = malloc(sizeof(eeprom_t));
    memset(gamepak->internalData, 0, sizeof(eeprom_t));
    memset(gamepak->savMemory, 0xFF, gamepak->savMemorySize);
    ((eeprom_t*)gamepak->internalData)->n = (size == EEPROM_512B_SIZE) ? 6 : 14; 
    gamepak->savSizeMask = size - 1;
}

u8 gba_eeprom_read(gba_gamepak_t* gamepak){
    eeprom_t* eeprom = gamepak->internalData;
    bool bit;
    switch(eeprom->state){
        case EEPROM_READ_DUMMY:
        eeprom->bit_counter += 1;
        if(eeprom->bit_counter == 4){
            eeprom_reset_state(eeprom, EEPROM_READ_DATA);
        }
        return 1;

        case EEPROM_READ_DATA:
        bit = eeprom->buffer >> 63;
        eeprom->buffer <<= 1;
        eeprom->bit_counter += 1;
        if(eeprom->bit_counter == 64){
            eeprom_reset_state(eeprom, EEPROM_IDLE);
        }
        return bit;

        default:
        return 1;
    }
}

void gba_eeprom_write(gba_gamepak_t* gamepak, u8 byte){
    eeprom_t* eeprom = gamepak->internalData;
    bool bit = byte & 1;
    eeprom->buffer <<= 1;
    eeprom->buffer |= bit;
    eeprom->bit_counter += 1;
    switch(eeprom->state){
        case EEPROM_IDLE:
        if(eeprom->bit_counter == 2){
            u8 mode = eeprom->buffer & 0b11;
            if(mode == 0b11)
                eeprom->state = EEPROM_SET_ADDRESS;
            if(mode == 0b10){
                eeprom->state = EEPROM_WRITE_REQUEST;
            }
            eeprom->bit_counter = 0;
        }
        return;

        case EEPROM_SET_ADDRESS:
        if(eeprom->bit_counter == eeprom->n + 1){
            eeprom->buffer >>= 1;
            eeprom->address = eeprom->buffer & ((1 << eeprom->n) - 1);
            // in 8kb eeprom first 4 bit should always be 0
            eeprom->address &= (1 << 10) - 1;
            eeprom->buffer = ((uint64_t*)gamepak->savMemory)[eeprom->address];
            eeprom_reset_state(eeprom, EEPROM_READ_DUMMY);

        }
        return;

        case EEPROM_WRITE_REQUEST:
        if(eeprom->bit_counter == eeprom->n){
            eeprom->address = eeprom->buffer & ((1 << eeprom->n) - 1);
            // in 8kb eeprom first 4 bit should always be 0
            eeprom->address &= (1 << 10) - 1;
            eeprom_reset_state(eeprom, EEPROM_WRITE_DATA);
        }
        return;

        case EEPROM_WRITE_DATA:
        if(eeprom->bit_counter == 64){
            ((uint64_t*)gamepak->savMemory)[eeprom->address] = eeprom->buffer;
            eeprom_reset_state(eeprom, EEPROM_WRITE_END);
        }
        return;

        case EEPROM_WRITE_END:
        eeprom_reset_state(eeprom, EEPROM_IDLE);
        return;

        default:
        return;
    }
}