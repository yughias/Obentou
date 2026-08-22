#include <stdio.h>

int main() {
    for (int high = 0; high < 256; high++) {
        for (int low = 0; low < 16; low++) {
            unsigned opcode = (high << 20) | (low << 4);
            unsigned lut_index = (high << 4) | low;

            if (((opcode >> 24) & 0xF) == 0b1111) {
                printf("arm_software_interrupt, ");
            } else if (!((opcode >> 22) & 0b111111) && (((opcode >> 4) & 0xF) == 0b1001)) {
                printf("arm_multiply, ");
            } else if ((((opcode >> 23) & 0b11111) == 0b00001) && (((opcode >> 4) & 0xF) == 0b1001)) {
                printf("arm_multiply_long, ");
            } else if (!(opcode & 0xFFF) && (((opcode >> 20) & 0b11) == 0b00) && (((opcode >> 23) & 0b11111) == 0b00010)) {
                printf("arm_mrs, ");
            } else if ((((opcode >> 23) & 0b11111) == 0b00110) && (((opcode >> 20) & 0b11) == 0b10)) {
                printf("arm_msr, ");
            } else if ((((opcode >> 23) & 0b11111) == 0b00010) && (((opcode >> 20) & 0b11) == 0b10) && !((opcode >> 4) & 0xF)) {
                printf("arm_msr, ");
            } else if ((((opcode >> 20) & 0xFF) == 0b00010010) && (((opcode >> 4) & 0xF) == 0b0001)) {
                printf("arm_bx, ");
            } else if (((opcode >> 25) & 0b111) == 0b101) {
                printf("arm_b_bl, ");
            } else if (((opcode >> 26) & 0b11) == 0b01) {
                printf("arm_single_data_transfer, ");
            } else if (!((opcode >> 25) & 0b111) && ((opcode >> 7) & 1) && ((opcode >> 4) & 1)) {
                printf("arm_halfword_data_transfer, ");
            } else if (((opcode >> 26) & 0b11) == 0b00) {
                printf("arm_data_processing, ");
            } else if (((opcode >> 25) & 0b111) == 0b100) {
                printf("arm_block_data_transfer, ");
            } else {
                printf("arm_not_decoded, ");
            }
        }
    }

    return 0;
}