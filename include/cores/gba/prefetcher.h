#ifndef __PREFETCHER_H__
#define __PREFETCHER_H__

#include "types.h"

typedef struct arm7tdmi_t arm7tdmi_t;
typedef struct gba_gamepak_t gba_gamepak_t;

typedef struct prefetcher_t {
    bool enabled;
    int size;
    u32 address;
    u32 cycle_counter;
} prefetcher_t;

void gba_prefetcher_add_word_delay(prefetcher_t* prefetch, gba_gamepak_t* gamepak, arm7tdmi_t* cpu, u32 addr, bool seq);
void gba_prefetcher_add_halfword_delay(prefetcher_t* prefetch, gba_gamepak_t* gamepak, arm7tdmi_t* cpu, u32 addr, bool seq);

#endif