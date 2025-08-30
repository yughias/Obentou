#include "cores/nes/ines.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u8 header_sign[] = { 0x4E, 0x45, 0x53, 0x1A};

static void ines1_load(ines_t* ines, u8 header[16]);
static void ines2_load(ines_t* ines, u8 header[16]);

void nes_ines_load(ines_t* ines, const char* filename){
    FILE* fptr = fopen(filename, "rb");

    if(!fptr){
        printf("cannot open %s\n", filename);
        exit(EXIT_FAILURE);
    }

    memset(ines, 0, sizeof(ines_t));

    u8 header[16];
    fread(header, 16, 1, fptr);
    if(memcmp(header, header_sign, sizeof(header_sign))){
        printf("file is not iNES\n");
        fclose(fptr);
        exit(EXIT_FAILURE);
    }

    bool ines2 = (header[7] & 0x0C) == 0x08;
    if(ines2)
        ines2_load(ines, header);
    else
        ines1_load(ines, header);
    
    if(header[6] & (1 << 3) && ines->mapper == 4)
        ines->vram_align = VRAM_4;

    if(ines->trainer_size){
        ines->trainer = malloc(ines->trainer_size);
        fread(ines->trainer, 512, 1, fptr);
    }

    ines->prg = malloc(ines->prg_size);
    fread(ines->prg, ines->prg_size, 1, fptr);

    ines->chr = malloc(ines->chr_size);
    fread(ines->chr, ines->chr_size, 1, fptr);

    ines->prg_ram_size = 1 << 13;
    ines->prg_ram = malloc(ines->prg_ram_size);

    fclose(fptr);
}

static void ines1_load(ines_t* ines, u8 header[16]){
    printf("using iNes1.0\n");

    ines->mapper = (header[6] >> 4) | (header[7] & 0xF0);
    ines->prg_size = header[4] * (1 << 14);
    ines->chr_size = header[5] * (1 << 13);
    ines->vram_align = header[6] & 1 ? VRAM_H : VRAM_V;
    if(header[6] & (1 << 3))
    ines->trainer_size = header[6] & (1 << 2) ? 512 : 0;

    if(!ines->chr_size){
        printf("chr ram detected\n");
        ines->is_chr_ram = true;
        ines->chr_size = (1 << 13);
    }
}


static void ines2_load(ines_t* ines, u8 header[16]){
    printf("using iNes2.0\n");

    ines->mapper = (header[6] >> 4) | (header[7] & 0xF0);
    printf("submapper %d\n", header[8] >> 4);
    ines->vram_align = header[6] & 1 ? VRAM_H : VRAM_V;
    if(header[6] & (1 << 3))
        ines->vram_align = VRAM_UND;
    ines->trainer_size = header[6] & (1 << 2) ? 512 : 0;
    ines->prg_size = (header[4] | ((header[9] & 0xF) << 8)) << 14;
    ines->chr_size = (header[5] | ((header[9] >> 4) << 8)) << 13;
    if(!ines->chr_size){
        ines->is_chr_ram = true;
        ines->chr_size = 1 << 13;
    }
}