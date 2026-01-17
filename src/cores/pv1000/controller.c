#include "cores/pv1000/controller.h"
#include "utils/controls.h"

u8 pv1000_controller_read(controller_t* controller){
    u8 out = 0;

    if(controller->selected_matrix & 8) {
        out |= controls_pressed(CONTROL_PV1000_BTN_1, 0);
        out |= controls_pressed(CONTROL_PV1000_BTN_2, 0) << 1;
    }

    if(controller->selected_matrix & 4) {
        out |= controls_pressed(CONTROL_PV1000_LEFT, 0);
        out |= controls_pressed(CONTROL_PV1000_UP, 0) << 1;
    }
    
    if(controller->selected_matrix & 2) {
        out |= controls_pressed(CONTROL_PV1000_DOWN, 0);
        out |= controls_pressed(CONTROL_PV1000_RIGHT, 0) << 1;
    }
    
    if(controller->selected_matrix & 1) {
        out |= controls_pressed(CONTROL_PV1000_SELECT, 0);
        out |= controls_pressed(CONTROL_PV1000_START, 0) << 1;
    }

    return out;
}