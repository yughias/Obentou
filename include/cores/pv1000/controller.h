#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include <stdint.h>

typedef struct controller_t
{
    uint8_t selected_matrix;
} controller_t;


uint8_t pv1000_controller_read(controller_t* controller);

#endif