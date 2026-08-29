#include "chips/arm7tdmi/arm7tdmi.h"
#include "chips/arm7tdmi/arm7tdmi_util.h"

#include "types.h"

typedef void (*thumb_handler_t)(arm7tdmi_t*, u32);

static void thumb_pc_relative_load(arm7tdmi_t* cpu, u32 opcode);
static void thumb_add_subtract(arm7tdmi_t* cpu, u32 opcode);
static void thumb_move_shifted_register(arm7tdmi_t* cpu, u32 opcode);
static void thumb_conditional_branch(arm7tdmi_t* cpu, u32 opcode);
static void thumb_unconditional_branch(arm7tdmi_t* cpu, u32 opcode);
static void thumb_misc_immediate(arm7tdmi_t* cpu, u32 opcode);
static void thumb_long_branch_link(arm7tdmi_t* cpu, u32 opcode);
static void thumb_alu_operations(arm7tdmi_t* cpu, u32 opcode);
static void thumb_load_store_immediate_offset(arm7tdmi_t* cpu, u32 opcode);
static void thumb_multiple_load_store(arm7tdmi_t* cpu, u32 opcode);
static void thumb_hi_reg_op(arm7tdmi_t* cpu, u32 opcode);
static void thumb_push_pop(arm7tdmi_t* cpu, u32 opcode);
static void thumb_load_store_halfword(arm7tdmi_t* cpu, u32 opcode);
static void thumb_load_store_sign_extended(arm7tdmi_t* cpu, u32 opcode);
static void thumb_load_address(arm7tdmi_t* cpu, u32 opcode);
static void thumb_load_store_register_offset(arm7tdmi_t* cpu, u32 opcode);
static void thumb_add_offset_sp(arm7tdmi_t* cpu, u32 opcode);
static void thumb_sp_relative_load_store(arm7tdmi_t* cpu, u32 opcode);
static void thumb_software_interrupt(arm7tdmi_t* cpu, u32 opcode);
static void thumb_not_encoded(arm7tdmi_t* cpu, u32 opcode);

static thumb_handler_t thumb_decode_lut[1 << 10] = {
    thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_move_shifted_register, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_add_subtract, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_misc_immediate, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_alu_operations, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_hi_reg_op, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_pc_relative_load, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_register_offset, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_sign_extended, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_immediate_offset, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_load_store_halfword, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_sp_relative_load_store, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_load_address, thumb_add_offset_sp, thumb_add_offset_sp, thumb_add_offset_sp, thumb_add_offset_sp, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_push_pop, thumb_push_pop, thumb_push_pop, thumb_push_pop, thumb_push_pop, thumb_push_pop, thumb_push_pop, thumb_push_pop, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_push_pop, thumb_push_pop, thumb_push_pop, thumb_push_pop, thumb_push_pop, thumb_push_pop, thumb_push_pop, thumb_push_pop, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_multiple_load_store, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_conditional_branch, thumb_software_interrupt, thumb_software_interrupt, thumb_software_interrupt, thumb_software_interrupt, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_unconditional_branch, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_not_encoded, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link, thumb_long_branch_link
};

void thumb_step(arm7tdmi_t* cpu) {
    u16 opcode = cpu->pipeline_opcode[0];
    cpu->pipeline_opcode[0] = cpu->pipeline_opcode[1];
    cpu->r[15] += 2;
    cpu->pipeline_opcode[1] = readHalfWordAndTick(cpu, cpu->r[15], cpu->fetch_seq);
    cpu->fetch_seq = true;

    thumb_decode_lut[opcode >> 6](cpu, opcode);
}

static void thumb_move_shifted_register(arm7tdmi_t* cpu, u32 opcode) {
    u32* rd = &cpu->r[opcode & 0b111];
    u32 rs = cpu->r[(opcode >> 3) & 0b111];
    u32 off5 = (opcode >> 6) & 0b11111;
    u32 op = (opcode >> 11) & 0b11;

    switch (op) {
        case 0:
        alu_MOV(cpu, rd, 0, alu_LSL(cpu, rs, off5, true), true);
        break;
        
        case 1:
        alu_MOV(cpu, rd, 0, alu_LSR(cpu, rs, off5 ? off5 : 32, true), true);
        break;
        
        case 2:
        alu_MOV(cpu, rd, 0, alu_ASR(cpu, rs, off5 ? off5 : 32, true), true);
        break;
        
        default:
        thumb_not_encoded(cpu, opcode);
        break;
    }
}

