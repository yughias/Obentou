#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "cpus/z80.h"
#include "cores/tms80/sn76489.h"
#include "cores/tms80/vdp.h"

#include "types.h"

#define CYCLES_PER_LINE 228

#define NTSC_REFRESH_RATE 59.9227434033
#define NTSC_CLOCK_RATE 3579545
#define NTSC_CYCLES_PER_FRAME 59736

#define PAL_REFRESH_RATE 49.7014596255682 
#define PAL_CLOCK_RATE 3546895
#define PAL_CYCLES_PER_FRAME 71364

#define RAM_SIZE 0x10000

typedef enum TMS80_TYPE {TMS80_UNKNOWN, SG1000, SC3000, SMS, GG} TMS80_TYPE;

typedef struct tms80_t
{
    TMS80_TYPE type;

    double refresh_rate;
    size_t cycles_per_frame;
    
    z80_t z80;
    vdp_t vdp;
    sn76489_t apu;

    u8* cartridge;
    size_t cartridge_size;

    u8* bios;
    size_t bios_size;

    bool has_keyboard;
    bool force_paddle_controller;
    bool paddle_status;
    u8 keypad_reg;

    u8 ram_bank;
    u8 banks[3];

    u8 RAM[RAM_SIZE];
} tms80_t;

bool tms80_detect_ram_adapter(u8* cartridge, size_t cartridge_size);

u8 tms80_get_keypad_a(tms80_t* tms80);
u8 tms80_get_keypad_b(tms80_t* tms80);
u8 tms80_gg_get_start_button();


#endif