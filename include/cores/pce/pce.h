#ifndef __PCE_H__
#define __PCE_H__

#include "cores/pce/timings.h"
#include "cpus/h6280.h"
#include "cores/pce/vce.h"
#include "cores/pce/vdc.h"
#include "cores/pce/tmr.h"
#include "cores/pce/psg.h"
#include "cores/pce/controller.h"

#include "utils/serializer.h"

#include <SDL3/SDL.h>

#define PCE_STRUCT(X) \
    X(h6280_t, cpu, 1, 1) \
    X(vce_t, vce, 1, 0) \
    X(vdc_t, vdc, 1, 1) \
    X(tmr_t, timer, 1, 0) \
    X(psg_t, psg, 1, 0) \
    X(controller_t, controller, 1, 0) \
    X(u8*, rom, 0, 0) \
    X(u32, rom_size, 0, 0) \
    X(u16, sf2, 1, 0) \
    X(u8, ram, 0x2000, 1, 0) \
    X(u8, cart_ram, 0x2000, 1, 0) \
    X(u8, irq_disable, 1, 0) \
    X(int, event_viewer, CYCLES_PER_SCANLINE*TOTAL_SCANLINES, 0, 0)

DECLARE_SERIALIZABLE_STRUCT(pce, PCE_STRUCT);

u8 pce_read(void* p, u32 addr);
void pce_write(void* p, u32 addr, u8 value);
void pce_set_resolution(pce_t* p);

void pce_notify_event(pce_t* gr, u8 r, u8 g, u8 b);
void pce_notify_line(pce_t* p, int frame_line, int* line, int w);
void pce_draw_events(SDL_Window** win, pce_t* p);


#endif