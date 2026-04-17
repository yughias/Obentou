#include "cores/spaceinvaders/spaceinvaders.h"
#include "cores/spaceinvaders/shifter.h"

void SI_shifter_write(shifter_t* shifter, u8 byte) {
    shifter->data = (byte << 8) | (shifter->data >> 8);
}

u8 SI_shifter_read(shifter_t* shifter) {
    return shifter->data >> (8 - shifter->amnt);
}
