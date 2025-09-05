#ifndef __MBC1_H__
#define __MBC1_H__

#include <stdint.h>

typedef struct gb_t gb_t;

u8 gb_mbc1_0000_3FFF(gb_t* gb, u16 addr);
u8 gb_mbc1_4000_7FFF(gb_t* gb, u16 addr);
u8 gb_mbc1_ram_read(gb_t* gb, u16 addr);
void gb_mbc1_ram_write(gb_t* gb, u16 addr, u8 byte);

#endif