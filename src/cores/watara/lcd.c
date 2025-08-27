#include "cores/watara/lcd.h"

#include "SDL_MAINLOOP.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

void watara_screen_update(lcd_t* lcd){
    u16 Vstart = (lcd->y_scroll * 0x30) & 0x1FFF;                   // initial V start value (limit to 8K size)
    if(Vstart == 0x1FE0) Vstart = 0;                                // if it has wrapped, perform the wrap here

    int end_y = lcd->y_size;
    int end_x = lcd->x_size >> 2;
    if(end_y > 160)
        end_y = 160;
    if(end_x > (160 >> 2) )
        end_x = 160 >> 2;

    for(int y = 0; y < end_y; y++){		                            // do Y_Size scanlines
        u16 Vtemp = Vstart + (lcd->x_scroll >> 2);                  // start address for this scanline
        u8 oldpix = lcd->vram[Vtemp];                               // get the pipeline rolling
        Vtemp++;
        for (int x = 0; x < end_x; x++){
            u16 pix16 = (lcd->vram[Vtemp] << 8) | oldpix;           // mingle old pixels from last read with new ones
            u8 pix4 = pix16 >> ((lcd->x_scroll & 0x03) << 1);       // get the correct 4 pixels (see below)
            for (int i = 0; i < 4; i++){
                u8 grey = 255 - ((pix4 & 3)*85);
                pixels[x*4+i + y*width] = color(grey, grey, grey);
                pix4 >>= 2;
            }                                                       // display them
            oldpix = pix16 >> 8;                                    // move current pixels into old pixels
            Vtemp++;
        }
        Vstart += ((lcd->x_size > 0xC3) ? 0x60 : 0x30) & 0x1FFF;    // add 30/60 and keep in 8K range
        if (Vstart == 0x1FE0) Vstart = 0;
    }
}