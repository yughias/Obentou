#ifndef __PACMAN_H__
#define __PACMAN_H__

#include "cpus/z80.h"
#include "types.h"
#include "utils/serializer.h"
#include "utils/controls.h"

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

typedef struct maketrax_regs_t {
    u8 disable_protection;
    u8 offset;
    u8 counter;   
} maketrax_regs_t;

typedef struct jrpacman_regs_t {
    bool bgpriority;
    bool tilebank;
    bool spritebank;
    bool palettebank;
    bool colorbank;
    u8 scroll;
} jrpacman_regs_t;

typedef enum PACMAN_TYPE {
    PACMAN_TYPE_NORMAL,
    PACMAN_TYPE_MSPACMAN,
    PACMAN_TYPE_MAKETRAX,
    PACMAN_TYPE_JRPACMAN,
} PACMAN_TYPE;

#define PACMAN_STRUCT(X) \
    X(z80_t,            z80,                  1, 1) \
    X(u8*,              ROM,                  0, 0) \
    X(u8,               RAM, PACMAN_RAM_SIZE, 1, 0) \
    X(u8,               IO,                   1, 0) \
    X(u8*,              AUX_ROM,              0, 0) \
    X(u8*,              ROM_HIGH,             0, 0) \
    X(bool,             AUX_ENABLED,          1, 0) \
    X(u8,               IN0,                  1, 0) \
    X(u8,               VBLANK_ENABLED,       1, 0) \
    X(u8,               SOUND_ENABLED,        1, 0) \
    X(u8,               FLIP_SCREEN,          1, 0) \
    X(u8,               P1_LAMP,              1, 0) \
    X(u8,               P2_LAMP,              1, 0) \
    X(u8,               COIN_LOCKOUT,         1, 0) \
    X(u8,               COIN_COUNTER,         1, 0) \
    X(u8,               IN1,                  1, 0) \
    X(u8,               SOUND_VOICE1,         6, 1, 0) \
    X(u8,               SOUND_VOICE2,         5, 1, 0) \
    X(u8,               SOUND_VOICE3,         5, 1, 0) \
    X(u8,               VOICE1_FREQ,          5, 1, 0) \
    X(u8,               VOICE1_VOLUME,        1, 0) \
    X(u8,               VOICE2_FREQ_VOL,      5, 1, 0) \
    X(u8,               VOICE3_FREQ_VOL,      5, 1, 0) \
    X(u8,               SPRITE_COORDS,       16, 1, 0) \
    X(u8,               DIP_SWITCH_SETTINGS,  1, 0) \
    X(u8,               WATCHDOG_RESET,       1, 0) \
    X(u8*,              colorROM,             0, 0) \
    X(u8*,              paletteROM,           0, 0) \
    X(u8*,              tileROM,              0, 0) \
    X(u8*,              spriteROM,            0, 0) \
    X(u8*,              audioROM,             0, 0) \
    X(u32,              voiceAccumulator,     3, 1, 0) \
    X(bool,             is_270_degree,        0, 0) \
    X(PACMAN_TYPE,      type,                 0, 0) \
    X(maketrax_regs_t,  maketrax,             1, 0) \
    X(jrpacman_regs_t,  jrpacman,             1, 0) \
    X(bool,             bg_priority_map,      PACMAN_VIDEO_COLS * PACMAN_VIDEO_ROWS * 8 * 8, 0, 0) \
    X(const control_t*, input_map,            0, 0)



DECLARE_SERIALIZABLE_STRUCT(pacman, PACMAN_STRUCT)

#endif
