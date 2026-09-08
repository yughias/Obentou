#include "cores/gbc/joypad.h"
#include "cores/gbc/gb.h"

#include "utils/controls.h"

u8 gb_read_joyp(gb_t* gb) {
    control_t controls[8] = {
        CONTROL_GBC_A, CONTROL_GBC_B, CONTROL_GBC_SELECT, CONTROL_GBC_START,
        CONTROL_GBC_RIGHT, CONTROL_GBC_LEFT, CONTROL_GBC_UP, CONTROL_GBC_DOWN
    };

    u8 chosen = (gb->JOYP_REG >> 4) & 0b11;
    bool arrow_btn = !(chosen & 0b1);
    bool action_btn = !(chosen & 0b10);

    u8 output_val = gb->JOYP_REG & 0b110000;
    output_val |= 0b11001111;

    bool sgb_multiplayer = gb->console_type == SGB_TYPE && gb->sgb.multiplayer_enabled;
    int port = sgb_multiplayer ? gb->sgb.player_count : 0;

    if (sgb_multiplayer && (chosen == 0b11)) {
        output_val &= ~0x0F;
        return output_val | (0x0F - gb->sgb.player_count);
    }

    for (int i = 0; i < 4; i++) {
        if (action_btn && controls_pressed(controls[i], port))
            output_val &= ~(1 << i);

        if (arrow_btn && controls_pressed(controls[i + 4], port))
            output_val &= ~(1 << i);
    }

    return output_val;
}

void gb_write_joyp(gb_t* gb, u8 byte) {
    bool prev_p15 = gb->JOYP_REG & (1 << 5);
    gb->JOYP_REG = byte;

    if(!prev_p15 && gb->JOYP_REG & (1 << 5))
        gb->sgb.player_count = (gb->sgb.player_count + 1) & (gb->sgb.multiplayer_mode - 1);

    if(gb->console_type == SGB_TYPE)
        sgb_write(&gb->sgb, byte & (1 << 4), byte & (1 << 5));
}