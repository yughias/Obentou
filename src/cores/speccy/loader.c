#include "cores/speccy/loader.h"
#include "cores/speccy/speccy.h"

#define Z80_V1_HEADER_SIZE 30

#include <stdio.h>

#include "cores/speccy/speccy.h"

static void decode_data_v1(speccy_t* speccy, u8* buffer, size_t filesize){
    printf("v1 loading!\n");

    bool isCompressed = buffer[12] & 0b100000;

    if(isCompressed){
        u8* ptr = buffer + Z80_V1_HEADER_SIZE;
        u8* ram_ptr = speccy->ram;
        while(!(ptr[0] == 0x00 && ptr[1] == 0xED && ptr[2] == 0xED && ptr[3] == 0x00)){
            if(ptr[0] == 0xED && ptr[1] == 0xED){
                memset(ram_ptr, ptr[3], ptr[2]);
                ram_ptr += ptr[2];
                ptr += 4;
            } else {
                ram_ptr[0] = ptr[0];
                ram_ptr += 1;
                ptr += 1;
            }
        }
    } else
        memcpy(speccy->ram, buffer + Z80_V1_HEADER_SIZE, filesize - Z80_V1_HEADER_SIZE);
}

static void decode_data_v2(speccy_t* speccy, u8* buffer, size_t filesize){
    printf("v2 loading!\n");

    u16 header_block_size = *(u16*)(buffer + 30);  
    speccy->cpu.PC = *(u16*)(buffer + 32);

    u8* ptr = buffer + 32 + header_block_size;

    while((ptr - buffer) < filesize){
        u16 length = *(u16*)ptr;
        u8  region = ptr[2];
        u8* memory_ptr;
        switch(region){
            case 0:
            memory_ptr = speccy->rom;
            break;

            case 4:
            memory_ptr = speccy->ram + 0x4000;
            break;

            case 5:
            memory_ptr = speccy->ram + 0x8000;
            break;

            case 8:
            memory_ptr = speccy->ram;
            break;

            default:
            printf("error on decoding v2 file format!\n"); 
            return;
        }
        ptr += 3;
        if(length == 0xFFFF){
            memcpy(memory_ptr, ptr, 16384);
            ptr += 16384;
        } else {
            while(length != 0){
                if(ptr[0] == 0xED && ptr[1] == 0xED){
                    memset(memory_ptr, ptr[3], ptr[2]);
                    memory_ptr += ptr[2];
                    ptr += 4;
                    length -= 4;
                } else {
                    memory_ptr[0] = ptr[0];
                    memory_ptr += 1;
                    ptr += 1;
                    length -= 1;
                }
            }
        }
    }
}

void speccy_load_z80_state(speccy_t* speccy, u8* buf, size_t size){
    z80_t* cpu = &speccy->cpu;

    cpu->A = buf[0];
    cpu->F = buf[1];
    cpu->BC = *(u16*)(buf + 2);
    cpu->HL = *(u16*)(buf + 4);
    cpu->PC = *(u16*)(buf + 6);
    cpu->SP = *(u16*)(buf + 8);
    cpu->DE = *(u16*)(buf + 13);
    cpu->BC_ = *(u16*)(buf + 15);
    cpu->DE_ = *(u16*)(buf + 17);
    cpu->HL_ = *(u16*)(buf + 19);

    cpu->AF_ = (buf[21] << 8) | buf[22];
    
    cpu->IYL = buf[23];
    cpu->IYH = buf[24];
    cpu->IXL = buf[25];
    cpu->IXH = buf[26];

    cpu->I = buf[10];
    cpu->R = (buf[11] & 0x7F) | ((buf[12] & 0x1) << 7);
    speccy->ula = (buf[12] & 0b1110) >> 1;

    cpu->IFF1 = buf[27];
    cpu->INTERRUPT_MODE = buf[29] & 0b11;
    cpu->HALTED = false;

    if(cpu->PC != 0)
        decode_data_v1(speccy, buf, size);
    else
        decode_data_v2(speccy, buf, size);
}

// TODO
// void saveState(const char* filename){
//     FILE* fptr = fopen(filename, "wb");

//     fwrite(&cpu.A,                  1, 1, fptr); // 00
//     fwrite(&cpu.F,                  1, 1, fptr); // 01
//     fwrite(&cpu.BC,                 2, 1, fptr); // 02
//     fwrite(&cpu.HL,                 2, 1, fptr); // 04
//     fwrite(&cpu.PC,                 2, 1, fptr); // 06
//     fwrite(&cpu.SP,                 2, 1, fptr); // 08
//     fwrite(&cpu.I,                  1, 1, fptr); // 10
//     fwrite(&cpu.R,                  1, 1, fptr); // 11

//     u8 byte12 = (cpu.R >> 7) | ((ULA & 0b111) << 1);
//     fwrite(&byte12,                1, 1, fptr); // 12
//     fwrite(&cpu.DE,                2, 1, fptr); // 13
//     fwrite(&cpu.BC_,               2, 1, fptr); // 15
//     fwrite(&cpu.DE_,               2, 1, fptr); // 17
//     fwrite(&cpu.HL_,               2, 1, fptr); // 19

//     u8 a_ = cpu.AF_ >> 8;
//     u8 f_ = cpu.AF_ & 0xff;
//     fwrite(&a_,                    1, 1, fptr); // 21
//     fwrite(&f_,                    1, 1, fptr); // 22

//     fwrite(&cpu.IYL,               1, 1, fptr); // 23
//     fwrite(&cpu.IYH,               1, 1, fptr); // 24
//     fwrite(&cpu.IXL,               1, 1, fptr); // 25
//     fwrite(&cpu.IXH,               1, 1, fptr); // 26
//     fwrite(&cpu.IFF1, 1, 1, fptr); // 27
    
//     u8 iff2 = 0;
//     fwrite(&iff2,                  1, 1, fptr); // 28
//     fwrite(&cpu.INTERRUPT_MODE,    1, 1, fptr); // 29 

//     fwrite(MEMORY + RAM_ADDR, 1, MEMORY_SIZE - RAM_ADDR, fptr);

//     fclose(fptr);
// }

void speccy_load_scr(speccy_t* speccy, u8* buffer, size_t size){
    memcpy(speccy->ram, buffer, size);

    // set border to black 'cause is cool
    speccy->ula = 0;

    // stuck cpu to allow image to be shown
    speccy->cpu.PC = 0x38;
    speccy->rom[0x38] = 0x76;
}