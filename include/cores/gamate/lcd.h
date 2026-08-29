#ifndef __LCD_H__
#define __LCD_H__

#include "utils/serializer.h"
#include "types.h"

#define LCD_STRUCT(X) \
    X(u8, vram, 0x4000, 1, 0) \
    X(u32, timer_counter, 1, 0) \
    X(bool, irq_req, 1, 0) \
    X(bool, swap_plane, 1, 0) \
    X(bool, window_mode, 1, 0) \
    X(u16, pointer, 1, 0) \
    X(u8, increment, 1, 0) \
    X(u8, xscroll, 1, 0) \
    X(u8, yscroll, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(lcd, LCD_STRUCT);

void lcd_update_timer(lcd_t* lcd);
void lcd_write_reg(lcd_t* lcd, u8 addr, u8 data);
u8 lcd_read_reg(lcd_t* lcd, u8 addr);
int lcd_compose_pixel(lcd_t* lcd, int x, int y);
void lcd_render(lcd_t* lcd);

#endif