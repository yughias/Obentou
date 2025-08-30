#ifndef __PV_1000_H__
#define __PV_1000_H__

#include "cpus/z80.h"
#include "cores/pv1000/vdp.h"
#include "cores/pv1000/psg.h"
#include "cores/pv1000/controller.h"

#include "types.h"

#define REFRESH_RATE 59.9227434033
#define CLOCK_RATE 3579545
#define CYCLES_PER_FRAME 59736
#define CYCLES_PER_LINE 228
#define VSYNC_CYCLE (CYCLES_PER_LINE*256)

#define SCREEN_WIDTH 224
#define SCREEN_HEIGHT 192

typedef struct pv1000_t
{
    z80_t z80;
    vdp_t vdp;
    psg_t psg;
    controller_t controller;
    u8 memory[0x10000];

    u8 status;
} pv1000_t;

void* pv1000_init(const char* filename, SDL_AudioDeviceID device_id);
void pv1000_run_frame(pv1000_t* pv1000);

#endif