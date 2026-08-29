#include "cores/gamate/lcd.h"

#include "SDL_MAINLOOP.h"

static size_t IRQ_CPU_CYCLES = 16384;

static void inc_pointer(lcd_t* lcd) {
    lcd->pointer = ((lcd->pointer + lcd->increment) & 0x1FFF) | (lcd->pointer & (1 << 13));
}

void lcd_update_timer(lcd_t* lcd) {
    lcd->timer_counter += 1;
    while (lcd->timer_counter >= IRQ_CPU_CYCLES) {
        lcd->timer_counter -= IRQ_CPU_CYCLES;
        lcd->irq_req = true;
    }
}

void lcd_write_reg(lcd_t* lcd, u8 addr, u8 data) {
    switch (addr) {
        case 1:
        lcd->swap_plane = data & (1 << 4);
        lcd->window_mode = data & (1 << 5);
        lcd->increment = data & (1 << 6) ? 32 : 1;
        if (data & (1 << 7))
            memset(lcd->vram, 0, sizeof(lcd->vram));
        break;

        case 2:
        lcd->xscroll = data;
        break;

        case 3:
        lcd->yscroll = data;
        break;

        case 4:
        lcd->pointer &= ~(0x1F);
        lcd->pointer &= ~(1 << 13);
        lcd->pointer |= (data & 0x1F);
        lcd->pointer |= (data >> 7) << 13;
        break;

        case 5:
        lcd->pointer &= ~(0xFF << 5);
        lcd->pointer |= data << 5;
        break;

        case 7:
        lcd->vram[lcd->pointer] = data;
        inc_pointer(lcd);
        break;

        default:
        break;
    }
}

u8 lcd_read_reg(lcd_t* lcd, u8 addr) {
    if (addr == 6) {
        u8 out = lcd->vram[lcd->pointer];
        inc_pointer(lcd);
        return out;
    }

    return 0xFF;
}

int lcd_compose_pixel(lcd_t* lcd, int x, int y) {
    int idx = (x >> 3) + y * 32;
    int px = x & 7;
    bool bit0 = lcd->vram[idx] & (1 << (7-px));
    bool bit1 = lcd->vram[0x2000 | idx] & (1 << (7-px));
    u8 gamate_pix = lcd->swap_plane ? bit1 | (bit0 << 1) : (bit0) | (bit1 << 1);
    switch (gamate_pix) {
        case 0:
        return color(127, 134, 15);

        case 1:
        return color(87, 124, 68);
        
        case 2:
        return color(54, 93, 72);
        
        case 3:
        return color(42, 69, 59);

        default:
        break;
    }

    return 0;
}

void lcd_render(lcd_t* lcd) {
    for (int y = 0; y < height; y++) {
        u8 off_y = ((y + lcd->yscroll) % 200);
        if (lcd->window_mode && y < 16)
            off_y = y + 0xD0;
        for (int x = 0; x < width; x++) {
            u8 off_x = x + (lcd->window_mode && y < 16 ? 0 : lcd->xscroll); 
            pixels[x + y * stride] = lcd_compose_pixel(lcd, off_x, off_y);
        }
    }
    renderPixels();
}