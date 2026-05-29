#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <SDL_MAINLOOP.h>
#include <stdint.h>

extern uint8_t KEYREG_FEFE;
extern uint8_t KEYREG_FDFE;
extern uint8_t KEYREG_FBFE;
extern uint8_t KEYREG_F7FE;
extern uint8_t KEYREG_EFFE;
extern uint8_t KEYREG_DFFE;
extern uint8_t KEYREG_BFFE;
extern uint8_t KEYREG_7FFE;

extern uint8_t WORK_REG;

extern uint8_t ULA;
extern uint8_t KEMPSTON_REG;
extern uint8_t AY_REG[16];
extern uint8_t AY_SELECTED_REG;

void emulateKeyboard(const Uint8*);
void emulateKempstonJoystick(const Uint8*);
void emulateCursorJoystick(const Uint8*);
void calculateULAReg(uint8_t);

#endif