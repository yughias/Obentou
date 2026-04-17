#ifndef _SPACEINVADERS_SHIFTER_H_
#define _SPACEINVADERS_SHIFTER_H_

#include "types.h"

#define SHIFT_IN 0x03
#define SHIFTAMNT 0x02
#define SHIFT_DATA 0x04

typedef struct shifter_t {
    u16 data;
    u8 amnt;
} shifter_t;

void SI_shifter_write(shifter_t* shifter, u8 byte);
u8 SI_shifter_read(shifter_t* shifter);

#endif
