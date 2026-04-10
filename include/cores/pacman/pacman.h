#ifndef __PACMAN_H__
#define __PACMAN_H__

#include "cpus/z80.h"
#include "types.h"
#include "utils/serializer.h"

#define PACMAN_HERTZ            3072000
#define PACMAN_CYCLES_PER_FRAME (PACMAN_HERTZ / 60)
#define PACMAN_AUDIO_ROM_SIZE   512

#define PACMAN_ROM_SIZE         0x4000
#define PACMAN_RAM_SIZE         0x1000

#define PACMAN_TILE_RAM_OFF    0x0000 
#define PACMAN_PAL_RAM_OFF     0x0400
#define PACMAN_SPR_RAM_OFF     0x0FF0  /* sprite regs */

#define PACMAN_VIDEO_COLS       28
#define PACMAN_VIDEO_ROWS       36

#define PACMAN_STRUCT(X) \
    X(z80_t,  z80,                    1, 1) \
    X(u8*,    ROM,                    0, 0) \
    X(u8,     RAM, PACMAN_RAM_SIZE,   1, 0) \
    X(u8,     IO,                     1, 0) \
    X(u8*,    AUX_ROM_LOW,            0, 0) \
    X(u8*,    AUX_ROM_HIGH,           0, 0) \
    X(bool,   AUX_INSTALLED,          0, 0) \
    X(bool,   AUX_ENABLED,            1, 0) \
    X(u8,     IN0,                    1, 0) \
    X(u8,     VBLANK_ENABLED,         1, 0) \
    X(u8,     SOUND_ENABLED,          1, 0) \
    X(u8,     FLIP_SCREEN,            1, 0) \
    X(u8,     P1_LAMP,                1, 0) \
    X(u8,     P2_LAMP,                1, 0) \
    X(u8,     COIN_LOCKOUT,           1, 0) \
    X(u8,     COIN_COUNTER,           1, 0) \
    X(u8,     IN1,                    1, 0) \
    X(u8,     SOUND_VOICE1,        6, 1, 0) \
    X(u8,     SOUND_VOICE2,        5, 1, 0) \
    X(u8,     SOUND_VOICE3,        5, 1, 0) \
    X(u8,     VOICE1_FREQ,         5, 1, 0) \
    X(u8,     VOICE1_VOLUME,          1, 0) \
    X(u8,     VOICE2_FREQ_VOL,     5, 1, 0) \
    X(u8,     VOICE3_FREQ_VOL,     5, 1, 0) \
    X(u8,     SPRITE_COORDS,      16, 1, 0) \
    X(u8,     DIP_SWITCH_SETTINGS,    1, 0) \
    X(u8,     WATCHDOG_RESET,         1, 0) \
    X(u8*,    colorROM,               0, 0) \
    X(u8*,    paletteROM,             0, 0) \
    X(u8*,    tileROM,                0, 0) \
    X(u8*,    spriteROM,              0, 0) \
    X(u8*,    audioROM,               0, 0) \
    X(u32,    voiceAccumulator,    3, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(pacman, PACMAN_STRUCT)

#endif
