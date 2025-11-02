#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "cpus/z80.h"
#include "cores/tms80/sn76489.h"
#include "cores/tms80/vdp.h"

#include "types.h"

#include "utils/serializer.h"

#define CYCLES_PER_LINE 228

#define NTSC_REFRESH_RATE 59.9227434033
#define NTSC_CLOCK_RATE 3579545
#define NTSC_CYCLES_PER_FRAME 59736

#define PAL_REFRESH_RATE 49.7014596255682 
#define PAL_CLOCK_RATE 3546895
#define PAL_CYCLES_PER_FRAME 71364

#define RAM_SIZE 0x10000

typedef enum TMS80_TYPE {TMS80_UNKNOWN, SG1000, SC3000, SMS, GG} TMS80_TYPE;

#define TMS80_STRUCT(X) \
    X(TMS80_TYPE, type, 0, 0) \
    X(double, refresh_rate, 0, 0) \
    X(size_t, cycles_per_frame, 0, 0) \
    X(z80_t, z80, 1, 1) \
    X(vdp_t, vdp, 1, 0) \
    X(sn76489_t, apu, 1, 0) \
    X(bool, no_cartridge, 0, 0) \
    X(u8*, cartridge, 0, 0) \
    X(size_t, cartridge_size, 0, 0) \
    X(u8*, bios, 0, 0) \
    X(size_t, bios_size, 0, 0) \
    X(bool, bios_masked, 1, 0) \
    X(bool, has_keyboard, 0, 0) \
    X(bool, force_paddle_controller, 0, 0) \
    X(bool, paddle_status, 0, 0) \
    X(u8, keypad_reg, 1, 0) \
    X(u8, ram_bank, 1, 0) \
    X(u8, banks, 3, 1, 0) \
    X(u8, RAM, RAM_SIZE, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(tms80, TMS80_STRUCT);

bool tms80_detect_ram_adapter(u8* cartridge, size_t cartridge_size);

u8 tms80_get_keypad_a(tms80_t* tms80);
u8 tms80_get_keypad_b(tms80_t* tms80);
u8 tms80_gg_get_start_button();


#endif