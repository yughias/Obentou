#ifndef __MBC_7_H__
#define __MBC_7_H__

#include "types.h"

typedef struct gb_t gb_t;

u8 gb_mbc7_ram_read(gb_t* gb, u16 addr);
void gb_mbc7_ram_write(gb_t* gb, u16 addr, u8 byte);
void* gb_mbc7_alloc(size_t* size);

#endif