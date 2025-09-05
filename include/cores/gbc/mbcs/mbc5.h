#ifndef __MBC5_H__
#define __MBC5_H__

#include "types.h"

typedef struct gb_t gb_t;

u8 gb_mbc5_4000_7FFF(gb_t* gb, u16 addr);
u8 gb_mbc5_ram_read(gb_t* gb, u16 addr);
void gb_mbc5_ram_write(gb_t* gb, u16 addr, u8 byte);
void gb_mbc5_registers(gb_t* gb, u16 addr, u8 byte);

#endif