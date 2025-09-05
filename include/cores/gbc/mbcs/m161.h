#ifndef __M161_H__
#define __M161_H__

#include <stdint.h>

typedef struct gb_t gb_t;

void gb_m161_registers(gb_t* gb, u16 addr, u8 byte);
u8 gb_m161_rom(gb_t* gb, u16 addr);

#endif