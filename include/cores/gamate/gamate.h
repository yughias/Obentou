#ifndef __GAMATE_H__
#define __GAMATE_H__

#include "chips/m6502.h"
#include "chips/ay.h"
#include "cores/gamate/lcd.h"

#include "utils/serializer.h"
#include "types.h"

#define GAMATE_STRUCT(X) \
    X(m6502_t, cpu, 1, 1) \
    X(u8, clock_divider, 1, 0) \
    X(ay_t, ay, 1, 0) \
    X(u8*, rom, 0, 0) \
    X(u8*, bios, 0, 0) \
    X(u8, ram, 0x400, 1, 0) \
    X(lcd_t, lcd, 1, 1) \
    X(u8, copyright_read, 1, 0) \
    X(size_t, rom_size, 0, 0) \
    X(bool, is_4in1, 0, 0) \
    X(u8, first_bank, 1, 0) \
    X(u8, second_bank, 1, 0) \

DECLARE_SERIALIZABLE_STRUCT(gamate, GAMATE_STRUCT);

#endif