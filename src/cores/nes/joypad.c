#include "cores/nes/joypad.h"
#include "SDL_MAINLOOP.h"

#define NES_A       SDL_SCANCODE_X
#define NES_B       SDL_SCANCODE_Z
#define NES_SELECT  SDL_SCANCODE_RSHIFT
#define NES_START   SDL_SCANCODE_RETURN
#define NES_UP      SDL_SCANCODE_UP
#define NES_DOWN    SDL_SCANCODE_DOWN
#define NES_LEFT    SDL_SCANCODE_LEFT
#define NES_RIGHT   SDL_SCANCODE_RIGHT

static SDL_GameController* controller;

static SDL_Scancode keymaps[] = {
    NES_A, NES_B, NES_SELECT, NES_START,
    NES_UP, NES_DOWN, NES_LEFT, NES_RIGHT
};

static SDL_GameControllerButton buttons[] = {
    SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT
};

u8 nes_joypad_read_1(joypad_t* joypad){
    bool out = joypad->controller_1_shifter & 1;
    if(!joypad->strobe)
        joypad->controller_1_shifter >>= 1;
    return out | 0x40;
}

u8 nes_joypad_read_2(joypad_t* joypad){
    Uint32 s = SDL_GetMouseState(NULL, NULL);
    bool trigger = !(s & SDL_BUTTON(SDL_BUTTON_LEFT));
    bool hit = true;
    if(pixels[mouseX+mouseY*width] == color(252, 252, 252))
        hit = false;
    return (trigger << 4) | (hit << 3);
}

void nes_joypad_write(joypad_t* joypad, u8 byte){
    joypad->strobe = byte & 1;
    if(joypad->strobe){
        const Uint8* ks = SDL_GetKeyboardState(NULL);
        for(int i = 0; i < 8; i++){
            joypad->controller_1_shifter |= ks[keymaps[i]] << i;
            if(controller)
                joypad->controller_1_shifter |= SDL_GameControllerGetButton(controller, buttons[i]) << i;
        }
    }
}