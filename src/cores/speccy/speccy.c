#include "cores/speccy/speccy.h"
#include "cores/speccy/loader.h"

#include "utils/archive.h"

#include "SDL_MAINLOOP.h"

#include "48k.rom.h"
#include "48k_autoloader.z80.h"

static void step_cpu(z80_t* cpu){
    if(!cpu->cycles)
        z80_step(cpu);
    cpu->cycles--;
}

static void run_frame(speccy_t* speccy){
    z80_t* cpu = &speccy->cpu;

    speccy_ula_update_color_flash(&speccy->flash_revert);

    for(speccy->master_clock_counter = 0; speccy->master_clock_counter < CLOCK_PER_FRAME; speccy->master_clock_counter++){
        if(cpu->PC == TRAP_TAPE_ROUTINE_ADDR && !cpu->cycles && speccy->tape.format == TAP_INSTANT_LOAD){
            speccy_tape_trap_routine(speccy);
        }

        step_cpu(cpu);
        if (speccy->half_divider)
            ay_step(&speccy->ay);
        speccy->half_divider ^= 1;
        speccy_send_audio(speccy);
        speccy_tape_step(speccy);
        
        ula_render(speccy->master_clock_counter, speccy->flash_revert, speccy->ula, speccy->ram);
    }

    if (z80_is_interrupt_enabled(cpu))
        z80_irq(cpu);
}

void* SPECCY_init(const archive_t* rom_archive, const archive_t* bios_archive){
    speccy_t* speccy = calloc(1, sizeof(speccy_t));
    z80_init(&speccy->cpu);
    speccy->cpu.readMemory = speccy_read_mem;
    speccy->cpu.writeMemory = speccy_write_mem;
    speccy->cpu.readIO = speccy_read_io;
    speccy->cpu.writeIO = speccy_write_io;
    speccy->cpu.ctx = speccy;
    memcpy(speccy->rom, assets_48k_rom, sizeof(assets_48k_rom));

    file_t* rom = archive_get_file_by_ext(rom_archive, "z80");
    if (rom)
        speccy_load_z80_state(speccy, rom->data, rom->size);

    rom = archive_get_file_by_ext(rom_archive, "scr");
    if (rom)
        speccy_load_scr(speccy, rom->data, rom->size);

    rom = archive_get_file_by_ext(rom_archive, "tap");
    if (rom) {
        tape_t* tape = &speccy->tape;
        tape->tape_buf = rom->data;
        tape->tape_size = rom->size;
        tape->format = TAP_INSTANT_LOAD;
        speccy_load_z80_state(speccy, assets_48k_autoloader_z80, sizeof(assets_48k_autoloader_z80));
    }

    file_t* base_rom = archive_get_file_by_ext(rom_archive, "rom");
    if (!base_rom)
        base_rom = archive_get_file_by_ext(bios_archive, "rom");
    if (base_rom)
        memcpy(speccy->rom, base_rom->data, sizeof(speccy->rom));

    ay_reset(&speccy->ay);

    return speccy;
}

void SPECCY_run_frame(speccy_t* speccy){
    run_frame(speccy);
    renderPixels();
}

bool SPECCY_detect(const archive_t* rom_archive, const archive_t* bios_archive){
    bool out = false;
    out |= archive_get_file_by_ext(rom_archive, "z80") != NULL;
    out |= archive_get_file_by_ext(rom_archive, "scr") != NULL;
    out |= archive_get_file_by_ext(rom_archive, "tap") != NULL;
    out |= archive_get_file_by_ext(rom_archive, "rom") != NULL;
    out |= archive_is_empty(rom_archive) && archive_get_file_by_ext(bios_archive, "rom") != NULL;
    return out;
}

byte_vec_t SPECCY_savestate(speccy_t* speccy){
    byte_vec_t state;
    byte_vec_init(&state);
    serialize_speccy_t(speccy, &state);
    byte_vec_shrink(&state);
    return state;
}

bool SPECCY_loadstate(speccy_t* speccy, byte_vec_t* state){
    const u8* end = state->data + state->size;
    return deserialize_speccy_t(speccy, state->data, state->data + state->size) == end;
}