#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "types.h"

#define SB_ADDR 0xFF01
#define SC_ADDR 0xFF02

typedef struct gb_t gb_t;
typedef enum SERIAL_MODE { SLAVE = 0, MASTER } SERIAL_MODE;

typedef struct serial_t {
    u8 SB_REG;
    u8 SC_REG;
    size_t counter;
    SERIAL_MODE mode;
} serial_t;

void gb_initSerial();
void gb_updateSerial(gb_t*);
void gb_freeSerial();

#endif