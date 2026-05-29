#include "cores/speccy/speccy.h"

// keyboard matrix regs
uint8_t KEYREG_FEFE;
uint8_t KEYREG_FDFE;
uint8_t KEYREG_FBFE;
uint8_t KEYREG_F7FE;
uint8_t KEYREG_EFFE;
uint8_t KEYREG_DFFE;
uint8_t KEYREG_BFFE;
uint8_t KEYREG_7FFE;

// ULA SETTINGS REGISTER
uint8_t ULA;

// THIS REGISTER CONTAIN THE CALCULATION DONE BY READ IO FUNCTION
uint8_t WORK_REG;

uint8_t KEMPSTON_REG;

uint8_t AY_REG[16];
uint8_t AY_SELECTED_REG;

#define MAP_KEY(key, reg, bit) if(keystate[SDL_SCANCODE_ ## key]) KEYREG_ ## reg |= (1 << bit)

void emulateKeyboard(const Uint8* keystate){
    KEYREG_FEFE = 0;
    KEYREG_FDFE = 0;
    KEYREG_FBFE = 0;
    KEYREG_F7FE = 0;
    KEYREG_EFFE = 0;
    KEYREG_DFFE = 0;
    KEYREG_BFFE = 0;
    KEYREG_7FFE = 0;
 
    // map left and  right shift to the seem bit of register FEFE
    MAP_KEY(LSHIFT,    FEFE, 0); MAP_KEY(RSHIFT, FEFE, 0);
    MAP_KEY(Z,         FEFE, 1);
    MAP_KEY(X,         FEFE, 2);
    MAP_KEY(C,         FEFE, 3);
    MAP_KEY(V,         FEFE, 4);
 
    MAP_KEY(A,         FDFE, 0);
    MAP_KEY(S,         FDFE, 1);
    MAP_KEY(D,         FDFE, 2);
    MAP_KEY(F,         FDFE, 3);
    MAP_KEY(G,         FDFE, 4);
 
    MAP_KEY(Q,         FBFE, 0);
    MAP_KEY(W,         FBFE, 1);
    MAP_KEY(E,         FBFE, 2);
    MAP_KEY(R,         FBFE, 3);
    MAP_KEY(T,         FBFE, 4);
 
    MAP_KEY(1,         F7FE, 0);
    MAP_KEY(2,         F7FE, 1);
    MAP_KEY(3,         F7FE, 2);
    MAP_KEY(4,         F7FE, 3);
    MAP_KEY(5,         F7FE, 4);
 
    MAP_KEY(0,         EFFE, 0);
    MAP_KEY(9,         EFFE, 1);
    MAP_KEY(8,         EFFE, 2);
    MAP_KEY(7,         EFFE, 3);
    MAP_KEY(6,         EFFE, 4);
 
    MAP_KEY(P,         DFFE, 0);
    MAP_KEY(O,         DFFE, 1);
    MAP_KEY(I,         DFFE, 2);
    MAP_KEY(U,         DFFE, 3);
    MAP_KEY(Y,         DFFE, 4);
 
    MAP_KEY(RETURN,    BFFE, 0);
    MAP_KEY(L,         BFFE, 1);
    MAP_KEY(K,         BFFE, 2);
    MAP_KEY(J,         BFFE, 3);
    MAP_KEY(H,         BFFE, 4);
 
    // TODO
    // if(activeInputDevice == KEYBOARD)
        MAP_KEY(SPACE,     7FFE, 0);
    // CONTROL MAP TO SYMBOL MAP
    MAP_KEY(RCTRL,     7FFE, 1); MAP_KEY(LCTRL, 7FFE, 1);
    MAP_KEY(RALT,      7FFE, 1); MAP_KEY(LALT, 7FFE, 1);
    MAP_KEY(M,         7FFE, 2);
    MAP_KEY(N,         7FFE, 3);
    MAP_KEY(B,         7FFE, 4);
 
    // CREATE A COMBO FOR BACKSPACE
    MAP_KEY(BACKSPACE, FEFE, 0);
    MAP_KEY(BACKSPACE, EFFE, 0);

    // CREATE A COMBO FOR ARROW KEYS
    // TODO
    // if(activeInputDevice == KEYBOARD){
        MAP_KEY(UP,    FEFE, 0);
        MAP_KEY(DOWN,  FEFE, 0);
        MAP_KEY(LEFT,  FEFE, 0);
        MAP_KEY(RIGHT, FEFE, 0);
        
        MAP_KEY(LEFT,  F7FE, 4);
        MAP_KEY(DOWN,  EFFE, 4);
        MAP_KEY(UP,    EFFE, 3);
        MAP_KEY(RIGHT, EFFE, 2);
    // }
} 

void emulateKempstonJoystick(const Uint8* keystate){
    KEMPSTON_REG = 0x00;
    //000FUDLR
    if(keystate[SDL_SCANCODE_RIGHT])
        KEMPSTON_REG |= 0b1;
    if(keystate[SDL_SCANCODE_LEFT])
        KEMPSTON_REG |= 0b10;
    if(keystate[SDL_SCANCODE_DOWN])
        KEMPSTON_REG |= 0b100;
    if(keystate[SDL_SCANCODE_UP])
        KEMPSTON_REG |= 0b1000;
    // TODO
    // if(activeInputDevice == KEMPSTON && keystate[SDL_SCANCODE_SPACE])
    //     KEMPSTON_REG |= 0b10000;
}

void emulateCursorJoystick(const Uint8* keystate){
    MAP_KEY(SPACE, EFFE, 0);
    
    MAP_KEY(LEFT,  F7FE, 4);
    MAP_KEY(DOWN,  EFFE, 4);
    MAP_KEY(UP,    EFFE, 3);
    MAP_KEY(RIGHT, EFFE, 2);
}

void calculateULAReg(uint8_t addr){
    WORK_REG = ~(uint8_t)0x1F;

    if((uint8_t)(~addr) & (uint8_t)(~0XFE))
        WORK_REG |= KEYREG_FEFE;

    if((uint8_t)(~addr) & (uint8_t)(~0XFD))
        WORK_REG |= KEYREG_FDFE;
    
    if((uint8_t)(~addr) & (uint8_t)(~0XFB))
        WORK_REG |= KEYREG_FBFE;

    if((uint8_t)(~addr) & (uint8_t)(~0XF7))
        WORK_REG |= KEYREG_F7FE;

    if((uint8_t)(~addr) & (uint8_t)(~0XEF))
        WORK_REG |= KEYREG_EFFE;

    if((uint8_t)(~addr) & (uint8_t)(~0XDF))
        WORK_REG |= KEYREG_DFFE;

    if((uint8_t)(~addr) & (uint8_t)(~0XBF))
        WORK_REG |= KEYREG_BFFE;

    if((uint8_t)(~addr) & (uint8_t)(~0X7F))
        WORK_REG |= KEYREG_7FFE;

    WORK_REG = ~WORK_REG;

    //bit 6 is EAR input bit
    WORK_REG |= (ULA & 0b10000) << 2;
}