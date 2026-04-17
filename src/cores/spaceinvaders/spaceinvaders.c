#include "cores/spaceinvaders/spaceinvaders.h"
#include "cores/spaceinvaders/memory.h"
#include "cores/spaceinvaders/shifter.h"

#include "utils/controls.h"
#include "SDL_MAINLOOP.h"

#include <stdlib.h>
#include <string.h>

static void updateJoystick(spaceinvaders_t* si){
    si->in[0] = 0x00;
    si->in[1] = 0x00;

    si->in[0] |= controls_pressed(CONTROL_SPACEINVADERS_COIN,   0) << 0;
    si->in[0] |= controls_pressed(CONTROL_SPACEINVADERS_START,  1) << 1;
    si->in[0] |= controls_pressed(CONTROL_SPACEINVADERS_START,  0) << 2;
    si->in[1] |=                                               (1) << 3;
    si->in[0] |= controls_pressed(CONTROL_SPACEINVADERS_FIRE,   0) << 4;
    si->in[0] |= controls_pressed(CONTROL_SPACEINVADERS_LEFT,   0) << 5;
    si->in[0] |= controls_pressed(CONTROL_SPACEINVADERS_RIGHT,  0) << 6;

    si->in[1] |= controls_pressed(CONTROL_SPACEINVADERS_FIRE,  1) << 4;
    si->in[1] |= controls_pressed(CONTROL_SPACEINVADERS_LEFT,  1) << 5;
    si->in[1] |= controls_pressed(CONTROL_SPACEINVADERS_RIGHT, 1) << 6;
}

static void displayScreen(spaceinvaders_t* si){
    // cabinet screen is rotated
    for(int y = 0; y < WIDTH; y++){
        for(int byte = 0; byte < 32; byte++){
            for(int i = 0; i < 8; i++){
                int offset = y + (HEIGHT - 1 - (byte*8+i)) * WIDTH;
                u8 val = si->memory[0x2400 + byte + y * 32] & (1 << i);
                val = val ? 255 : 0;
                pixels[offset] = color(val, val, val);
            }
        }
    }
}

bool SPACEINVADERS_detect(const archive_t* rom_archive, const archive_t* bios_archive){
    (void)bios_archive;
    return archive_get_file_by_name(rom_archive, "invaders.h")
        && archive_get_file_by_name(rom_archive, "invaders.g")
        && archive_get_file_by_name(rom_archive, "invaders.f")
        && archive_get_file_by_name(rom_archive, "invaders.e");
}

void* SPACEINVADERS_init(const archive_t* rom_archive, const archive_t* bios_archive){
    spaceinvaders_t* si = calloc(1, sizeof(spaceinvaders_t));
    
    i8080_initCPU(&si->cpu, si, SI_read_memory, SI_write_memory, SI_read_IO, SI_write_IO);

    static const char* filenames[4] = {
        "invaders.h",
        "invaders.g",
        "invaders.f",
        "invaders.e"
    };

    u16 offset = 0x0000;
    u16 bank_size = 0x800;

    for(int i = 0; i < 4; i++){
        file_t* f = archive_get_file_by_name(rom_archive, filenames[i]);
        if(!f || f->size < bank_size)
            return NULL;
        memcpy(si->memory + offset, f->data, bank_size);
        offset += bank_size;
    }

    return si;
}

void SPACEINVADERS_run_frame(void* ctx){
    spaceinvaders_t* si = ctx;
    updateJoystick(si);

    bool midInterrupt = false;
    while(si->cpu.cycles < CYCLES_PER_FRAME){
        i8080_stepCPU(&si->cpu);
        if(midInterrupt == false && si->cpu.cycles >= CYCLES_HALF_FRAME){
            i8080_generateInterrupt(&si->cpu, 0x08);
            midInterrupt = true;
        }
    }
    i8080_generateInterrupt(&si->cpu, 0x10);

    si->cpu.cycles -= CYCLES_PER_FRAME;

    displayScreen(si);
    renderPixels();
}

byte_vec_t SPACEINVADERS_savestate(spaceinvaders_t* si){
    byte_vec_t state;
    byte_vec_init(&state);
    serialize_spaceinvaders_t(si, &state);
    byte_vec_shrink(&state);
    return state;
}

bool SPACEINVADERS_loadstate(spaceinvaders_t* si, byte_vec_t* state){
    const u8* end = state->data + state->size;
    return deserialize_spaceinvaders_t(si, state->data, state->data + state->size) == end;
}
