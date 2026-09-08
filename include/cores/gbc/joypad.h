#ifndef __JOYPAD_H__
#define __JOYPAD_H__

#include "cores/gbc/gb.h"

#include "types.h"

#define JOYP_ADDR 0xFF00

u8 gb_read_joyp(gb_t* gb);
void gb_write_joyp(gb_t* gb, u8 byte);

#endif