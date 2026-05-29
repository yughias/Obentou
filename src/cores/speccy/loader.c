// #include "cores/speccy/loader.h"

// #include <stdio.h>
// #include <stdint.h>
// #include "cores/speccy/speccy.h"

// void decodeVersion1Data(uint8_t* buffer, size_t filesize){
//     printf("v1 loading!\n");

//     bool isCompressed = buffer[12] & 0b100000;

//     if(isCompressed){
//         uint8_t* ptr = buffer + Z80_V1_HEADER_SIZE;
//         uint8_t* ram_ptr = MEMORY + RAM_ADDR;
//         while(!(ptr[0] == 0x00 && ptr[1] == 0xED && ptr[2] == 0xED && ptr[3] == 0x00)){
//             if(ptr[0] == 0xED && ptr[1] == 0xED){
//                 memset(ram_ptr, ptr[3], ptr[2]);
//                 ram_ptr += ptr[2];
//                 ptr += 4;
//             } else {
//                 ram_ptr[0] = ptr[0];
//                 ram_ptr += 1;
//                 ptr += 1;
//             }
//         }
//     } else
//         memcpy(MEMORY + RAM_ADDR, buffer + Z80_V1_HEADER_SIZE, filesize - Z80_V1_HEADER_SIZE);
// }

// void decodeVersion2Data(uint8_t* buffer, size_t filesize){
//     printf("v2 loading!\n");

//     uint16_t header_block_size = *(uint16_t*)(buffer + 30);  
//     cpu.PC = *(uint16_t*)(buffer + 32);

//     uint8_t* ptr = buffer + 32 + header_block_size;
//     uint8_t* memory_ptr = MEMORY;

//     while((ptr - buffer) < filesize){
//         uint16_t length = *(uint16_t*)ptr;
//         uint8_t  region = ptr[2];
//         uint8_t* memory_ptr;
//         switch(region){
//             case 0:
//             memory_ptr = MEMORY;
//             break;

//             case 4:
//             memory_ptr = MEMORY + 0x8000;
//             break;

//             case 5:
//             memory_ptr = MEMORY + 0xC000;
//             break;

//             case 8:
//             memory_ptr = MEMORY + 0x4000;
//             break;

//             default:
//             printf("error on decoding v2 file format!\n"); 
//             return;
//         }
//         ptr += 3;
//         if(length == 0xFFFF){
//             memcpy(memory_ptr, ptr, 16384);
//             ptr += 16384;
//         } else {
//             while(length != 0){
//                 if(ptr[0] == 0xED && ptr[1] == 0xED){
//                     memset(memory_ptr, ptr[3], ptr[2]);
//                     memory_ptr += ptr[2];
//                     ptr += 4;
//                     length -= 4;
//                 } else {
//                     memory_ptr[0] = ptr[0];
//                     memory_ptr += 1;
//                     ptr += 1;
//                     length -= 1;
//                 }
//             }
//         }
//     }
// }

// size_t getFileSize(const char* filename){
//     FILE* fptr = fopen(filename, "rb");
//     fseek(fptr, 0, SEEK_END);
//     size_t size = ftell(fptr);
//     fclose(fptr);
//     return size;
// }

// void loadROM(const char* filename){
//     size_t size = getFileSize(filename);
//     if(size > RAM_ADDR){
//         printf("rom too big, cut dimension to 16k!\n");
//         size = RAM_ADDR;
//     }
//     FILE* fptr = fopen(filename, "rb");
//     fread(MEMORY, size, 1, fptr);
//     fclose(fptr);
// }

// void loadState(const char* filename){
//     uint8_t* buf;

//     size_t size = getFileSize(filename);
//     buf = malloc(size);
    
//     FILE* fptr = fopen(filename, "rb");
//     fread(buf, size, 1, fptr);
//     fclose(fptr);

//     cpu.A = buf[0];
//     cpu.F = buf[1];
//     cpu.BC = *(uint16_t*)(buf + 2);
//     cpu.HL = *(uint16_t*)(buf + 4);
//     cpu.PC = *(uint16_t*)(buf + 6);
//     cpu.SP = *(uint16_t*)(buf + 8);
//     cpu.DE = *(uint16_t*)(buf + 13);
//     cpu.BC_ = *(uint16_t*)(buf + 15);
//     cpu.DE_ = *(uint16_t*)(buf + 17);
//     cpu.HL_ = *(uint16_t*)(buf + 19);

//     cpu.AF_ = (buf[21] << 8) | buf[22];
    
//     cpu.IYL = buf[23];
//     cpu.IYH = buf[24];
//     cpu.IXL = buf[25];
//     cpu.IXH = buf[26];

//     cpu.I = buf[10];
//     cpu.R = (buf[11] & 0x7F) | ((buf[12] & 0x1) << 7);
//     ULA = (buf[12] & 0b1110) >> 1;

//     cpu.IFF1 = buf[27];
//     cpu.INTERRUPT_MODE = buf[29] & 0b11;
//     cpu.INTERRUPT_PENDING = false;
//     cpu.HALTED = false;

//     if(cpu.PC != 0)
//         decodeVersion1Data(buf, size);
//     else
//         decodeVersion2Data(buf, size);

//     free(buf);
// }

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

//     uint8_t byte12 = (cpu.R >> 7) | ((ULA & 0b111) << 1);
//     fwrite(&byte12,                1, 1, fptr); // 12
//     fwrite(&cpu.DE,                2, 1, fptr); // 13
//     fwrite(&cpu.BC_,               2, 1, fptr); // 15
//     fwrite(&cpu.DE_,               2, 1, fptr); // 17
//     fwrite(&cpu.HL_,               2, 1, fptr); // 19

//     uint8_t a_ = cpu.AF_ >> 8;
//     uint8_t f_ = cpu.AF_ & 0xff;
//     fwrite(&a_,                    1, 1, fptr); // 21
//     fwrite(&f_,                    1, 1, fptr); // 22

//     fwrite(&cpu.IYL,               1, 1, fptr); // 23
//     fwrite(&cpu.IYH,               1, 1, fptr); // 24
//     fwrite(&cpu.IXL,               1, 1, fptr); // 25
//     fwrite(&cpu.IXH,               1, 1, fptr); // 26
//     fwrite(&cpu.IFF1, 1, 1, fptr); // 27
    
//     uint8_t iff2 = 0;
//     fwrite(&iff2,                  1, 1, fptr); // 28
//     fwrite(&cpu.INTERRUPT_MODE,    1, 1, fptr); // 29 

//     fwrite(MEMORY + RAM_ADDR, 1, MEMORY_SIZE - RAM_ADDR, fptr);

//     fclose(fptr);
// }

// void loadSCR(const char* filename){
//     FILE* fptr = fopen(filename, "rb");

//     uint8_t* ram_ptr = MEMORY + 0x4000;

//     while(fread(ram_ptr++, 1, 1, fptr));

//     fclose(fptr);

//     // set border to black 'cause is cool
//     ULA = 0;

//     // stuck cpu to allow image to be shown
//     cpu.PC = 0x38;
//     MEMORY[0x38] = 0x76;
// }