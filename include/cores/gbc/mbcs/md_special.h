#ifndef __MD_SPECIAL_H__
#define __MD_SPECIAL_H__

#include "types.h"

typedef struct gb_t gb_t;

u8 gb_megaduck_special_mapper(gb_t* gb, u16 addr);
u8 gb_megaduck_special_registers_read(gb_t* gb, u16 addr);
void gb_megaduck_special_registers_write(gb_t* gb, u16 addr, u8 byte);

#endif