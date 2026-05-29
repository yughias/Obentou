#include "cores/speccy/speccy.h"
#include <SDL_MAINLOOP.h>

bool flashRevert = false;

// http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/screen-memory-layout
uint16_t getScreenPixelIndex(uint8_t x_pix, uint8_t y_pix){
    uint8_t x_byte = x_pix >> 3;
    uint16_t index = 0b01000000 << 8;
    index |= x_byte;
    uint8_t reorder_y = y_pix & 0b11000000;
    reorder_y |= (y_pix & 0b111) << 3;
    reorder_y |= (y_pix & 0b00111000) >> 3; 
    index |= reorder_y << 5;

    return index;
}

int getPixelColor(bool isForeground, uint8_t x_byte, uint8_t y_byte){
    // isForeground:
    //           true  => the dot on the screen is a foreground pixel
    //           false => the dot on the screen is a background pixel
    
    uint8_t colorData = MEMORY[0x5800+x_byte+y_byte*32];

    // according to https://worldofspectrum.org/faq/reference/48kreference.htm
    // flashing happens every 16 frames    
    if(colorData & 0x80 && flashRevert)
        isForeground = !isForeground;

    uint8_t shifter = !isForeground * 3;
    uint8_t r = ((colorData >> shifter) & 0b10) >> 1 ;
    uint8_t g = ((colorData >> shifter) & 0b100) >> 2;
    uint8_t b = (colorData >> shifter) & 0b1;
    r *= 0xD8;
    g *= 0xD8;
    b *= 0xD8; 

    if(colorData & 0b01000000){
        r = r ? 0xFF : 0x00;
        g = g ? 0xFF : 0x00;
        b = b ? 0xFF : 0x00;
    }

    return color(r, g, b);
}

void updateColorFlash(){
    if(frameCount % 16 == 0)
        flashRevert = !flashRevert;
}

void getBorderColor(uint8_t* r, uint8_t* g, uint8_t* b){
    *r = (ULA & 0b10) >> 1 ;
    *g = (ULA & 0b100) >> 2;
    *b = ULA & 0b1;
    *r *= 0xD8;
    *g *= 0xD8;
    *b *= 0xD8; 
}

void emulateUlaRender(size_t clock){
    if(clock < NON_VISIBLE_LINE*LINE_T_STATES)
        return;
    
    int scanline = clock / LINE_T_STATES - NON_VISIBLE_LINE;
    int clockOffset = clock % LINE_T_STATES;

    int x = clockOffset*2;
    int y = scanline - SCREEN_Y0;

    for(int i = 0; i < 2; i++, x++){

        if(clockOffset < SCREEN_T_STATES){
            if(scanline >= SCREEN_Y0 && scanline < SCREEN_HEIGHT+SCREEN_Y0){
                uint16_t index = getScreenPixelIndex(x, y); 
                uint8_t pixels8 = MEMORY[index];
                bool isForeground = pixels8 & ( 1 << (7 - (x % 8) ) );
                int col = getPixelColor(isForeground, x >> 3, y >> 3);
                pixels[(SCREEN_X0+x) + (SCREEN_Y0+y)*width] = col;
            } else {
                uint8_t r, g, b;
                getBorderColor(&r, &g, &b);
                pixels[(SCREEN_X0+x) + (SCREEN_Y0+y)*width] = color(r, g, b);
            }
        } else if(clockOffset < SCREEN_T_STATES + BORDER_T_STATES){
            uint8_t r, g, b;
            getBorderColor(&r, &g, &b);
            pixels[(SCREEN_X0+x) + (SCREEN_Y0+y)*width] = color(r, g, b);
        } else if(clockOffset < SCREEN_T_STATES + BORDER_T_STATES + RETRACE_T_STATES){
            return;
        } else {
            uint8_t r, g, b;
            getBorderColor(&r, &g, &b);
            pixels[x-(SCREEN_T_STATES + BORDER_T_STATES + RETRACE_T_STATES)*2 + (SCREEN_Y0+y)*width] = color(r, g, b);
        }
        
    }
}