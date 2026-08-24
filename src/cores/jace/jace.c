#include "cores/jace/jace.h"

#include "utils/archive.h"
#include "utils/controls.h"
#include "utils/sound.h"

#include "SDL_MAINLOOP.h"

#include "ace.rom.h"

#define CYCLES_PER_FRAME 65000

// based on
// https://github.com/mamedev/mame/blob/04a5015022e6d79e321b54aa787714ff272fe0f9/src/mame/cantab/jupace.cpp#L583
#define SCREEN_X0 40
#define SCREEN_Y0 56
#define ACTIVE_WIDTH 256
#define ACTIVE_HEIGHT 192

static void jace_render(const jace_t* jace){
    background(0);

    int* active_screen = pixels + SCREEN_X0 + SCREEN_Y0 * stride;

    for (int y = 0; y < ACTIVE_HEIGHT; y++) {
        for (int x = 0; x < ACTIVE_WIDTH; x++) {
            int screen_idx = (x / 8) + (y / 8) * (ACTIVE_WIDTH / 8);
            int idx = jace->vram[screen_idx] & 0x7F;
            bool flip = jace->vram[screen_idx] & 0x80;
            int px = x & 7;
            int py = y & 7;
            bool pix = jace->cram[idx * 8 + py] & (1 << (px ^ 7));
            active_screen[x + y * stride] = pix ^ flip ? color(255, 255, 255) : color(0, 0, 0);
        }
    }
}

static u8 jace_read_keyboard(u8 addr) {
    u8 pressed_bits = 0; 

    // A8 (0xFE) - SHIFT, SYM_SHIFT, Z, X, C
    if ((uint8_t)(~addr) & (uint8_t)(~0xFE)) {
        bool arrows = false;

        arrows |= controls_pressed(CONTROL_JACE_UP, 0);
        arrows |= controls_pressed(CONTROL_JACE_DOWN, 0);
        arrows |= controls_pressed(CONTROL_JACE_LEFT, 0);
        arrows |= controls_pressed(CONTROL_JACE_RIGHT, 0);

        pressed_bits |= (controls_pressed(CONTROL_JACE_SHIFT, 0) || controls_pressed(CONTROL_JACE_DELETE, 0) || arrows) << 0; 
        pressed_bits |= controls_pressed(CONTROL_JACE_SYM_SHIFT, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_JACE_Z, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_JACE_X, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_JACE_C, 0) << 4;
    }

    // A9 (0xFD) - A, S, D, F, G
    if ((uint8_t)(~addr) & (uint8_t)(~0xFD)) {
        pressed_bits |= controls_pressed(CONTROL_JACE_A, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_JACE_S, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_JACE_D, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_JACE_F, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_JACE_G, 0) << 4;
    }

    // A10 (0xFB) - Q, W, E, R, T
    if ((uint8_t)(~addr) & (uint8_t)(~0xFB)) {
        pressed_bits |= controls_pressed(CONTROL_JACE_Q, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_JACE_W, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_JACE_E, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_JACE_R, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_JACE_T, 0) << 4;
    }

    // A11 (0xF7) - 1, 2, 3, 4, 5
    if ((uint8_t)(~addr) & (uint8_t)(~0xF7)) {
        pressed_bits |= controls_pressed(CONTROL_JACE_1, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_JACE_2, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_JACE_3, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_JACE_4, 0) << 3;
        pressed_bits |= (controls_pressed(CONTROL_JACE_5, 0) || controls_pressed(CONTROL_JACE_LEFT, 0)) << 4;
    }

    // A12 (0xEF) - 0, 9, 8, 7, 6
    if ((uint8_t)(~addr) & (uint8_t)(~0xEF)) {
        pressed_bits |= (controls_pressed(CONTROL_JACE_0, 0) || controls_pressed(CONTROL_JACE_DELETE, 0)) << 0;
        pressed_bits |= controls_pressed(CONTROL_JACE_9, 0) << 1;
        pressed_bits |= (controls_pressed(CONTROL_JACE_8, 0) || controls_pressed(CONTROL_JACE_RIGHT, 0)) << 2;
        pressed_bits |= (controls_pressed(CONTROL_JACE_7, 0) || controls_pressed(CONTROL_JACE_DOWN, 0)) << 3;
        pressed_bits |= (controls_pressed(CONTROL_JACE_6, 0) || controls_pressed(CONTROL_JACE_UP, 0)) << 4;
    }

    // A13 (0xDF) - P, O, I, U, Y
    if ((uint8_t)(~addr) & (uint8_t)(~0xDF)) {
        pressed_bits |= controls_pressed(CONTROL_JACE_P, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_JACE_O, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_JACE_I, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_JACE_U, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_JACE_Y, 0) << 4;
    }

    // A14 (0xBF) - ENTER, L, K, J, H
    if ((uint8_t)(~addr) & (uint8_t)(~0xBF)) {
        pressed_bits |= controls_pressed(CONTROL_JACE_ENTER, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_JACE_L, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_JACE_K, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_JACE_J, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_JACE_H, 0) << 4;
    }

    // A15 (0x7F) - SPACE, SYM SHIFT, M, N, B
    if ((uint8_t)(~addr) & (uint8_t)(~0x7F)) {
        pressed_bits |= controls_pressed(CONTROL_JACE_SPACE, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_JACE_M, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_JACE_N, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_JACE_B, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_JACE_V, 0) << 4;
    }

    u8 out = ~(pressed_bits | 0xE0);

    return out;
}

