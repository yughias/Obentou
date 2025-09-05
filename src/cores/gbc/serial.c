#include "cores/gbc/serial.h"
#include "cores/gbc/gb.h"

#include "SDL_MAINLOOP.h"

#include <stdint.h>
#include <string.h>

void gb_initSerial(){
}

void gb_freeSerial(){
}

void gb_updateSerial(gb_t* gb){
    sm83_t* cpu = &gb->cpu;
    serial_t* serial = &gb->serial;
    if(serial->counter--)
        return;

    serial->counter = 4096;
    if((serial->SC_REG & 0x80) && (serial->SC_REG & 0x01)){
        serial->SB_REG = 0xFF;
        serial->SC_REG &= 0x7F;
        cpu->IF |= SERIAL_IRQ;
    }  
}