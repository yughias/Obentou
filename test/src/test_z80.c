#include "cpus/z80.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u8 memory[1 << 16];
static u8 io[1 << 16];

static void copy_to_mem(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("can't open file: %s\n", filename);
        exit(EXIT_FAILURE);
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    fread(memory + 0x100, 1, size, f);
    fclose(f);
}

static u8 read_mem(void* ctx, u16 addr) {
    return memory[addr];
}

static void write_mem(void* ctx, u16 addr, u8 byte) {
    memory[addr] = byte;
}

static u8 read_io(void* ctx, u16 addr) {
    return io[addr];
}

static void write_io(void* ctx, u16 addr, u8 byte) {
    io[addr] = byte;
}

static void run_exerciser(const char* filename) {
    printf("\nrunning %s\n", filename);
    copy_to_mem(filename);

    z80_t z80;
    z80_init(&z80);
    z80.readMemory = read_mem;
    z80.writeMemory = write_mem;
    z80.readIO = read_io;
    z80.writeIO = write_io;
    z80.PC = 0x100;

    memory[0x5] = 0xC9;

    while (z80.PC) {
        z80_step(&z80);

        if (z80.PC == 5) {
            if (z80.C == 2)
                printf("%c", z80.E); 
            else if (z80.C == 9) {
                u16 i = z80.DE;
                while (memory[i] != '$') {
                    printf("%c", memory[i]);
                    i++;
                }
            }
        }
    }

    printf("\nend of program %s\n", filename);
}

int main() {
    run_exerciser("test/assets/zexdoc.com");
    run_exerciser("test/assets/zexall.com");
    
    return 0;
}