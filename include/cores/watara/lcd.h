#ifndef __LCD_H__
#define __LCD_H__

#define VRAM_SIZE 0x2000

#include "types.h"

typedef struct lcd_t {
    u8 vram[VRAM_SIZE];
    u8 x_size;
    u8 y_size;
    u8 x_scroll;
    u8 y_scroll;
} lcd_t;

void watara_screen_update(lcd_t* lcd);

#endif