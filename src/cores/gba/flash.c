#include "cores/gba/flash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void gba_flash_setup_memory(gba_gamepak_t* gamepak, size_t size, u16 id_code){
    gamepak->type = GAMEPAK_FLASH;
    gamepak->savMemorySize = size;
    gamepak->savMemory = malloc(size);
    gamepak->internalData = malloc(sizeof(flash_t));
    flash_t* flash = gamepak->internalData;
    flash->bank = 0;
    flash->state = FLASH_READY;
    flash->id_byte[0] = id_code & 0xFF;
    flash->id_byte[1] = id_code >> 8;
    memset(gamepak->savMemory, 0xFF, gamepak->savMemorySize);
    gamepak->savSizeMask = size - 1;

    if(size == FLASH_64K_SIZE) printf("FLASH 64K DETECTED!\n");
    if(size == FLASH_128K_SIZE) printf("FLASH 128K DETECTED!\n");
}

u8 gba_flash_read(gba_gamepak_t* gamepak, u16 addr){
    flash_t* flash = gamepak->internalData;
    if(flash->state == FLASH_ID_MODE && addr <= 1)
        return flash->id_byte[addr];

    int banked_addr = (addr + flash->bank * (1 << 16));
    return gamepak->savMemory[banked_addr & gamepak->savSizeMask]; 
}

void gba_flash_write(gba_gamepak_t* gamepak, u16 addr, u8 byte){
    flash_t* flash = gamepak->internalData;

    if(flash->state == FLASH_WRITE_MODE){
        int banked_addr = (addr + flash->bank * (1 << 16));
        gamepak->savMemory[banked_addr & gamepak->savSizeMask] = byte;
        flash->state = FLASH_READY;
        return;
    }

    if(flash->state == FLASH_ERASE_MODE){
        if(addr == 0x5555 && byte == 0x10){
            memset(gamepak->savMemory, 0xFF, gamepak->savMemorySize);
            flash->state = FLASH_READY;
            return;
        }

        if(byte == 0x30){
            int bank = addr & 0xF000;
            int banked_addr = (bank + flash->bank * (1 << 16));
            banked_addr &= gamepak->savSizeMask;
            memset(&gamepak->savMemory[banked_addr], 0xFF, 0x1000);
            flash->state = FLASH_READY;
            return;
        }
    }

    if(flash->state == FLASH_SET_BANK && addr == 0){
        flash->bank = byte & 1;
        flash->state = FLASH_READY;
        return;
    }

    if(addr == 0x5555){
        if(flash->state == FLASH_ID_MODE && byte == 0xF0){
            flash->state = FLASH_READY;
            return;
        }

        if(flash->state == FLASH_READY){
            switch(byte){
                case 0xAA:
                return;

                case 0xA0:
                flash->state = FLASH_WRITE_MODE;
                break;

                case 0x90:
                flash->state = FLASH_ID_MODE;
                return;

                case 0x80:
                flash->state = FLASH_ERASE_MODE;
                return;

                case 0xF0:
                flash->state = FLASH_READY;
                return;

                case 0xB0:
                flash->state = FLASH_SET_BANK;
                return;

                default:
                printf("UNKNOWN! %x\n", byte);
                return;
            }
        }
    }
}