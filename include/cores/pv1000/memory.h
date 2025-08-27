#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "cpus/z80.h"

uint8_t pv1000_readMemory(z80_t* z80, uint16_t addr);
void pv1000_writeMemory(z80_t* z80, uint16_t addr, uint8_t byte);

uint8_t pv1000_readIO(z80_t* z80, uint16_t addr);
void pv1000_writeIO(z80_t* z80, uint16_t addr, uint8_t byte);

#endif