static u8 read_mem(void* ctx, u16 addr){
    jace_t* jace = ctx;

    if (addr < 0x2000)
        return jace->rom[addr];

    if (addr < 0x2800)
        return jace->vram[addr & 0x3FF];

    // TODO : WRITE ONLY
    if (addr < 0x3000)
        return jace->cram[addr & 0x3FF];

    if (addr < 0x4000)
        return jace->ram[addr & 0x3FF];

    return jace->expansion_ram[(addr - 0x4000) & 0x7FFF];
}

static void write_mem(void* ctx, u16 addr, u8 byte){
    jace_t* jace = ctx;

    if (addr < 0x2000)
        return;
    else if (addr < 0x2800)
        jace->vram[addr & 0x3FF] = byte;
    else if (addr < 0x3000)
        jace->cram[addr & 0x3FF] = byte;
    else if (addr < 0x4000)
        jace->ram[addr & 0x3FF] = byte;
    else
        jace->expansion_ram[(addr - 0x4000) & 0x7FFF] = byte;
}

static u8 read_io(void* ctx, u16 addr){
    jace_t* jace = ctx;
    
    if (!(addr & 1)) {
        jace->beeper = false;
        return jace_read_keyboard(addr >> 8) | (jace->ear_in << 5) | 0xC0;
    }

    return 0xFF;
}

static void write_io(void* ctx, u16 addr, u8 byte){
    jace_t* jace = ctx;

    printf("write io %04x %02x\n", addr, byte);
    
    if (!(addr & 1)) {
        jace->beeper = true;
    }
}


static void jace_load_ace_state(jace_t* jace, const u8* state, size_t size) {
    u8* buf = malloc(1 << 16);
    
    size_t src_idx = 0;
    size_t dst_idx = 0x2000;

    while (src_idx < size && dst_idx < 0x8001) {
        u8 current_byte = state[src_idx++];

        if (current_byte == 0xED) {
            u8 ace_byte = state[src_idx++];

            if (ace_byte == 0x00)
                break;
            else {
                u8 ace_repeat = state[src_idx++];
                memset(&buf[dst_idx], ace_repeat, ace_byte);
                dst_idx += ace_byte;
            }
        } else {
            buf[dst_idx++] = current_byte;
        }
    }

    int value = buf[0x2080] | (buf[0x2081] << 8);

    if (!(value & 0x3fff)) {
        jace->cpu.AF = buf[0x2100] | (buf[0x2101] << 8);
        jace->cpu.BC = buf[0x2104] | (buf[0x2105] << 8);
        jace->cpu.DE = buf[0x2108] | (buf[0x2109] << 8);
        jace->cpu.HL = buf[0x210c] | (buf[0x210d] << 8);
        jace->cpu.IX = buf[0x2110] | (buf[0x2111] << 8);
        jace->cpu.IY = buf[0x2114] | (buf[0x2115] << 8);
        jace->cpu.PC = buf[0x211c] | (buf[0x211d] << 8);
        jace->cpu.AF_ = buf[0x2120] | (buf[0x2121] << 8);
        jace->cpu.BC_ = buf[0x2124] | (buf[0x2125] << 8);
        jace->cpu.DE_ = buf[0x2128] | (buf[0x2129] << 8);
        jace->cpu.HL_ = buf[0x212c] | (buf[0x212d] << 8);
        jace->cpu.INTERRUPT_MODE = buf[0x2130];
        jace->cpu.IFF1 = buf[0x2134];
        jace->cpu.IFF2 = buf[0x2138];
        jace->cpu.I = buf[0x213c];
        jace->cpu.R = buf[0x2140];

        if ((buf[0x2119] < 0x80) || !value)
            jace->cpu.SP = buf[0x2118] | (buf[0x2119] << 8);
    }
    
    for (int i = 0x2000; i < 0x8000; i++)
        jace->cpu.writeMemory(jace, i, buf[i]);

    free(buf);
}

