#include <stdio.h>

int main() {
    for (unsigned int i = 0; i < (1 << 10); ++i) {
        unsigned int opcode = i << 6;

        if ((opcode >> 8) == 0b11011111) {
            printf("thumb_software_interrupt, ");
        } else if ((opcode >> 11) == 0b00011) {
            printf("thumb_add_subtract, ");
        } else if (!(opcode >> 13)) {
            printf("thumb_move_shifted_register, ");
        } else if ((opcode >> 13) == 0b001) {
            printf("thumb_misc_immediate, ");
        } else if ((opcode >> 10) == 0b010000) {
            printf("thumb_alu_operations, ");
        } else if ((opcode >> 12) == 0b0101 && !((opcode >> 9) & 1)) {
            printf("thumb_load_store_register_offset, ");
        } else if ((opcode >> 12) == 0b0101 && ((opcode >> 9) & 1)) {
            printf("thumb_load_store_sign_extended, ");
        } else if ((opcode >> 10) == 0b010001) {
            printf("thumb_hi_reg_op, ");
        } else if ((opcode >> 11) == 0b01001) {
            printf("thumb_pc_relative_load, ");
        } else if ((opcode >> 13) == 0b011) {
            printf("thumb_load_store_immediate_offset, ");
        } else if ((opcode >> 12) == 0b1001) {
            printf("thumb_sp_relative_load_store, ");
        } else if ((opcode >> 12) == 0b1000) {
            printf("thumb_load_store_halfword, ");
        } else if ((opcode >> 12) == 0b1010) {
            printf("thumb_load_address, ");
        } else if ((opcode >> 8) == 0b10110000) {
            printf("thumb_add_offset_sp, ");
        } else if ((opcode >> 12) == 0b1011 && (((opcode >> 9) & 0b11) == 0b10)) {
            printf("thumb_push_pop, ");
        } else if ((opcode >> 12) == 0b1100) {
            printf("thumb_multiple_load_store, ");
        } else if ((opcode >> 12) == 0b1101) {
            printf("thumb_conditional_branch, ");
        } else if ((opcode >> 11) == 0b11100) {
            printf("thumb_unconditional_branch, ");
        } else if ((opcode >> 12) == 0b1111) {
            printf("thumb_long_branch_link, ");
        } else {
            printf("thumb_not_encoded, ");
        }
    }

    return 0;
}