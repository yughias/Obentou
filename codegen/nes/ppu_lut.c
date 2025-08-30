#include <stdio.h>
#include <string.h>

#define SCANLINES 262
#define PPU_CYCLES_PER_SCANLINE 341
#define PPU_CYCLES_PER_FRAME (SCANLINES*PPU_CYCLES_PER_SCANLINE)

char buffer[256];

void ppu_generate_task(int cycle);

int main(){
    printf("{\n");
    for(int i = 0; i < PPU_CYCLES_PER_FRAME; i++){
        ppu_generate_task(i);
        printf("%s, ", buffer);
        if(i % PPU_CYCLES_PER_SCANLINE == PPU_CYCLES_PER_SCANLINE - 1)
            printf("\n");
    }
    printf("}\n");
}

void ppu_generate_task(int i){
    buffer[0] = 0;
    strcat(buffer, "PPU_TASK");
    int cycles = i % PPU_CYCLES_PER_FRAME;
    int intra_scan_cycles = cycles % PPU_CYCLES_PER_SCANLINE;
    int scanline = cycles / PPU_CYCLES_PER_SCANLINE;

    if(cycles == 1+PPU_CYCLES_PER_SCANLINE*241){
        strcat(buffer, "_VBLANK_START");
        return;
    }
    
    if(cycles == 1+PPU_CYCLES_PER_SCANLINE*261){
        strcat(buffer, "_VBLANK_END");
        return;
    }

    if(!intra_scan_cycles){
        strcat(buffer, "_IDLE");
        return; 
    }

    if(cycles == PPU_CYCLES_PER_FRAME - 1){
        strcat(buffer, "_END_OF_FRAME");
    }

    if(scanline < 240 || scanline == 261){
        if(scanline < 240){
            if(intra_scan_cycles >= 2 && intra_scan_cycles < 258)
                strcat(buffer, "_PUT_PIX_SHIFT");
            if(intra_scan_cycles == 257)
                strcat(buffer, "_LOAD_OAM");
            if(intra_scan_cycles >= 261 && intra_scan_cycles < 320){
                if((intra_scan_cycles - 261) % 8 == 0){
                    strcat(buffer, "_FETCH_SPRITE_LO");
                }
                if((intra_scan_cycles - 263) % 8 == 0){
                    strcat(buffer, "_FETCH_SPRITE_HI");
                }
            }
        }

        if(intra_scan_cycles >= 321 && intra_scan_cycles < 338)
            strcat(buffer, "_SHIFT");

        if(intra_scan_cycles >= 2 && (intra_scan_cycles - 9) % 8 == 0)
            strcat(buffer, "_FILL_SHIFTER");

        switch(intra_scan_cycles & 0b111){
            case 0:
            if(intra_scan_cycles <= 256 || intra_scan_cycles == 328 || intra_scan_cycles == 336)
                strcat(buffer, "_INC_HORI");
            break;

            case 1:
            strcat(buffer, "_READ_NT");
            break;

            case 3:
            if(intra_scan_cycles < 252 || intra_scan_cycles == 323 || intra_scan_cycles == 331){
                strcat(buffer, "_READ_AT");
            }
            if(intra_scan_cycles > 256 && (intra_scan_cycles < 321 || intra_scan_cycles == 339)){
                strcat(buffer, "_READ_NT");
            }
            break;

            case 5:
            if(intra_scan_cycles < 252 || intra_scan_cycles == 325 || intra_scan_cycles == 333){
                strcat(buffer, "_READ_BG_LSB");
            }
            break;

            case 7:
            if(intra_scan_cycles < 252 || intra_scan_cycles == 327 || intra_scan_cycles == 335){
                strcat(buffer, "_READ_BG_MSB");
            }
            break;
        }

        if(intra_scan_cycles == 256){
            strcat(buffer, "_INC_VERT");
        }
        if(intra_scan_cycles == 257)
            strcat(buffer, "_RESET_HORI");
        if(scanline == 261){
            if(intra_scan_cycles >= 280 && intra_scan_cycles <= 304){
                strcat(buffer, "_RESET_VERT");
            }
        }
    }

    if(!strcmp(buffer, "PPU_TASK"))
        strcat(buffer, "_IDLE");
}