#ifndef _SPACEINVADERS_H_
#define _SPACEINVADERS_H_

#include "types.h"
#include "cpus/i8080.h"
#include "cores/spaceinvaders/memory.h"
#include "cores/spaceinvaders/shifter.h"

#include "utils/serializer.h"

#define CPU_HZ 1.9968e6
#define CYCLES_PER_FRAME  (33300)
#define CYCLES_HALF_FRAME (11650)
#define WIDTH 224
#define HEIGHT 256

#define SPACEINVADERS_STRUCT(X) \
X(i8080_t, cpu, 1, 1) \
X(shifter_t, shifter, 1, 0) \
X(u8, memory, MEMORY_SIZE, 1, 0) \
X(u8, in, 2, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(spaceinvaders, SPACEINVADERS_STRUCT);

#endif
