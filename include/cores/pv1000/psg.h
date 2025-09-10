#ifndef __PSG_H__
#define __PSG_H__

#include "SDL2/SDL.h"

#include <stdbool.h>

#define ASIC_FREQ 17.897727e6
#define CALCULATE_SQUARE_FREQ(x) (ASIC_FREQ / 1024) / (63 - x)
#define HOST_GAIN 16

typedef struct psg_t {
    float time_elapsed;
    float updateRate;
    float square_freq[3];

    bool enabled;
    bool mixer;
} psg_t;

void PV1000_psg_callback(void *userdata, Uint8 * stream, int len);

#endif