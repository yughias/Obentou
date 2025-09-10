#include "cores/gbc/joypad.h"
#include "peripherals/controls.h"

#include <SDL2/SDL.h>

static void emulateJoypad(joypad_t*);

const Uint8* keystate;
SDL_GameController* gameController;

void gb_initJoypad(){
    keystate = SDL_GetKeyboardState(NULL);
    gameController = SDL_GameControllerOpen(0);
    if(gameController)
        SDL_GameControllerSetSensorEnabled(gameController, SDL_SENSOR_ACCEL, SDL_TRUE);
}

static void emulateJoypad(joypad_t* joy){
    control_t controls[8] = {
        CONTROL_GBC_A, CONTROL_GBC_B, CONTROL_GBC_SELECT, CONTROL_GBC_START,
        CONTROL_GBC_RIGHT, CONTROL_GBC_LEFT, CONTROL_GBC_UP, CONTROL_GBC_DOWN
    };

    u8 new_arrow_btn = 0x0F;
    u8 new_action_btn = 0x0F;

    for(int i = 0; i < 4; i++){
        if(controls_pressed(controls[i]))
            new_action_btn &= ~(1 << i);

        if(controls_pressed(controls[i + 4]))
            new_arrow_btn &= ~(1 << i);
    }

    joy->ARROW_BTN = new_arrow_btn;
    joy->ACTION_BTN = new_action_btn;
}

u8 gb_getJoypadRegister(joypad_t* joy){
    u8 output_val;

    emulateJoypad(joy);

    u8 chosen = (joy->JOYP_REG >> 4) & 0b11;

    output_val = joy->JOYP_REG & 0b110000;
    output_val |= 0b11000000;

    if(chosen == 0b10)
        output_val |= joy->ARROW_BTN;
    
    if(chosen == 0b01)
        output_val |= joy->ACTION_BTN;

    if(chosen == 0b11)
        output_val = 0xFF;

    return output_val;
}

void gb_setGameControllerLed(){
    SDL_JoystickPowerLevel controllerBattery = SDL_JoystickCurrentPowerLevel(SDL_GameControllerGetJoystick(gameController));
    int red = 255;
    int led_values[4] = {5, 60, 100, 255};
    if(controllerBattery >= SDL_JOYSTICK_POWER_EMPTY && controllerBattery <= SDL_JOYSTICK_POWER_FULL)
        red = led_values[controllerBattery];
    SDL_GameControllerSetLED(gameController, red, 0, 0);
}