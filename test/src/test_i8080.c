#include "chips/i8080.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u8 memory[1 << 16];
static u8 io[1 << 8];

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

static u8 read_io(void* ctx, u8 addr) {
    return io[addr];
}

static void write_io(void* ctx, u8 addr, u8 byte) {
    io[addr] = byte;
}

static void run_exerciser(const char* filename) {
    copy_to_mem(filename);
    printf("\nrunning %s\n", filename);

    i8080_t i8080;
    i8080_initCPU(&i8080, NULL, read_mem, write_mem, read_io, write_io);
    i8080.PC = 0x100;

    memory[0x5] = 0xC9;

    while (i8080.PC) {
        i8080_stepCPU(&i8080);

        if (i8080.PC == 5) {
            if (i8080.C_8 == 2)
                printf("%c", i8080.E_8); 
            else if (i8080.C_8 == 9) {
                u16 i = i8080.D_16;
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
    run_exerciser("test/assets/8080PRE.com");
    run_exerciser("test/assets/TST8080.com");
    run_exerciser("test/assets/8080EXM.com");
    
    return 0;
}