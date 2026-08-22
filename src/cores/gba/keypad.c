#include "cores/gba/keypad.h"

#include "utils/controls.h"

u16 gba_keypad_read() {
    const control_t controls[10] = {
        CONTROL_GBA_A, CONTROL_GBA_B, CONTROL_GBA_SELECT, CONTROL_GBA_START,
        CONTROL_GBA_RIGHT, CONTROL_GBA_LEFT, CONTROL_GBA_UP, CONTROL_GBA_DOWN,
        CONTROL_GBA_R, CONTROL_GBA_L
    };

    u16 out = 0x3FF;

    for (int i = 0; i < 10; i++)
        if (controls_pressed(controls[i], 0))
            out &= ~(1 << i);

    return out;
}