#include "cores/gbc/mbc.h"
#include "cores/gbc/memory.h"
#include "cores/gbc/joypad.h"
#include "cores/gbc/gb.h"

#include "SDL_MAINLOOP.h"

#include <string.h>

typedef struct {
    bool DO;
    bool DI;
    bool CLK;
    bool CS;
    u16 command;
    u16 read_bits;
    u8 argument_bits_left;
} eeprom_t;

typedef struct mbc7_t {
    eeprom_t eeprom;
    u16 accelerometer_x;
    u16 accelerometer_y; 
} mbc7_t;

void* gb_mbc7_alloc(){
    void* out = malloc(sizeof(mbc7_t));
    memset(out, 0, sizeof(mbc7_t));
    return out; 
}

static void stepEeprom(u8* eram, eeprom_t* eeprom, u8 value);

u8 gb_mbc7_ram_read(gb_t* gb, u16 addr){
    mbc_t* mbc = &gb->mbc;
    mbc7_t* mbc7 = (mbc7_t*)mbc->data;
    if(mbc->REG_0000_1FFF != 0x0A || mbc->REG_4000_5FFF != 0x40)
        return 0xFF;

    if((addr & 0xF0F0) == 0xA020)
        return mbc7->accelerometer_x & 0xFF;
    if((addr & 0xF0F0) == 0xA030)
        return mbc7->accelerometer_x >> 8;

    if((addr & 0xF0F0) == 0xA040)
        return mbc7->accelerometer_y & 0xFF;
    if((addr & 0xF0F0) == 0xA050)
        return mbc7->accelerometer_y >> 8;

    if((addr & 0xF0F0) == 0xA080){
        eeprom_t* eeprom = &mbc7->eeprom;
        return eeprom->DO | (eeprom->DI << 1) | (eeprom->CLK << 6) | (eeprom->CS << 7);
    }

    return 0xFF;
}

void gb_mbc7_ram_write(gb_t* gb, u16 addr, u8 byte){
    mbc_t* mbc = &gb->mbc;
    mbc7_t* mbc7 = (mbc7_t*)mbc->data;
    if(mbc->REG_0000_1FFF != 0x0A || mbc->REG_4000_5FFF != 0x40)
        return;

    if((addr & 0xF0F0) == 0xA000){
        if(byte == 0x55){
            mbc7->accelerometer_x = 0x8000;
            mbc7->accelerometer_y = 0x8000;
        }
    }

    if((addr & 0xF0F0) == 0xA010){
        if(byte == 0xAA){
            int dx = 0;
            int dy = 0;
            //TODO
            Uint32 ms = SDL_GetMouseState(NULL, NULL);
            bool isMousePressed = ms & SDL_BUTTON(SDL_BUTTON_LEFT);
            if(isMousePressed){
                dx = -(mouseX - width / 2);
                dy = -(mouseY - height / 2);
            }/* else if(gameController){
                float sensors[3] = { 0.f };
                SDL_GameControllerGetSensorData(gameController, SDL_SENSOR_ACCEL, sensors, 3);
                dx = sensors[0] * 10;
                dy = (sensors[2] - 2) * 10;
            }*/
            mbc7->accelerometer_x = 0x81D0 + dx;
            mbc7->accelerometer_y = 0x81D0 + dy;
        }
    }

    if((addr & 0xF0F0) == 0xA080)
        stepEeprom(gb->ERAM, &mbc7->eeprom, byte);
}

/*
https://github.com/LIJI32/SameBoy/blob/master/Core/memory.c
https://web.archive.org/web/20230126183707/https://ww1.microchip.com/downloads/aemDocuments/documents/MPD/ProductDocuments/DataSheets/21712C.pdf
*/

static void stepEeprom(u8* eram, eeprom_t* eeprom, u8 value){
    eeprom->CS = value & 0x80;
    eeprom->DI = value & 0x02;

    if(eeprom->CS){
        if(!eeprom->CLK && (value & 0x40)) { // CLK posedge
            eeprom->DO = eeprom->read_bits >> 15;
            eeprom->read_bits <<= 1;
            eeprom->read_bits |= 1;
            if(eeprom->argument_bits_left == 0) {
                /* Not transferring extra bits for a command*/
                eeprom->command <<= 1;
                eeprom->command |= eeprom->DI;
                if (eeprom->command & 0x400) {
                    // Got full command
                    switch ((eeprom->command >> 6) & 0xF) {
                        case 0x8: case 0x9: case 0xA: case 0xB:
                            // READ
                            eeprom->read_bits = ((u16 *)eram)[eeprom->command & 0x7F];
                            eeprom->command = 0;
                            break;
                        case 0x3: // EWEN (Eeprom Write ENable)
                            eeprom->command = 0;
                            break;
                        case 0x0: // EWDS (Eeprom Write DiSable)
                            eeprom->command = 0;
                            break;
                        case 0x4: case 0x5: case 0x6: case 0x7:
                            // WRITE
                            ((u16 *)eram)[eeprom->command & 0x7F] = 0;
                            eeprom->argument_bits_left = 16;
                            // We still need to process this command, don't erase eeprom_command
                            break;
                        case 0xC: case 0xD: case 0xE: case 0xF:
                            // ERASE
                            ((u16 *)eram)[eeprom->command & 0x7F] = 0xFFFF;
                            eeprom->command = 0;
                            break;
                        case 0x2:
                            // ERAL (ERase ALl)
                            memset(eram, 0xFF, 256);
                            eeprom->command = 0;
                            break;
                        case 0x1:
                            // WRAL (WRite ALl)
                            memset(eram, 0, 256);
                            eeprom->argument_bits_left = 16;
                            // We still need to process this command, don't erase eeprom_command
                            break;
                    }
                }
            }
            else {
                // We're shifting in extra bits for a WRITE/WRAL command
                eeprom->argument_bits_left--;
                eeprom->DO = true;
                if (eeprom->DI) {
                    u16 bit = (1 << eeprom->argument_bits_left);
                    if (eeprom->command & 0x100) {
                        // WRITE
                        ((u16 *)eram)[eeprom->command & 0x7F] |= bit;
                    }
                    else {
                        // WRAL
                        for (unsigned i = 0; i < 0xFF; i++) {
                            ((u16 *)eram)[i] |= bit;
                        }
                    }
                }
                if (eeprom->argument_bits_left == 0) // We're done
                    eeprom->command = 0;
            }
        }
    }
    eeprom->CLK = value & 0x40;
}