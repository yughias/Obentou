#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#define CLOCK_PER_FRAME 69888

#include <stdio.h>

#include "cpus/z80.h"
#include "cores/speccy/ula.h"
#include "cores/speccy/audio.h"
#include "cores/speccy/memory.h"
#include "cores/speccy/io_port.h"
#include "cores/speccy/tape.h"

extern size_t master_clock_counter;
extern z80_t cpu;

void initAll();
void freeAll();

void emulateHardware();
void emulateCpu();
void sendInterrupt();

#endif