#include "cores/pv1000/controller.h"
#include "peripherals/controls.h"

u8 pv1000_controller_read(controller_t* controller){
    u8 out = 0;

    if(controller->selected_matrix & 8) {
        out |= controls_pressed(CONTROL_PV1000_BTN_1);
        out |= controls_pressed(CONTROL_PV1000_BTN_2) << 1;
    }

    if(controller->selected_matrix & 4) {
        out |= controls_pressed(CONTROL_PV1000_LEFT);
        out |= controls_pressed(CONTROL_PV1000_UP) << 1;
    }
    
    if(controller->selected_matrix & 2) {
        out |= controls_pressed(CONTROL_PV1000_DOWN);
        out |= controls_pressed(CONTROL_PV1000_RIGHT) << 1;
    }
    
    if(controller->selected_matrix & 1) {
        out |= controls_pressed(CONTROL_PV1000_SELECT);
        out |= controls_pressed(CONTROL_PV1000_START) << 1;
    }

    return out;
}