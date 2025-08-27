#include "cores/pv1000/controller.h"

#include "SDL2/SDL.h"

uint8_t pv1000_controller_read(controller_t* controller){
    const Uint8* keystate = SDL_GetKeyboardState(NULL);

    uint8_t out = 0;

    if(controller->selected_matrix & 8) {
        out |= keystate[SDL_SCANCODE_A];
        out |= keystate[SDL_SCANCODE_S] << 1;
    }

    if(controller->selected_matrix & 4) {
        out |= keystate[SDL_SCANCODE_LEFT];
        out |= keystate[SDL_SCANCODE_UP] << 1;
    }
    
    if(controller->selected_matrix & 2) {
        out |= keystate[SDL_SCANCODE_DOWN];
        out |= keystate[SDL_SCANCODE_RIGHT] << 1;
    }
    
    if(controller->selected_matrix & 1) {
        out |= keystate[SDL_SCANCODE_Z];
        out |= keystate[SDL_SCANCODE_X] << 1;
    }

    return out;
}