#ifndef __PV_1000_H__
#define __PV_1000_H__

#include "cpus/z80.h"
#include "cores/pv1000/vdp.h"
#include "cores/pv1000/psg.h"
#include "cores/pv1000/controller.h"

#include "types.h"

#include "utils/serializer.h"

#define REFRESH_RATE 59.9227434033
#define CLOCK_RATE 3579545
#define CYCLES_PER_FRAME 59736
#define CYCLES_PER_LINE 228
#define VSYNC_CYCLE (CYCLES_PER_LINE*256)

#define SCREEN_WIDTH 224
#define SCREEN_HEIGHT 192

#define PV1000_STRUCT(X) \
    X(z80_t, z80, 1, 1) \
    X(vdp_t, vdp, 1, 0) \
    X(psg_t, psg, 1, 0) \
    X(controller_t, controller, 1, 0) \
    X(u8, memory, 0x10000, 1, 0) \
    X(u8, status, 1, 0) \

DECLARE_SERIALIZABLE_STRUCT(pv1000, PV1000_STRUCT)

#endif