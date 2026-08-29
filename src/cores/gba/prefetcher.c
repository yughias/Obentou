#include "cores/gba/prefetcher.h"
#include "cores/gba/gamepak.h"
#include "chips/arm7tdmi/arm7tdmi.h"

static void prefetch_update(prefetcher_t* prefetch, u32* elapsed,u8 wt){
    while(*elapsed >= wt + 1){
        prefetch->size += prefetch->size < 16;
        prefetch->cycle_counter += (wt + 1);
        *elapsed -= (wt + 1);
    }
}

static void prefetch_reset(prefetcher_t* prefetch, u32 cycles){
    prefetch->size = 0;
    prefetch->cycle_counter = cycles;
}

void gba_prefetcher_add_halfword_delay(prefetcher_t* prefetch, gba_gamepak_t* gamepak, arm7tdmi_t* cpu, u32 addr, bool seq){
    u32 elapsed = cpu->cycles - prefetch->cycle_counter;
    u8 wt = gamepak->waitstates[0][true];

    prefetch_update(prefetch, &elapsed, wt);

    if(addr == prefetch->address){
        if(prefetch->size >= 1){
            prefetch->size -= 1;
        } else {
            cpu->cycles += wt + 1 - elapsed;
            prefetch->cycle_counter = cpu->cycles;
        }
    } else {
        if(!(addr & 0x1FFFF) && !elapsed)
            cpu->cycles += 1;
        cpu->cycles += gamepak->waitstates[0][seq];
        prefetch_reset(prefetch, cpu->cycles);
    }
    prefetch->address = addr + 2;
}

void gba_prefetcher_add_word_delay(prefetcher_t* prefetch, gba_gamepak_t* gamepak, arm7tdmi_t* cpu, u32 addr, bool seq){
    u32 elapsed = cpu->cycles - prefetch->cycle_counter;
    u8 wt = gamepak->waitstates[0][true];
    
    prefetch_update(prefetch, &elapsed, wt);

    if(addr == prefetch->address){
        if(prefetch->size >= 2){
            prefetch->size -= 2;
        } else {
            cpu->cycles += wt + 1 - elapsed;
            if(!prefetch->size)
                cpu->cycles += wt + 1;
            prefetch_reset(prefetch, cpu->cycles);
        }
    } else {
        if(!(addr & 0x1FFFF) && !elapsed)
            cpu->cycles += 1;
        cpu->cycles += gamepak->waitstates[0][seq] + 1 + gamepak->waitstates[0][true];
        prefetch_reset(prefetch, cpu->cycles);
    }
    prefetch->address = addr + 4;
}