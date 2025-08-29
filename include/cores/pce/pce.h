#ifndef __PCE_H__
#define __PCE_H__

#include "cores/pce/timings.h"
#include "cpus/h6280.h"
#include "cores/pce/vce.h"
#include "cores/pce/vdc.h"
#include "cores/pce/tmr.h"
#include "cores/pce/psg.h"
#include "cores/pce/controller.h"

#include <SDL2/SDL.h>

typedef struct pce_t {
    h6280_t cpu;
    vce_t vce;
    vdc_t vdc;
    tmr_t timer;
    psg_t psg;
    controller_t controller;
    u8* rom;
    u32 rom_size;
    u16 sf2;
    u8 ram[0x2000];
    u8 cart_ram[0x2000];
    u8 irq_disable;

    int event_viewer[CYCLES_PER_SCANLINE*TOTAL_SCANLINES];
} pce_t;

u8 pce_read(void* p, u32 addr);
void pce_write(void* p, u32 addr, u8 value);
void pce_set_resolution(pce_t* p);

void pce_notify_event(pce_t* gr, u8 r, u8 g, u8 b);
void pce_notify_line(pce_t* p, int frame_line, int* line, int w);
void pce_draw_events(SDL_Window** win, pce_t* p);


#endif