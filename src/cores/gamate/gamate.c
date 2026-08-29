#include "cores/gamate/gamate.h"

#include "utils/archive.h"
#include "utils/controls.h"

#include <string.h>

#include "SDL_MAINLOOP.h"

static uint8_t COPYRIGHT_BYTE = 0x47;
static const char COPYRIGHT_STR[] = "COPYRIGHT BIT CORPORATION";
static size_t COPYRIGHT_STR_OFFSET = 0x05; 
static size_t CPU_CYCLES_PER_FRAME = 36450;

static u8 read_controller() {
    u8 out = 0xFF;
    const control_t controls[8] = {
        CONTROL_GAMATE_UP, CONTROL_GAMATE_DOWN, CONTROL_GAMATE_LEFT, CONTROL_GAMATE_RIGHT,
        CONTROL_GAMATE_A, CONTROL_GAMATE_B, CONTROL_GAMATE_START, CONTROL_GAMATE_SELECT 
    };
    for (size_t i = 0; i < 8; i++)
        if (controls_pressed(controls[i], 0))
            out &= ~(1 << i);
    return out;
}

static bool detect_4in1(u8* rom, size_t size) {
    if (size < (1 <<  17))
        return false;

    for (int i = 0; i < 4; i++)
        if (memcmp(rom + COPYRIGHT_STR_OFFSET + i * (1 << 15), COPYRIGHT_STR, strlen(COPYRIGHT_STR)))
            return false;

    return true;
}

static void gamate_write(void* ctx, u16 addr, u8 data) {
    gamate_t* gamate = ctx;

    lcd_update_timer(&gamate->lcd);
    
    if (addr < 0x2000) {
        gamate->ram[addr & 0x3FF] = data;
        return;
    }

    if (addr >= 0x5000 && addr < 0x5400) {
        lcd_write_reg(&gamate->lcd, addr & 0b111, data);
        return;
    }

    if (addr >= 0x8000 && addr < 0xA000) {
        gamate->first_bank = data;
        return;
    }

    if (addr >= 0xC000 && addr < 0xE000) {
        gamate->second_bank = data;
        return;
    }
}

static u8 gamate_read(void* ctx, u16 addr) {
    gamate_t* gamate = ctx;

    lcd_update_timer(&gamate->lcd);

    if (addr < 0x2000)
        return gamate->ram[addr & 0x3FF];

    if (addr >= 0x4400 && addr < 0x4800) {
        return read_controller();
    }

    if (addr >= 0x5000 && addr < 0x5400) {
        return lcd_read_reg(&gamate->lcd, addr & 0b111);
    }

    if (addr >= 0x5A00 && addr < 0x5B00) {
        return addr >> 8 | 0b11;
    }

    if (addr >= 0x6000 && addr < 0xA000) {
        if (gamate->copyright_read < 8) {
            bool bit = COPYRIGHT_BYTE & (1 << (7-gamate->copyright_read));
            gamate->copyright_read += 1;
            return bit << 1;
        }
        
        return gamate->rom[((gamate->is_4in1 ? gamate->first_bank << 14 : 0) | (addr - 0x6000)) % gamate->rom_size];
    }

    if (addr >= 0xA000 && addr < 0xE000) {
        return gamate->rom[((gamate->second_bank << 14) | (addr - 0xA000)) % gamate->rom_size];
    }

    if (addr >= 0xE000)
        return gamate->bios[addr & 0xFFF];

    return addr >> 8;
}

bool GAMATE_detect(const archive_t* rom_archive, const archive_t* bios_archive) {
    file_t* rom = archive_get_file_by_ext(rom_archive, "bin");
    if (!rom)
        return false;
    
    if (rom->size < 16)
        return false;

    if (!memcmp(rom->data + COPYRIGHT_STR_OFFSET, COPYRIGHT_STR, strlen(COPYRIGHT_STR)))
        return true;
    
    return false;
}

void* GAMATE_init(const archive_t* rom_archive, const archive_t* bios_archive) {
    file_t* bios = archive_get_file_by_ext(bios_archive, "bin");
    if (!bios) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bios needed", "Gamate BIOS is required", getMainWindow());
        return NULL;
    }
    file_t* rom = archive_get_file_by_ext(rom_archive, "bin");

    gamate_t* gamate = calloc(1, sizeof(gamate_t));
    gamate->rom = rom->data;
    gamate->rom_size = rom->size;
    gamate->bios = bios->data;

    gamate->is_4in1 = detect_4in1(gamate->rom, gamate->rom_size);
    gamate->first_bank = 0;
    gamate->second_bank = 1;


    m6502_init(&gamate->cpu);
    gamate->cpu.ctx = gamate;
    gamate->cpu.read = gamate_read;
    gamate->cpu.write = gamate_write;

    m6502_reset(&gamate->cpu);

    return gamate;
}

void GAMATE_run_frame(gamate_t* gamate) {
    m6502_t* cpu = &gamate->cpu;
    while (cpu->cycles < CPU_CYCLES_PER_FRAME) {
        if (gamate->lcd.irq_req && m6502_interrupt_enabled(cpu)) {
            m6502_irq(cpu);
            gamate->lcd.irq_req = false;
        }
        m6502_step(cpu);
    }
    cpu->cycles -= CPU_CYCLES_PER_FRAME;

    lcd_render(&gamate->lcd);
}

byte_vec_t GAMATE_savestate(gamate_t* gamate) {
    byte_vec_t state;
    return state;
}

bool GAMATE_loadstate(gamate_t* gamate, byte_vec_t* state) {
    return false;
}