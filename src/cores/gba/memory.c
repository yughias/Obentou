#include "cores/gba/gba.h"

#include "cores/gba/sram.h"
#include "cores/gba/flash.h"
#include "cores/gba/eeprom.h"

#include "cores/gba/memory_utils.h"

#include "types.h"

DEFINE_READ_FUNC(byte, 8, 0)
DEFINE_READ_FUNC(halfword, 16, 0b1)
DEFINE_READ_FUNC(word, 32, 0b11)

DEFINE_WRITE_FUNC(byte, 8, 0)
DEFINE_WRITE_FUNC(halfword, 16, 0b1)
DEFINE_WRITE_FUNC(word, 32, 0b11)