static void thumb_pc_relative_load(arm7tdmi_t* cpu, u32 opcode) {
    u8 w8 = opcode & 0xFF;
    u32* rd = &cpu->r[(opcode >> 8) & 0b111];
    u32 addr = (cpu->r[15] & 0xFFFFFFFC) + (w8 << 2);

    *rd = readWordAndTick(cpu, addr, false);
    cpu->cycles += I_CYCLES;
    cpu->fetch_seq = false;
}

static void thumb_conditional_branch(arm7tdmi_t* cpu, u32 opcode) {
    u8 w8 = opcode & 0xFF;

    if (!condFuncs[(opcode >> 8) & 0x0F](cpu))
        return;

    cpu->r[15] += (w8 & 0x80) ? (u32)((i32)(i8)w8 << 1) : ((u32)w8 << 1);

    thumb_pipeline_refill(cpu);
}

static void thumb_unconditional_branch(arm7tdmi_t* cpu, u32 opcode) {
    u32 offset = (opcode & 0b11111111111) << 1;

    cpu->r[15] += (offset & 0x800) ? (0xFFFFF000 | offset) : offset;

    thumb_pipeline_refill(cpu);
}

static void thumb_misc_immediate(arm7tdmi_t* cpu, u32 opcode) {
    u8 w8 = opcode & 0xFF;
    u32* rd = &cpu->r[(opcode >> 8) & 0b111];

    switch ((opcode >> 11) & 0b11) {
        case 0b00:
        alu_MOV(cpu, rd, *rd, w8, true);
        break;
        
        case 0b01:
        alu_CMP(cpu, rd, *rd, w8, true);
        break;
        
        case 0b10:
        alu_ADD(cpu, rd, *rd, w8, true);
        break;
        
        case 0b11:
        alu_SUB(cpu, rd, *rd, w8, true);
        break;
    }
}

static void thumb_long_branch_link(arm7tdmi_t* cpu, u32 opcode) {
    u32 offset = opcode & 0b11111111111;

    if (!((opcode >> 11) & 1)) {
        offset <<= 1;
        if (offset & 0x800)
            offset |= 0xFFFFF000;
        cpu->r[14] = cpu->r[15] + (offset << 11);
    } else {
        u32 old_r15 = cpu->r[15];
        cpu->r[15] = cpu->r[14] + (offset << 1);
        cpu->r[15] &= 0xFFFFFFFE;
        cpu->r[14] = (old_r15 - 2) | 1;
        thumb_pipeline_refill(cpu);
    }
}

