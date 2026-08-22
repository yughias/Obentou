#include "cores/gba/gba.h"
#include "cores/gba/gamepak.h"
#include "utils/archive.h"

#include "normatt_bios.bin.h"

#include <SDL3/SDL.h>

#include <string.h>
#include <stdlib.h>

void gba_check_interrupts(gba_t* gba){
    if(!(gba->IME & 1) || gba->cpu.irq_disable)
        return;
    bool irq_mask = gba->IE & gba->IF & 0x3FFF;
    if(irq_mask == 0)
        return;

    arm7tdmi_trigger_exception(&gba->cpu, 0x18, 0x12);
}

void GBA_run_frame(gba_t* gba){
    u32 prev_cycles = gba->cpu.cycles;
    while(gba->cpu.cycles < CYCLES_PER_FRAME){
        u32 elapsed = 0;

        while(elapsed < gba->scheduler_head->remaining){
            if(gba->HALTCNT && (gba->HALTCNT = !(gba->IF & gba->IE & 0x3FFF))){
                gba->cpu.cycles += gba->scheduler_head->remaining - elapsed;
            } else {
                arm7tdmi_step(&gba->cpu);
                gba_check_interrupts(gba);
            }
            elapsed = gba->cpu.cycles - prev_cycles;
        }

        gba_scheduler_step(gba, &gba->scheduler_head, elapsed);
        prev_cycles = gba->cpu.cycles;
    }

    gba->cpu.cycles -= CYCLES_PER_FRAME;
    gba->clock_before_scheduling -= CYCLES_PER_FRAME;
    gba->prefetcher.cycle_counter -= CYCLES_PER_FRAME;
    gba->frame_clock += CYCLES_PER_FRAME;
}

void* GBA_init(const archive_t* rom_archive, const archive_t* bios_archive){
    gba_t* gba = malloc(sizeof(gba_t));
    memset(gba, 0, sizeof(gba_t));

    file_t* bios = archive_get_file_by_ext(bios_archive, "bin");
    if (!bios)
        bios = archive_get_file_by_ext(bios_archive, "gba");

    if (bios)
        gba->bios.data = bios->data;
    else
        gba->bios.data = assets_normatt_bios_bin; 

    gba_load_gamepak(&gba->gamepak, rom_archive);

    gba_reset(gba);

    return gba;
}

void gba_reset(gba_t* gba){
    bios_t tmp_bios;
    gba_gamepak_t tmp_gamepak;

    memcpy(&tmp_bios, &gba->bios, sizeof(bios_t));
    memcpy(&tmp_gamepak, &gba->gamepak, sizeof(gba_gamepak_t));
    memset(gba, 0, sizeof(gba_t));
    memcpy(&gba->bios, &tmp_bios, sizeof(bios_t));
    memcpy(&gba->gamepak, &tmp_gamepak, sizeof(gba_gamepak_t));

    for(int i = 0; i < 2; i++){
        gba->ppu.BGP[0 + 4*i] = 0x100;
        gba->ppu.BGP[1 + 4*i] = 0;
        gba->ppu.BGP[2 + 4*i] = 0;
        gba->ppu.BGP[3 + 4*i] = 0x100;
    }

    gba->cpu.mode_bits = 0x13;
    gba->cpu.fiq_disable = true;
    gba->cpu.irq_disable = true;
    gba->cpu.readByte = gba_read_byte;
    gba->cpu.readHalfWord = gba_read_halfword;
    gba->cpu.readWord = gba_read_word;
    gba->cpu.writeByte = gba_write_byte;
    gba->cpu.writeHalfWord = gba_write_halfword;
    gba->cpu.writeWord = gba_write_word;
    gba->cpu.master = gba;

    gba->active_dma = DMA_OFF;

    gba->RCNT = 0x8000;
    gba->apu.SOUNDBIAS = 0x200;
    gba_update_waitstates(&gba->gamepak, &gba->cpu, gba->WAITCNT);

    arm7tdmi_pipeline_refill(&gba->cpu);

    gba_scheduler_t* block = gba_scheduler_occupy_block(gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE);
    block->remaining = DRAW_CYCLES;
    block->event = gba_ppu_event_start_hblank;
    gba_scheduler_add_event(&gba->scheduler_head, block);

    apu_t* apu = &gba->apu;
    apu->sound_channels_amplifier_left = 1;
    apu->sound_channels_amplifier_right = 1;
    gba_scheduler_create_add_event_0_args(&gba->scheduler_head, gba->scheduler_pool, GBA_SCHEDULER_POOL_SIZE, gba_event_push_sample, 0);
}

bool GBA_detect(const archive_t* rom_archive, const archive_t* bios_archive){
    return archive_get_file_by_ext(rom_archive, "gba");
}

void GBA_save(gba_t* gba, const char* sav_path){
    if(gba->gamepak.type != GAMEPAK_ROM_ONLY && gba->gamepak.type != NO_GAMEPAK)
        file_save(sav_path, gba->gamepak.savMemory, gba->gamepak.savMemorySize);
}

void GBA_free(gba_t* gba){
    gba_free_gamepak(&gba->gamepak);
}

byte_vec_t GBA_savestate(gba_t* gba){
    byte_vec_t state;
    byte_vec_init(&state);
    serialize_gba_t(gba, &state);
    byte_vec_shrink(&state);
    return state;
}

bool GBA_loadstate(gba_t* gba, byte_vec_t* state){
    const u8* end = state->data + state->size;
    return deserialize_gba_t(gba, state->data, state->data + state->size) == end;
}