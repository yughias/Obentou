#include "cores/speccy/speccy.h"
#include "cores/speccy/loader.h"

#define RAM_ADDR 0x4000

static void ula_contention(speccy_t* speccy){
    const int delay_pattern[] = {6, 5, 4, 3, 2, 1, 0, 0};
    const size_t delay_pattern_size = 8;

    int vertical_line = speccy->master_clock_counter / LINE_T_STATES;
    int clockOffset = speccy->master_clock_counter % LINE_T_STATES;

    if(vertical_line < 64 || vertical_line >= 64+SCREEN_HEIGHT)
        return;

    if(clockOffset >= SCREEN_T_STATES)
        return;

    speccy->cpu.cycles += delay_pattern[ clockOffset % delay_pattern_size];
}

static void mem_contention(speccy_t* speccy, uint16_t addr){
    if(addr < 0x4000 || addr > 0x7FFF)
        return;
    
    ula_contention(speccy);
}

uint8_t speccy_read_mem(void* ctx, uint16_t addr){
    speccy_t* speccy = ctx;
    mem_contention(speccy, addr);

    if(addr < RAM_ADDR)
        return speccy->rom[addr];

    return speccy->ram[addr - RAM_ADDR];
}

void speccy_write_mem(void* ctx, uint16_t addr, uint8_t byte){
    speccy_t* speccy = ctx;
    mem_contention(speccy, addr);
    if(addr < RAM_ADDR){
        return;
    }

    speccy->ram[addr - RAM_ADDR] = byte;
}

uint8_t speccy_read_io(void* ctx, uint16_t ioaddr){
    speccy_t* speccy = ctx;
    if(!(ioaddr & 1))
        ula_contention(speccy);

    if(!(ioaddr & 0b11100000))
        return speccy_get_kempston_state();

    return speccy_get_ula(ioaddr >> 8, speccy->ula);
}

void speccy_write_io(void* ctx, uint16_t ioaddr, uint8_t byte){
    speccy_t* speccy = ctx;
    ay_t* ay = &speccy->ay;
    if(!(ioaddr & 1)){
        ula_contention(speccy);
        speccy->ula = byte;
        return;
    }
    
    if((ioaddr >> 14) == 0b11 && !(ioaddr & 0b10)){
        ay->selected = byte;
        return;
    }

    if((ioaddr >> 14) == 0b10 && !(ioaddr & 0b10)){  
        ay_write_selected_port(ay, byte);
        return;
    }
}
