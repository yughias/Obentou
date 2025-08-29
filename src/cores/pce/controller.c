#include "cores/pce/controller.h"

#include "SDL_MAINLOOP.h"

static SDL_GameController* controller;

static SDL_Scancode buttons[2][4] = {
    {SDL_SCANCODE_UP, SDL_SCANCODE_RIGHT, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT},
    {SDL_SCANCODE_X, SDL_SCANCODE_Z, SDL_SCANCODE_RSHIFT, SDL_SCANCODE_RETURN}
};

static SDL_GameControllerButton controller_buttons[2][4] = {
    {SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT},
    {SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y}
};

void pce_controller_init(){
    if(SDL_NumJoysticks())
        controller = SDL_GameControllerOpen(0);   
}

void pce_controller_write(controller_t* c, u8 val){
    if(val & 0b10)
        c->taps = 0;

    if(!val & !c->taps)
        c->taps = 1;
}

u8 pce_controller_read(controller_t* c){
    if(c->taps == 2)
        return 0xFF;
    
    const Uint8* ks = SDL_GetKeyboardState(NULL);
    u8 out = 0xF;
    
    for(int i = 0; i < 4; i++){
        bool pressed = ks[buttons[c->taps][i]] || SDL_GameControllerGetButton(controller, controller_buttons[c->taps][i]);
        out &= ~(pressed << i);
    }

    return out;
}