static void thumb_alu_operations(arm7tdmi_t* cpu, u32 opcode) {
    u32* rd = &cpu->r[opcode & 0b111];
    u32 rs = cpu->r[(opcode >> 3) & 0b111];

    switch ((opcode >> 6) & 0xF) {
        case 0:
        alu_AND(cpu, rd, *rd, rs, true);
        break;
        
        case 1:
        alu_EOR(cpu, rd, *rd, rs, true);
        break;
    
        case 2:
        cpu->cycles += I_CYCLES;
        cpu->fetch_seq = false;
        alu_MOV(cpu, rd, 0, alu_LSL(cpu, *rd, rs, true), true);
        break;
    
        case 3:
        cpu->cycles += I_CYCLES;
        cpu->fetch_seq = false;
        alu_MOV(cpu, rd, 0, alu_LSR(cpu, *rd, rs, true), true);
        break;
    
        case 4:
        cpu->cycles += I_CYCLES;
        cpu->fetch_seq = false;
        alu_MOV(cpu, rd, 0, alu_ASR(cpu, *rd, rs, true), true);
        break;
    
        case 5:
        alu_ADC(cpu, rd, *rd, rs, true);
        break;
    
        case 6:
        alu_SBC(cpu, rd, *rd, rs, true);
        break;
    
        case 7:
        cpu->cycles += I_CYCLES;
        cpu->fetch_seq = false;
        alu_MOV(cpu, rd, 0, alu_ROR(cpu, *rd, rs, true), true);
        break;
    
        case 8:
        alu_TST(cpu, rd, *rd, rs, true);
        break;
    
        case 9:
        alu_SUB(cpu, rd, 0, rs, true);
        break;
    
        case 10:
        alu_CMP(cpu, rd, *rd, rs, true);
        break;
    
        case 11:
        alu_CMN(cpu, rd, *rd, rs, true);
        break;
    
        case 12:
        alu_ORR(cpu, rd, *rd, rs, true);
        break;
    
        case 13: 
        {
            u32 old_rd = *rd;
            *rd = ((i32)*rd) * ((i32)rs);
            cpu->Z_FLAG = !(*rd);
            cpu->N_FLAG = *rd >> 31;

            if (!(old_rd & 0xFFFFFF00) || !((~old_rd) & 0xFFFFFF00))
                cpu->cycles += 1;
            else if (!(old_rd & 0xFFFF0000) || !((~old_rd) & 0xFFFF0000))
                cpu->cycles += 2;
            else if (!(old_rd & 0xFF000000) || !((~old_rd) & 0xFF000000))
                cpu->cycles += 3;
            else
                cpu->cycles += 4;

            cpu->fetch_seq = false;
        }
        break;
    
        case 14:
        alu_BIC(cpu, rd, *rd, rs, true);
        break;
        
        case 15:
        alu_MVN(cpu, rd, *rd, rs, true);
        break;
    }
}

static void thumb_load_store_register_offset(arm7tdmi_t* cpu, u32 opcode) {
    u32 ro = cpu->r[(opcode >> 6) & 0b111];
    u32 rb = cpu->r[(opcode >> 3) & 0b111];
    u32* rd = &cpu->r[opcode & 0b111];
    u32 addr = rb + ro;
    bool l = (opcode >> 11) & 1;
    bool b = (opcode >> 10) & 1;

    if (l) {
        if (b) {
            *rd = readByteAndTick(cpu, addr, false);
        } else {
            *rd = readWordAndTick(cpu, addr, false);
            *rd = alu_ROR(cpu, *rd, (addr & 0b11) << 3, false);
        }
        cpu->cycles += I_CYCLES;
    } else {
        if (b)
            writeByteAndTick(cpu, addr, *rd, false);
        else
            writeWordAndTick(cpu, addr, *rd, false);
    }

    cpu->fetch_seq = false;
}

static void thumb_load_store_sign_extended(arm7tdmi_t* cpu, u32 opcode) {
    u32 ro = cpu->r[(opcode >> 6) & 0b111];
    u32 rb = cpu->r[(opcode >> 3) & 0b111];
    u32* rd = &cpu->r[opcode & 0b111];
    u32 addr = rb + ro;
    bool h = (opcode >> 11) & 1;
    bool s = (opcode >> 10) & 1;

    if (!h) {
        if (!s) {
            writeHalfWordAndTick(cpu, addr, *rd, false);
        } else {
            *rd = readByteAndTick(cpu, addr, false);
            if (*rd & 0x80)
                *rd |= 0xFFFFFF00;
            cpu->cycles += I_CYCLES;
        }
    } else {
        cpu->cycles += I_CYCLES;
        cpu->fetch_seq = false;

        if (addr & 1) {
            if (!s) {
                *rd = readHalfWordAndTick(cpu, addr, false);
                *rd = (*rd >> 8) | (*rd << 24);
            } else {
                *rd = readByteAndTick(cpu, addr, false);
                if (*rd & 0x80)
                    *rd |= 0xFFFFFF00;
            }
        } else {
            *rd = readHalfWordAndTick(cpu, addr, false);
            if (s) {
                if (*rd & 0xFFFF8000)
                    *rd |= 0xFFFF0000;
            }
        }
    }
}

