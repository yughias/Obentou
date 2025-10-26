#ifndef _BYTEPUSHER_H_
#define _BYTEPUSHER_H_

#include "types.h"

#include "utils/serializer.h"

#define HERTZ_PER_FRAME 65536
#define MEMORY_SIZE     0x1000008
#define BYTEPUSHER_W    256
#define BYTEPUSHER_H    256

#define BYTEPUSHER_STRUCT(X) \
    X(u8, memory, MEMORY_SIZE, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(bytepusher, BYTEPUSHER_STRUCT);

#endif