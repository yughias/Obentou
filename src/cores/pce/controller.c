#include "cores/pce/controller.h"
#include "utils/controls.h"

// real turbo tap supports 5 gamepads
#define PCE_MAX_GAMEPADS 2

static control_t buttons[2][4] = {
    {CONTROL_PCE_UP, CONTROL_PCE_RIGHT, CONTROL_PCE_DOWN, CONTROL_PCE_LEFT},
    {CONTROL_PCE_BTN_1, CONTROL_PCE_BTN_2, CONTROL_PCE_SELECT, CONTROL_PCE_START}
};

void pce_controller_write(controller_t* c, u8 val){
    bool prev_sel = c->sel;
    bool prev_clr = c->clr;
    c->sel = val & (1 << 0);
    c->clr = val & (1 << 1);
    c->reg = 0x3F;

    // EMULATE JAP VERSION
    c->reg |= (1 << 6);

    // mutex emulation for turbo tap
    if(!c->clr && !prev_sel && c->sel && c->selected_pad < PCE_MAX_GAMEPADS)
        c->selected_pad++;

    if(c->sel && !prev_clr && c->clr)
        c->selected_pad = 0;

    if (c->selected_pad >= PCE_MAX_GAMEPADS)
        return;

    if(!c->clr){
        for(int i = 0; i < 4; i++)
            c->reg &= ~(controls_pressed(buttons[!c->sel][i], c->selected_pad) << i);
    }
}

u8 pce_controller_read(controller_t* c){
    return c->reg;
}