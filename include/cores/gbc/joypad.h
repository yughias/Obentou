#ifndef __JOYPAD_H__
#define __JOYPAD_H__

#include "SDL_MAINLOOP.h"

#include "types.h"

#define JOYP_ADDR 0xFF00

typedef struct joypad_t {
    u8 JOYP_REG;
    u8 ARROW_BTN;
    u8 ACTION_BTN;
} joypad_t;

void gb_initJoypad();
u8 gb_getJoypadRegister(joypad_t*);
void gb_setGameControllerLed();

#endif