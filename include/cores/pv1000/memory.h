#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "cores/pv1000/pv1000.h"
#include "types.h"

u8 pv1000_readMemory(void* ctx, u16 addr);
void pv1000_writeMemory(void* ctx, u16 addr, u8 byte);

u8 pv1000_readIO(void* ctx, u16 addr);
void pv1000_writeIO(void* ctx, u16 addr, u8 byte);

#endif