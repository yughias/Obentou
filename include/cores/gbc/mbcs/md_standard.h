#ifndef __MD_STANDARD_H__
#define __MD_STANDARD_H__

#include "types.h"

typedef struct gb_t gb_t;

u8 gb_megaduck_standard_mapper(gb_t* gb, u16 addr);
void gb_megaduck_standard_registers(gb_t* gb, u16 addr, u8 byte);

#endif