#include "cores/pce/controller.h"
#include "utils/controls.h"

static control_t buttons[2][4] = {
    {CONTROL_PCE_UP, CONTROL_PCE_RIGHT, CONTROL_PCE_DOWN, CONTROL_PCE_LEFT},
    {CONTROL_PCE_BTN_1, CONTROL_PCE_BTN_2, CONTROL_PCE_SELECT, CONTROL_PCE_START}
};

void pce_controller_write(controller_t* c, u8 val){
    if(val & 0b10)
        c->taps = 0;

    if(!val & !c->taps)
        c->taps = 1;
}

u8 pce_controller_read(controller_t* c){
    if(c->taps == 2)
        return 0xFF;
    
    u8 out = 0xF;
    for(int i = 0; i < 4; i++)
        out &= ~(controls_pressed(buttons[c->taps][i], 0) << i);

    return out;
}