#ifndef __AY_H__
#define __AY_H__

#include "types.h"

typedef struct ay_t {
    u8 reg[0x10];
    u8 selected;
    u16 pulse_counter[3];
    u16 noise_counter;
    u32 lfsr;
    u32 env_counter;
    u8 env_step;
    u8 volume[3];
    enum {NO_ENV, ASC_ENV, DESC_ENV} env;
    bool pulse[3];
} ay_t;

void ay_step(ay_t* ay);
void ay_reset(ay_t* ay);
u16 ay_get_sample(ay_t* ay, int volume_multiplier);
void ay_write_selected_port(ay_t* ay, u8 data);

#endif