static void thumb_hi_reg_op(arm7tdmi_t* cpu, u32 opcode) {
    bool h1 = (opcode >> 7) & 1;
    bool h2 = (opcode >> 6) & 1;
    u32 op = (opcode >> 8) & 0b11;
    u8 rd_idx = (opcode & 0b111) + (h1 ? 8 : 0);
    u8 rs_idx = ((opcode >> 3) & 0b111) + (h2 ? 8 : 0);

    switch (op) {
        
        case 0b00:
        {
            u32* rd = &cpu->r[rd_idx];
            u32 rs = cpu->r[rs_idx];
            alu_ADD(cpu, rd, *rd, rs, false);
            if (rd_idx == 15) {
                cpu->r[15] &= 0xFFFFFFFE;
                thumb_pipeline_refill(cpu);
            }
        }
        break;
        
        case 0b01:
        {
            u32* rd = &cpu->r[rd_idx];
            u32 rs = cpu->r[rs_idx];
            alu_CMP(cpu, rd, *rd, rs, true);
        }
        break;
        
        case 0b10:
        {
            u32* rd = &cpu->r[rd_idx];
            u32 rs = cpu->r[rs_idx];
            alu_MOV(cpu, rd, *rd, rs, false);
            if (rd_idx == 15) {
                cpu->r[15] &= 0xFFFFFFFE;
                thumb_pipeline_refill(cpu);
            }
        }
        break;
        
        case 0b11:
        {
            u32 rs = cpu->r[rs_idx];
            cpu->r[15] = rs;
            cpu->thumb_mode = cpu->r[15] & 1;
            cpu->r[15] &= 0xFFFFFFFE;
            arm7tdmi_pipeline_refill(cpu);
        }
        break;
    }
}

static void thumb_load_store_immediate_offset(arm7tdmi_t* cpu, u32 opcode) {
    bool b = (opcode >> 12) & 1;
    bool l = (opcode >> 11) & 1;
    u32 off5 = (opcode >> 6) & 0b11111;
    u32 rb = cpu->r[(opcode >> 3) & 0b111];
    u32* rd = &cpu->r[opcode & 0b111];

    if (l) {
        if (b) {
            *rd = readByteAndTick(cpu, rb + off5, false);
        } else {
            u32 addr = rb + (off5 << 2);
            *rd = readWordAndTick(cpu, addr, false);
            *rd = alu_ROR(cpu, *rd, (addr & 0b11) << 3, false);
        }
        cpu->cycles += I_CYCLES;
    } else {
        if (b)
            writeByteAndTick(cpu, rb + off5, *rd, false);
        else
            writeWordAndTick(cpu, rb + (off5 << 2), *rd, false);
    }

    cpu->fetch_seq = false;
}

static void thumb_multiple_load_store(arm7tdmi_t* cpu, u32 opcode) {
    u32 base_idx = (opcode >> 8) & 0b111;
    u32 base = cpu->r[base_idx];
    u8 rlist = opcode & 0xFF;
    bool l = (opcode >> 11) & 1;

    cpu->fetch_seq = false;

    if (!rlist) {
        if (l) {
            cpu->r[15] = readWordAndTick(cpu, base, false);
            thumb_pipeline_refill(cpu);
        } else {
            writeWordAndTick(cpu, base, cpu->r[15] + 2, false);
        }
        cpu->r[base_idx] += 0x40;
        return;
    }

    u8 rlist_size = __builtin_popcount(rlist);
    bool first_transfer = true;

    for (u8 reg = 0; rlist; ++reg) {
        bool should_transfer = rlist & 1;
        rlist >>= 1;

        if (!should_transfer)
            continue;

        if (l) {
            cpu->r[reg] = readWordAndTick(cpu, base, !first_transfer);
        } else {
            if (reg == base_idx && !first_transfer)
                writeWordAndTick(cpu, base, cpu->r[reg] + (rlist_size << 2), true);
            else
                writeWordAndTick(cpu, base, cpu->r[reg], true);
        }

        base += 4;
        first_transfer = false;
    }

    if (l)
        cpu->cycles += I_CYCLES;

    cpu->r[base_idx] = base;
}

