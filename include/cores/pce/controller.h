#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include "types.h"

typedef struct controller_t {
    u8 taps;
} controller_t;

void pce_controller_write(controller_t* c, u8 val);
u8 pce_controller_read(controller_t* c);

#endif