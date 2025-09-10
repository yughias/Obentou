#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include "types.h"

typedef struct controller_t
{
    u8 selected_matrix;
} controller_t;

u8 pv1000_controller_read(controller_t* controller);

#endif