void* JACE_init(const archive_t* rom_archive, const archive_t* bios_archive){
    jace_t* jace = calloc(1, sizeof(jace_t));
    if(!jace)
        return NULL;

    z80_init(&jace->cpu);
    jace->cpu.readMemory = read_mem;
    jace->cpu.writeMemory = write_mem;
    jace->cpu.readIO = read_io;
    jace->cpu.writeIO = write_io;
    jace->cpu.ctx = jace;
    jace->tape.paused = true;

    jace->rom = assets_ace_rom;

    file_t* rom = archive_get_file_by_ext(rom_archive, "ace");
    if (rom)
        jace_load_ace_state(jace, rom->data, rom->size);

    rom = archive_get_file_by_ext(rom_archive, "tap");
    if (rom) {
        jace->tape.tape = rom;
        jace->tape.format = TAP_TAPE_LOAD;
    }

    rom = archive_get_file_by_ext(rom_archive, "wav");
    if (rom)
        jace->tape.wav = wav_load(rom->data, rom->size);

    return jace;
}

static void jace_get_sample(jace_t* jace, u8* sample){
    *sample = sound_set_channel_sample(jace->beeper * 64, 0);
}

static void jace_step(jace_t* jace, void (*z80_step)(z80_t*)){
    size_t prev_cycles = jace->cpu.cycles;
    z80_step(&jace->cpu);
    u8 sample;
    size_t elapsed = jace->cpu.cycles - prev_cycles;
    for (int i = 0; i < elapsed; i++)
        jace_tape_step(jace);
    sound_push_sample(elapsed, sizeof(sample), jace, &sample, (sound_get_sample_ptr)jace_get_sample);
}

void JACE_run_frame(jace_t* jace){
    jace->tape.paused ^= controls_released(CONTROL_JACE_TAPE_PLAY_STOP, 0);

    while (jace->cpu.cycles < CYCLES_PER_FRAME)
        jace_step(jace, z80_step);
    
    if (z80_is_interrupt_enabled(&jace->cpu))
        jace_step(jace, z80_irq);

    jace->cpu.cycles -= CYCLES_PER_FRAME;

    jace_render(jace);
    renderPixels();
}

bool JACE_detect(const archive_t* rom_archive, const archive_t* bios_archive){
    file_t* f = archive_get_file_by_ext(rom_archive, "ace");
    if (f) return true;
    f = archive_get_file_by_ext(rom_archive, "wav");
    if (f) return true;
    f = archive_get_file_by_ext(rom_archive, "tap");
    if (f && (f->data[0] >= 0x19 && f->data[0] <= 0x1A) && f->data[1] == 0x00) return true;
    return false;
}

byte_vec_t JACE_savestate(jace_t* jace){
    byte_vec_t state;
    byte_vec_init(&state);
    serialize_jace_t(jace, &state);
    byte_vec_shrink(&state);
    return state;
}

bool JACE_loadstate(jace_t* jace, byte_vec_t* state){
    const u8* end = state->data + state->size;
    return deserialize_jace_t(jace, state->data, state->data + state->size) == end;
}

void JACE_free(jace_t* jace) {
    if (jace->tape.wav.data)
        wav_free(&jace->tape.wav);
}