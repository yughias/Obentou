#ifndef _SPACEINVADERS_MEMORY_H_
#define _SPACEINVADERS_MEMORY_H_

#include "types.h"
#include "utils/archive.h"

#define MEMORY_SIZE 0x4000
#define ROM_SIZE 0x2000
#define MIRROR_MASK 0x3FFF

u8 SI_read_memory(void* ctx, u16 addr);
void SI_write_memory(void* ctx, u16 addr, u8 byte);
u8 SI_read_IO(void* ctx, u8 addr);
void SI_write_IO(void* ctx, u8 addr, u8 byte);

#endif
