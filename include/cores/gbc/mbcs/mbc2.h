#ifndef __MBC2_H__
#define __MBC2_H__

#include <stdint.h>

typedef struct gb_t gb_t;

u8 gb_mbc2_4000_7FFF(gb_t* gb, u16 addr);
u8 gb_mbc2_ram_read(gb_t* gb, u16 addr);
void gb_mbc2_ram_write(gb_t* gb, u16 addr, u8 byte);
void gb_mbc2_registers(gb_t* gb, u16 addr, u8 byte);

#endif