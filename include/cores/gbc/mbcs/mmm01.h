#ifndef __MMM01_H__
#define __MMM01_H__

#include "types.h"

typedef struct gb_t gb_t;

void gb_mmm01_registers(gb_t* gb, u16 addr, u8 byte);
u8 gb_mmm01_0000_3FFF(gb_t* gb, u16 addr);
u8 gb_mmm01_4000_7FFF(gb_t* gb, u16 addr);
u8 gb_mmm01_ram_read(gb_t* gb, u16 addr);
void gb_mmm01_ram_write(gb_t* gb, u16 addr, u8 byte);

#endif