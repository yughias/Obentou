#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "types.h"

u8 speccy_read_mem(void*, u16);
void speccy_write_mem(void*, u16, u8);

u8 speccy_read_io(void*, u16);
void speccy_write_io(void*, u16, u8);

#endif