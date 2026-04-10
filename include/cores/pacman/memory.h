#ifndef __PACMAN_MEMORY_H__
#define __PACMAN_MEMORY_H__

#include "types.h"

u8   pacman_read_memory(void* ctx, u16 addr);
void pacman_write_memory(void* ctx, u16 addr, u8 val);
u8   pacman_read_io(void* ctx, u16 addr);
void pacman_write_io(void* ctx, u16 addr, u8 val);

#endif
