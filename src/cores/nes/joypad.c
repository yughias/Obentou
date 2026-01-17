#include "cores/nes/joypad.h"
#include "utils/controls.h"

#include "SDL_MAINLOOP.h"

static control_t maps[] = {
    CONTROL_NES_A, CONTROL_NES_B, CONTROL_NES_SELECT, CONTROL_NES_START,
    CONTROL_NES_UP, CONTROL_NES_DOWN, CONTROL_NES_LEFT, CONTROL_NES_RIGHT
};

u8 nes_joypad_read_1(joypad_t* joypad){
    bool out = joypad->controller_1_shifter & 1;
    if(!joypad->strobe)
        joypad->controller_1_shifter >>= 1;
    return out | 0x40;
}

u8 nes_joypad_read_2(joypad_t* joypad){
    SDL_MouseButtonFlags s = SDL_GetMouseState(NULL, NULL);
    bool trigger = !(s & SDL_BUTTON_LMASK);
    bool hit = true;
    if(pixels[mouseX+mouseY*width] == color(252, 252, 252))
        hit = false;
    return (trigger << 4) | (hit << 3);
}

void nes_joypad_write(joypad_t* joypad, u8 byte){
    joypad->strobe = byte & 1;
    if(joypad->strobe){
        for(int i = 0; i < 8; i++)
            joypad->controller_1_shifter |= controls_pressed(maps[i], 0) << i;
    }
}