static void thumb_push_pop(arm7tdmi_t* cpu, u32 opcode) {
    bool l = (opcode >> 11) & 1;
    bool r = (opcode >> 8) & 1;
    u8 rlist = opcode & 0xFF;
    u8 count = __builtin_popcount(rlist);

    if (!l) {
        cpu->r[13] -= (count + r) * 4;
    } else {
        cpu->cycles += I_CYCLES;
    }

    cpu->fetch_seq = false;

    for (int i = 0; rlist; ++i) {
        bool should_transfer = rlist & 1;
        rlist >>= 1;

        if (!should_transfer)
            continue;

        if (l)
            cpu->r[i] = readWordAndTick(cpu, cpu->r[13], false);
        else
            writeWordAndTick(cpu, cpu->r[13], cpu->r[i], false);

        cpu->r[13] += 4;
    }

    if (r) {
        if (!l) {
            writeWordAndTick(cpu, cpu->r[13], cpu->r[14], false);
        } else {
            cpu->r[15] = readWordAndTick(cpu, cpu->r[13], false);
            cpu->r[15] &= 0xFFFFFFFE;
            thumb_pipeline_refill(cpu);
        }
        cpu->r[13] += 4;
    }

    if (!l)
        cpu->r[13] -= (count + r) * 4;
}

static void thumb_load_store_halfword(arm7tdmi_t* cpu, u32 opcode) {
    bool l = (opcode >> 11) & 1;
    u32 off5 = ((opcode >> 6) & 0b11111) << 1;
    u32* rd = &cpu->r[opcode & 0b111];
    u32 rb = cpu->r[(opcode >> 3) & 0b111];

    if (l) {
        *rd = readHalfWordAndTick(cpu, rb + off5, false);
        if (rb & 1)
            *rd = (*rd << 24) | (*rd >> 8);
        cpu->cycles += I_CYCLES;
    } else {
        writeHalfWordAndTick(cpu, rb + off5, *rd, false);
    }

    cpu->fetch_seq = false;
}

static void thumb_load_address(arm7tdmi_t* cpu, u32 opcode) {
    bool sp = (opcode >> 11) & 1;
    u8 w8 = opcode & 0xFF;
    u32* rd = &cpu->r[(opcode >> 8) & 0b111];

    if (sp)
        *rd = cpu->r[13];
    else
        *rd = cpu->r[15] & 0xFFFFFFFC;

    *rd += w8 << 2;
}

static void thumb_add_offset_sp(arm7tdmi_t* cpu, u32 opcode) {
    u16 w9 = (opcode & 0b1111111) << 2;

    if ((opcode >> 7) & 1)
        cpu->r[13] -= w9;
    else
        cpu->r[13] += w9;
}

static void thumb_sp_relative_load_store(arm7tdmi_t* cpu, u32 opcode) {
    bool l = (opcode >> 11) & 1;
    u32* rd = &cpu->r[(opcode >> 8) & 0b111];
    u8 w8 = opcode & 0xFF;
    u32 addr = cpu->r[13] + (w8 << 2);

    if (l) {
        *rd = readWordAndTick(cpu, addr, false);
        *rd = alu_ROR(cpu, *rd, (addr & 0b11) << 3, false);
        cpu->cycles += I_CYCLES;
    } else {
        writeWordAndTick(cpu, addr, *rd, false);
    }

    cpu->fetch_seq = false;
}

static void thumb_software_interrupt(arm7tdmi_t* cpu, u32 opcode) {
    (void)opcode;
    arm7tdmi_trigger_exception(cpu, 0x8, 0x13);
}

static void thumb_add_subtract(arm7tdmi_t* cpu, u32 opcode) {
    bool i = (opcode >> 10) & 1;
    bool op = (opcode >> 9) & 1;
    u32 off3 = (opcode >> 6) & 0b111;
    u32* rd = &cpu->r[opcode & 0b111];
    u32 rs = cpu->r[(opcode >> 3) & 0b111];

    if (op) {
        if (i)
            alu_SUB(cpu, rd, rs, off3, true);
        else
            alu_SUB(cpu, rd, rs, cpu->r[off3], true);
    } else {
        if (i)
            alu_ADD(cpu, rd, rs, off3, true);
        else
            alu_ADD(cpu, rd, rs, cpu->r[off3], true);
    }
}

static void thumb_not_encoded(arm7tdmi_t* cpu, u32 opcode) {
    printf("Illegal/unimplemented Thumb opcode: %08X\n", opcode);
}
