#include "cores/speccy/speccy.h"

#include "utils/controls.h"

u8 speccy_get_kempston_state() {
    if (controls_get_actual_type() != CONTROL_TYPE_SPECCY_KEYBOARD_WITH_KEMPSTON)
        return 0;

    u8 out = 0;
    out |= controls_pressed(CONTROL_SPECCY_RIGHT, 0) << 0;
    out |= controls_pressed(CONTROL_SPECCY_LEFT, 0) << 1;
    out |= controls_pressed(CONTROL_SPECCY_DOWN, 0) << 2;
    out |= controls_pressed(CONTROL_SPECCY_UP, 0) << 3;
    out |= controls_pressed(CONTROL_SPECCY_JOY_BTN, 0) << 4;
    return out;
}

u8 speccy_get_ula(u8 addr, u8 ula) {
    u8 pressed_bits = 0; 

    // A8 (0xFE) - SHIFT, Z, X, C, V
    if ((uint8_t)(~addr) & (uint8_t)(~0xFE)) {
        bool arrows = false;
        if (controls_get_actual_type() == CONTROL_TYPE_SPECCY_KEYBOARD) {
            arrows |= controls_pressed(CONTROL_SPECCY_UP, 0);
            arrows |= controls_pressed(CONTROL_SPECCY_DOWN, 0);
            arrows |= controls_pressed(CONTROL_SPECCY_LEFT, 0);
            arrows |= controls_pressed(CONTROL_SPECCY_RIGHT, 0);
        }

        pressed_bits |= (controls_pressed(CONTROL_SPECCY_SHIFT, 0) || controls_pressed(CONTROL_SPECCY_DELETE, 0) || arrows) << 0; 
        pressed_bits |= controls_pressed(CONTROL_SPECCY_Z, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_X, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_C, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_V, 0) << 4;
    }

    // A9 (0xFD) - A, S, D, F, G
    if ((uint8_t)(~addr) & (uint8_t)(~0xFD)) {
        pressed_bits |= controls_pressed(CONTROL_SPECCY_A, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_S, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_D, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_F, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_G, 0) << 4;
    }

    // A10 (0xFB) - Q, W, E, R, T
    if ((uint8_t)(~addr) & (uint8_t)(~0xFB)) {
        pressed_bits |= controls_pressed(CONTROL_SPECCY_Q, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_W, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_E, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_R, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_T, 0) << 4;
    }

    // A11 (0xF7) - 1, 2, 3, 4, 5
    if ((uint8_t)(~addr) & (uint8_t)(~0xF7)) {
        pressed_bits |= controls_pressed(CONTROL_SPECCY_1, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_2, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_3, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_4, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_5, 0) << 4;

        switch (controls_get_actual_type()) {
            case CONTROL_TYPE_SPECCY_KEYBOARD:
            case CONTROL_TYPE_SPECCY_KEYBOARD_WITH_CURSOR:
            pressed_bits |= controls_pressed(CONTROL_SPECCY_LEFT, 0) << 4;
            break;

            default:
            break;
        }
    }

    // A12 (0xEF) - 0, 9, 8, 7, 6
    if ((uint8_t)(~addr) & (uint8_t)(~0xEF)) {
        pressed_bits |= (controls_pressed(CONTROL_SPECCY_0, 0) || controls_pressed(CONTROL_SPECCY_DELETE, 0)) << 0;
        if (controls_get_actual_type() == CONTROL_TYPE_SPECCY_KEYBOARD_WITH_CURSOR)
            pressed_bits |= controls_pressed(CONTROL_SPECCY_JOY_BTN, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_9, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_8, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_7, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_6, 0) << 4;

        switch (controls_get_actual_type()) {
            case CONTROL_TYPE_SPECCY_KEYBOARD:
            case CONTROL_TYPE_SPECCY_KEYBOARD_WITH_CURSOR:
            pressed_bits |= controls_pressed(CONTROL_SPECCY_DOWN, 0) << 4;
            pressed_bits |= controls_pressed(CONTROL_SPECCY_UP, 0) << 3;
            pressed_bits |= controls_pressed(CONTROL_SPECCY_RIGHT, 0) << 2;
            break;

            default:
            break;
        }
    }

    // A13 (0xDF) - P, O, I, U, Y
    if ((uint8_t)(~addr) & (uint8_t)(~0xDF)) {
        pressed_bits |= controls_pressed(CONTROL_SPECCY_P, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_O, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_I, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_U, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_Y, 0) << 4;
    }

    // A14 (0xBF) - ENTER, L, K, J, H
    if ((uint8_t)(~addr) & (uint8_t)(~0xBF)) {
        pressed_bits |= controls_pressed(CONTROL_SPECCY_ENTER, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_L, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_K, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_J, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_H, 0) << 4;
    }

    // A15 (0x7F) - SPACE, SYM SHIFT, M, N, B
    if ((uint8_t)(~addr) & (uint8_t)(~0x7F)) {
        pressed_bits |= controls_pressed(CONTROL_SPECCY_SPACE, 0) << 0;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_SYM_SHIFT, 0) << 1;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_M, 0) << 2;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_N, 0) << 3;
        pressed_bits |= controls_pressed(CONTROL_SPECCY_B, 0) << 4;
    }

    u8 out = ~(pressed_bits | 0xE0);

    // Bit 6 is the EAR input bit from ULA
    out |= (ula & 0b10000) << 2;

    return out;
}