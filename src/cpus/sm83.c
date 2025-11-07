#include "cpus/sm83.h"

#include "utils/serializer.h"

#include <stdio.h>

//cpu NEW INSTRUCTION
static inline void STOP(sm83_t*);
static inline void JR(sm83_t*, int8_t);
static inline void JRNZ(sm83_t*, int8_t);
static inline void JRZ(sm83_t*, int8_t);
static inline void JRNC(sm83_t*, int8_t);
static inline void JRC(sm83_t*, int8_t);
static inline void ADD_16(sm83_t*, u16*, u16);
static inline void INC_16(sm83_t*, u16*);
static inline void DEC_16(sm83_t*, u16*);
static inline void INC_8(sm83_t*, u8*);
static inline void DEC_8(sm83_t*, u8*);
static inline void DAA(sm83_t*);
static inline void CPL(sm83_t*);
static inline void SCF(sm83_t*);
static inline void CCF(sm83_t*);
static inline void HLT(sm83_t*);
static inline void RETNZ(sm83_t*);
static inline void RETZ(sm83_t*);
static inline void RETNC(sm83_t*);
static inline void RETC(sm83_t*);
static inline void LDH1(sm83_t*, u8);
static inline void LDH2(sm83_t*, u8);
static inline void ADD_SP(sm83_t*, int8_t);
static inline void LD_SP(sm83_t*, int8_t);
static inline void POP(sm83_t*, u16*);
static inline void RET(sm83_t*);
static inline void JP(sm83_t*, u16);
static inline void JPNZ(sm83_t*, u16);
static inline void JPZ(sm83_t*, u16);
static inline void JPNC(sm83_t*, u16);
static inline void JPC(sm83_t*, u16);
static inline void EI(sm83_t*);
static inline void DI(sm83_t*);
static inline void CALL(sm83_t*, u16);
static inline void CALLNZ(sm83_t*, u16);
static inline void CALLZ(sm83_t*, u16);
static inline void CALLNC(sm83_t*, u16);
static inline void CALLC(sm83_t*, u16);
static inline void PUSH(sm83_t*, u16);
static inline void RST(sm83_t*, u8);
static inline void RLC(sm83_t*, u8*);
static inline void RRC(sm83_t*, u8*);
static inline void RLCA(sm83_t*);
static inline void RRCA(sm83_t*);
static inline void RLA(sm83_t*);
static inline void RRA(sm83_t*);
static inline void RLC(sm83_t*, u8*);
static inline void RRC(sm83_t*, u8*);
static inline void RL(sm83_t*, u8*);
static inline void RR(sm83_t*, u8*);
static inline void SLA(sm83_t*, u8*);
static inline void SRA(sm83_t*, u8*);
static inline void SWAP(sm83_t*, u8*);
static inline void SRL(sm83_t*, u8*);
static inline void BIT(sm83_t*, u8, u8*);
static inline void SET(sm83_t*, u8, u8*);
static inline void RES(sm83_t*, u8, u8*);
static inline void ADD(sm83_t*, u8*, u8);
static inline void ADC(sm83_t*, u8*, u8);
static inline void SUB(sm83_t*, u8*, u8);
static inline void SBC(sm83_t*, u8*, u8);
static inline void AND(sm83_t*, u8*, u8);
static inline void XOR(sm83_t*, u8*, u8);
static inline void OR(sm83_t*, u8*, u8);
static inline void CP(sm83_t*, u8*, u8);
static inline void RETI(sm83_t*);

// flag masks to set/clear registers
#define SET_Z   0b10000000
#define SET_N   0b01000000
#define SET_H   0b00100000
#define SET_C   0b00010000

#define CLEAR_Z 0b01111111
#define CLEAR_N 0b10111111
#define CLEAR_H 0b11011111
#define CLEAR_C 0b11101111

#define pc_inc(x) cpu->PC += x
#define ld_r16_im(r)  pc_inc(2); r = readShortAndTick(cpu, cpu->PC - 2); 
#define ld_ind_r(r1, r2) writeByteAndTick(cpu, r1, r2); 
#define ld_r_ind(r1, r2) r1 = readByteAndTick(cpu, r2); 
#define ld_ind_r_inc(r1, r2) writeByteAndTick(cpu, r1, r2); r1 += 1; 
#define ld_r_ind_inc(r1, r2) r1 = readByteAndTick(cpu, r2); r2 += 1; 
#define ld_ind_r_dec(r1, r2) writeByteAndTick(cpu, r1, r2); r1 -= 1; 
#define ld_r_ind_dec(r1, r2) r1 = readByteAndTick(cpu, r2); r2 -= 1; 
#define inc8(r) INC_8(cpu, &r); 
#define inc16(r) cpu->tickSystem(cpu->ctx, 4); INC_16(cpu, &r); 
#define dec8(r) DEC_8(cpu, &r); 
#define dec16(r) cpu->tickSystem(cpu->ctx, 4); DEC_16(cpu, &r); 
#define ld_rr(r1, r2) r1 = r2; 
#define ld_r8_im(r) pc_inc(1); r = readByteAndTick(cpu, cpu->PC - 1); 
#define add16(r1, r2) cpu->tickSystem(cpu->ctx, 4); ADD_16(cpu, &r1, r2);
#define jr(CC) pc_inc(1); JR ## CC (cpu, readByteAndTick(cpu, cpu->PC - 1));
#define inc8_ind(r) { uint8_t tmp = readByteAndTick(cpu, r); INC_8(cpu, &tmp); writeByteAndTick(cpu, r, tmp); } 
#define dec8_ind(r) { uint8_t tmp = readByteAndTick(cpu, r); DEC_8(cpu, &tmp); writeByteAndTick(cpu, r, tmp); } 
#define ld_ind_im(r) pc_inc(1); writeByteAndTick(cpu, r, readByteAndTick(cpu, cpu->PC - 1));
#define alu_rr(fun, r1, r2) fun(cpu, &r1, r2);  
#define alu_hl(fun) fun(cpu, &cpu->A, readByteAndTick(cpu, cpu->HL));   
#define alu_im(fun) pc_inc(1); fun(cpu, &cpu->A, readByteAndTick(cpu, cpu->PC - 1));   
#define ret(CC) cpu->tickSystem(cpu->ctx, 4); RET ## CC (cpu); 
#define pop(r) POP(cpu, &r); 
#define push(r) cpu->tickSystem(cpu->ctx, 4); PUSH(cpu, r);
#define jp(CC) pc_inc(2); JP ## CC (cpu, readShortAndTick(cpu, cpu->PC - 2)); 
#define call(CC) pc_inc(2); CALL ## CC (cpu, readShortAndTick(cpu, cpu->PC - 2)); 
#define rst(idx) cpu->tickSystem(cpu->ctx, 4); RST(cpu, idx); 
#define ld_u16_A() pc_inc(2); writeByteAndTick(cpu, readShortAndTick(cpu, cpu->PC - 2), cpu->A); 
#define ld_A_u16() pc_inc(2); cpu->A = readByteAndTick(cpu, readShortAndTick(cpu, cpu->PC - 2)); 

#define cb_no_bit(op, r) op (cpu, &r); 
#define cb_no_bit_hl(op) { uint8_t tmp = readByteAndTick(cpu, cpu->HL); op (cpu, &tmp); writeByteAndTick(cpu, cpu->HL, tmp); } 
#define cb_bit(op, bit, r) op (cpu, bit, &r); 
#define cb_bit_hl(op, bit) { uint8_t tmp = readByteAndTick(cpu, cpu->HL); op (cpu, bit, &tmp); writeByteAndTick(cpu, cpu->HL, tmp); } 
#define cb_bit_hl_fast(op, bit) { uint8_t tmp = readByteAndTick(cpu, cpu->HL); op (cpu, bit, &tmp); } 


// cpu utility function
static inline bool calculateCarry(int, u16, u16, bool);
static inline void prefix_CB(sm83_t*, u8 opcode);

static inline u8 readByteAndTick(sm83_t*, u16);
static inline u16 readShortAndTick(sm83_t*, u16);
static inline void writeByteAndTick(sm83_t*, u16, u8);
static inline void writeShortAndTick(sm83_t*, u16, u16);

void initCPU(sm83_t* cpu){
    cpu->cycles = 0;

    cpu->AF = 0x0000;
    cpu->BC = 0x0000;
    cpu->DE = 0x0000;
    cpu->HL = 0x0000;
    cpu->SP = 0x0000;
    cpu->PC = 0x0000;

    cpu->HALTED = false;
    cpu->HALT_BUG = false;
    cpu->IME = false;
    cpu->EI_DELAY = false;
    cpu->IE = 0x00;
    cpu->IF = 0x00;
}

void infoCPU(sm83_t* cpu){
    fprintf(stderr, "A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X ",
            cpu->A, cpu->F, cpu->B, cpu->C, cpu->D, cpu->E, cpu->H, cpu->L);

    fprintf(stderr, "SP: %04X PC: 00:%04X (%02X %02X %02X %02X)\n",
            cpu->SP, cpu->PC, cpu->readByte(cpu->ctx, cpu->PC), cpu->readByte(cpu->ctx, cpu->PC+1), cpu->readByte(cpu->ctx, cpu->PC+2), cpu->readByte(cpu->ctx, cpu->PC+3));
}

static inline void dispatchInterrupt(sm83_t* cpu){
    cpu->PC -= 1;
    cpu->tickSystem(cpu->ctx, 4);

    cpu->SP -= 1;
    writeByteAndTick(cpu, cpu->SP, cpu->PC >> 8);

    u8 ie_flag = cpu->IE;

    cpu->SP -= 1;
    writeByteAndTick(cpu, cpu->SP, cpu->PC & 0xFF);
    
    cpu->tickSystem(cpu->ctx, 4);
    u8 mask = ie_flag & cpu->IF;
    u16 jmp_addr = 0;
    for(int i = 0; i < 5; i++){
        bool bit = mask & 1;
        mask >>= 1;
        if(bit){
            cpu->IF &= ~(u8)(1 << i);
            jmp_addr = 0x40 + 0x08*i;
            break;
        }
    }
    JP(cpu, jmp_addr);
}

void stepCPU(sm83_t* cpu){
    u8 opcode = readByteAndTick(cpu, cpu->PC);

    if(cpu->HALT_BUG)
        cpu->HALT_BUG = false;
    else
        pc_inc(1);

    if(cpu->IE & cpu->IF & 0x1F){
        cpu->HALTED = false;
        if(cpu->IME && !cpu->EI_DELAY){
            cpu->IME = false;
            dispatchInterrupt(cpu);
            return;
        }
    }

    if(cpu->EI_DELAY)
        cpu->EI_DELAY = false;

    if(cpu->HALTED){
        cpu->PC -= 1;
        return;
    }

    static void* opcode_table[256] = {
        &&op_00, &&op_01, &&op_02, &&op_03, &&op_04, &&op_05, &&op_06, &&op_07, &&op_08, &&op_09, &&op_0A, &&op_0B, &&op_0C, &&op_0D, &&op_0E, &&op_0F,
        &&op_10, &&op_11, &&op_12, &&op_13, &&op_14, &&op_15, &&op_16, &&op_17, &&op_18, &&op_19, &&op_1A, &&op_1B, &&op_1C, &&op_1D, &&op_1E, &&op_1F,
        &&op_20, &&op_21, &&op_22, &&op_23, &&op_24, &&op_25, &&op_26, &&op_27, &&op_28, &&op_29, &&op_2A, &&op_2B, &&op_2C, &&op_2D, &&op_2E, &&op_2F,
        &&op_30, &&op_31, &&op_32, &&op_33, &&op_34, &&op_35, &&op_36, &&op_37, &&op_38, &&op_39, &&op_3A, &&op_3B, &&op_3C, &&op_3D, &&op_3E, &&op_3F,
        &&op_40, &&op_41, &&op_42, &&op_43, &&op_44, &&op_45, &&op_46, &&op_47, &&op_48, &&op_49, &&op_4A, &&op_4B, &&op_4C, &&op_4D, &&op_4E, &&op_4F,
        &&op_50, &&op_51, &&op_52, &&op_53, &&op_54, &&op_55, &&op_56, &&op_57, &&op_58, &&op_59, &&op_5A, &&op_5B, &&op_5C, &&op_5D, &&op_5E, &&op_5F,
        &&op_60, &&op_61, &&op_62, &&op_63, &&op_64, &&op_65, &&op_66, &&op_67, &&op_68, &&op_69, &&op_6A, &&op_6B, &&op_6C, &&op_6D, &&op_6E, &&op_6F,
        &&op_70, &&op_71, &&op_72, &&op_73, &&op_74, &&op_75, &&op_76, &&op_77, &&op_78, &&op_79, &&op_7A, &&op_7B, &&op_7C, &&op_7D, &&op_7E, &&op_7F,
        &&op_80, &&op_81, &&op_82, &&op_83, &&op_84, &&op_85, &&op_86, &&op_87, &&op_88, &&op_89, &&op_8A, &&op_8B, &&op_8C, &&op_8D, &&op_8E, &&op_8F,
        &&op_90, &&op_91, &&op_92, &&op_93, &&op_94, &&op_95, &&op_96, &&op_97, &&op_98, &&op_99, &&op_9A, &&op_9B, &&op_9C, &&op_9D, &&op_9E, &&op_9F,
        &&op_A0, &&op_A1, &&op_A2, &&op_A3, &&op_A4, &&op_A5, &&op_A6, &&op_A7, &&op_A8, &&op_A9, &&op_AA, &&op_AB, &&op_AC, &&op_AD, &&op_AE, &&op_AF,
        &&op_B0, &&op_B1, &&op_B2, &&op_B3, &&op_B4, &&op_B5, &&op_B6, &&op_B7, &&op_B8, &&op_B9, &&op_BA, &&op_BB, &&op_BC, &&op_BD, &&op_BE, &&op_BF,
        &&op_C0, &&op_C1, &&op_C2, &&op_C3, &&op_C4, &&op_C5, &&op_C6, &&op_C7, &&op_C8, &&op_C9, &&op_CA, &&op_CB, &&op_CC, &&op_CD, &&op_CE, &&op_CF,
        &&op_D0, &&op_D1, &&op_D2, &&op_D3, &&op_D4, &&op_D5, &&op_D6, &&op_D7, &&op_D8, &&op_D9, &&op_DA, &&op_DB, &&op_DC, &&op_DD, &&op_DE, &&op_DF,
        &&op_E0, &&op_E1, &&op_E2, &&op_E3, &&op_E4, &&op_E5, &&op_E6, &&op_E7, &&op_E8, &&op_E9, &&op_EA, &&op_EB, &&op_EC, &&op_ED, &&op_EE, &&op_EF,
        &&op_F0, &&op_F1, &&op_F2, &&op_F3, &&op_F4, &&op_F5, &&op_F6, &&op_F7, &&op_F8, &&op_F9, &&op_FA, &&op_FB, &&op_FC, &&op_FD, &&op_FE, &&op_FF,
    };

    goto *opcode_table[opcode];

    {
        op_00: return;
        op_01: ld_r16_im(cpu->BC); return;
        op_02: ld_ind_r(cpu->BC, cpu->A); return;
        op_03: inc16(cpu->BC); return;
        op_04: inc8(cpu->B); return;
        op_05: dec8(cpu->B); return;
        op_06: ld_r8_im(cpu->B); return;
        op_07: RLCA(cpu); return;
        op_08: pc_inc(2); writeShortAndTick(cpu, readShortAndTick(cpu, cpu->PC - 2), cpu->SP); return;
        op_09: add16(cpu->HL, cpu->BC); return;
        op_0A: ld_r_ind(cpu->A, cpu->BC); return;
        op_0B: dec16(cpu->BC); return;
        op_0C: inc8(cpu->C); return;
        op_0D: dec8(cpu->C); return;
        op_0E: ld_r8_im(cpu->C); return;
        op_0F: RRCA(cpu); return;

        op_10: pc_inc(1); STOP(cpu); return;
        op_11: ld_r16_im(cpu->DE); return;
        op_12: ld_ind_r(cpu->DE, cpu->A); return;
        op_13: inc16(cpu->DE); return;
        op_14: inc8(cpu->D); return;
        op_15: dec8(cpu->D); return;
        op_16: ld_r8_im(cpu->D); return;
        op_17: RLA(cpu); return;
        op_18: pc_inc(1); JR(cpu, readByteAndTick(cpu, cpu->PC - 1)); return;
        op_19: add16(cpu->HL, cpu->DE); return;
        op_1A: ld_r_ind(cpu->A, cpu->DE); return;
        op_1B: dec16(cpu->DE); return;
        op_1C: inc8(cpu->E); return;
        op_1D: dec8(cpu->E); return;
        op_1E: ld_r8_im(cpu->E); return;
        op_1F: RRA(cpu); return;

        op_20: jr(NZ); return;
        op_21: ld_r16_im(cpu->HL); return;
        op_22: ld_ind_r_inc(cpu->HL, cpu->A); return;
        op_23: inc16(cpu->HL); return;
        op_24: inc8(cpu->H); return;
        op_25: dec8(cpu->H); return;
        op_26: ld_r8_im(cpu->H); return;
        op_27: DAA(cpu); return;
        op_28: jr(Z); return;
        op_29: add16(cpu->HL, cpu->HL); return;
        op_2A: ld_r_ind_inc(cpu->A, cpu->HL); return;
        op_2B: dec16(cpu->HL); return;
        op_2C: inc8(cpu->L); return;
        op_2D: dec8(cpu->L); return;
        op_2E: ld_r8_im(cpu->L); return;
        op_2F: CPL(cpu); return;

        op_30: jr(NC); return;
        op_31: ld_r16_im(cpu->SP); return;
        op_32: ld_ind_r_dec(cpu->HL, cpu->A); return;
        op_33: inc16(cpu->SP); return;
        op_34: inc8_ind(cpu->HL); return;
        op_35: dec8_ind(cpu->HL); return;
        op_36: ld_ind_im(cpu->HL); return;
        op_37: SCF(cpu); return;
        op_38: jr(C); return;
        op_39: add16(cpu->HL, cpu->SP); return;
        op_3A: ld_r_ind_dec(cpu->A, cpu->HL); return;
        op_3B: dec16(cpu->SP); return;
        op_3C: inc8(cpu->A); return;
        op_3D: dec8(cpu->A); return;
        op_3E: ld_r8_im(cpu->A); return;
        op_3F: CCF(cpu); return;

        op_40: ld_rr(cpu->B, cpu->B); return;
        op_41: ld_rr(cpu->B, cpu->C); return;
        op_42: ld_rr(cpu->B, cpu->D); return;
        op_43: ld_rr(cpu->B, cpu->E); return;
        op_44: ld_rr(cpu->B, cpu->H); return;
        op_45: ld_rr(cpu->B, cpu->L); return;
        op_46: ld_r_ind(cpu->B, cpu->HL); return;
        op_47: ld_rr(cpu->B, cpu->A); return;
        op_48: ld_rr(cpu->C, cpu->B); return;
        op_49: ld_rr(cpu->C, cpu->C); return;
        op_4A: ld_rr(cpu->C, cpu->D); return;
        op_4B: ld_rr(cpu->C, cpu->E); return;
        op_4C: ld_rr(cpu->C, cpu->H); return;
        op_4D: ld_rr(cpu->C, cpu->L); return;
        op_4E: ld_r_ind(cpu->C, cpu->HL); return;
        op_4F: ld_rr(cpu->C, cpu->A); return;

        op_50: ld_rr(cpu->D, cpu->B); return;
        op_51: ld_rr(cpu->D, cpu->C); return;
        op_52: ld_rr(cpu->D, cpu->D); return;
        op_53: ld_rr(cpu->D, cpu->E); return;
        op_54: ld_rr(cpu->D, cpu->H); return;
        op_55: ld_rr(cpu->D, cpu->L); return;
        op_56: ld_r_ind(cpu->D, cpu->HL); return;
        op_57: ld_rr(cpu->D, cpu->A); return;
        op_58: ld_rr(cpu->E, cpu->B); return;
        op_59: ld_rr(cpu->E, cpu->C); return;
        op_5A: ld_rr(cpu->E, cpu->D); return;
        op_5B: ld_rr(cpu->E, cpu->E); return;
        op_5C: ld_rr(cpu->E, cpu->H); return;
        op_5D: ld_rr(cpu->E, cpu->L); return;
        op_5E: ld_r_ind(cpu->E, cpu->HL); return;
        op_5F: ld_rr(cpu->E, cpu->A); return;

        op_60: ld_rr(cpu->H, cpu->B); return;
        op_61: ld_rr(cpu->H, cpu->C); return;
        op_62: ld_rr(cpu->H, cpu->D); return;
        op_63: ld_rr(cpu->H, cpu->E); return;
        op_64: ld_rr(cpu->H, cpu->H); return;
        op_65: ld_rr(cpu->H, cpu->L); return;
        op_66: ld_r_ind(cpu->H, cpu->HL); return;
        op_67: ld_rr(cpu->H, cpu->A); return;
        op_68: ld_rr(cpu->L, cpu->B); return;
        op_69: ld_rr(cpu->L, cpu->C); return;
        op_6A: ld_rr(cpu->L, cpu->D); return;
        op_6B: ld_rr(cpu->L, cpu->E); return;
        op_6C: ld_rr(cpu->L, cpu->H); return;
        op_6D: ld_rr(cpu->L, cpu->L); return;
        op_6E: ld_r_ind(cpu->L, cpu->HL); return;
        op_6F: ld_rr(cpu->L, cpu->A); return;

        op_70: ld_ind_r(cpu->HL, cpu->B); return;
        op_71: ld_ind_r(cpu->HL, cpu->C); return;
        op_72: ld_ind_r(cpu->HL, cpu->D); return;
        op_73: ld_ind_r(cpu->HL, cpu->E); return;
        op_74: ld_ind_r(cpu->HL, cpu->H); return;
        op_75: ld_ind_r(cpu->HL, cpu->L); return;
        op_76: HLT(cpu); return;
        op_77: ld_ind_r(cpu->HL, cpu->A); return;
        op_78: ld_rr(cpu->A, cpu->B); return;
        op_79: ld_rr(cpu->A, cpu->C); return;
        op_7A: ld_rr(cpu->A, cpu->D); return;
        op_7B: ld_rr(cpu->A, cpu->E); return;
        op_7C: ld_rr(cpu->A, cpu->H); return;
        op_7D: ld_rr(cpu->A, cpu->L); return;
        op_7E: ld_r_ind(cpu->A, cpu->HL); return;
        op_7F: ld_rr(cpu->A, cpu->A); return;

        op_80: alu_rr(ADD, cpu->A, cpu->B); return;
        op_81: alu_rr(ADD, cpu->A, cpu->C); return;
        op_82: alu_rr(ADD, cpu->A, cpu->D); return;
        op_83: alu_rr(ADD, cpu->A, cpu->E); return;
        op_84: alu_rr(ADD, cpu->A, cpu->H); return;
        op_85: alu_rr(ADD, cpu->A, cpu->L); return;
        op_86: alu_hl(ADD); return;
        op_87: alu_rr(ADD, cpu->A, cpu->A); return;
        op_88: alu_rr(ADC, cpu->A, cpu->B); return;
        op_89: alu_rr(ADC, cpu->A, cpu->C); return;
        op_8A: alu_rr(ADC, cpu->A, cpu->D); return;
        op_8B: alu_rr(ADC, cpu->A, cpu->E); return;
        op_8C: alu_rr(ADC, cpu->A, cpu->H); return;
        op_8D: alu_rr(ADC, cpu->A, cpu->L); return;
        op_8E: alu_hl(ADC); return;
        op_8F: alu_rr(ADC, cpu->A, cpu->A); return;

        op_90: alu_rr(SUB, cpu->A, cpu->B); return;
        op_91: alu_rr(SUB, cpu->A, cpu->C); return;
        op_92: alu_rr(SUB, cpu->A, cpu->D); return;
        op_93: alu_rr(SUB, cpu->A, cpu->E); return;
        op_94: alu_rr(SUB, cpu->A, cpu->H); return;
        op_95: alu_rr(SUB, cpu->A, cpu->L); return;
        op_96: alu_hl(SUB); return;
        op_97: alu_rr(SUB, cpu->A, cpu->A); return;
        op_98: alu_rr(SBC, cpu->A, cpu->B); return;
        op_99: alu_rr(SBC, cpu->A, cpu->C); return;
        op_9A: alu_rr(SBC, cpu->A, cpu->D); return;
        op_9B: alu_rr(SBC, cpu->A, cpu->E); return;
        op_9C: alu_rr(SBC, cpu->A, cpu->H); return;
        op_9D: alu_rr(SBC, cpu->A, cpu->L); return;
        op_9E: alu_hl(SBC); return;
        op_9F: alu_rr(SBC, cpu->A, cpu->A); return;

        op_A0: alu_rr(AND, cpu->A, cpu->B); return;
        op_A1: alu_rr(AND, cpu->A, cpu->C); return;
        op_A2: alu_rr(AND, cpu->A, cpu->D); return;
        op_A3: alu_rr(AND, cpu->A, cpu->E); return;
        op_A4: alu_rr(AND, cpu->A, cpu->H); return;
        op_A5: alu_rr(AND, cpu->A, cpu->L); return;
        op_A6: alu_hl(AND); return;
        op_A7: alu_rr(AND, cpu->A, cpu->A); return;
        op_A8: alu_rr(XOR, cpu->A, cpu->B); return;
        op_A9: alu_rr(XOR, cpu->A, cpu->C); return;
        op_AA: alu_rr(XOR, cpu->A, cpu->D); return;
        op_AB: alu_rr(XOR, cpu->A, cpu->E); return;
        op_AC: alu_rr(XOR, cpu->A, cpu->H); return;
        op_AD: alu_rr(XOR, cpu->A, cpu->L); return;
        op_AE: alu_hl(XOR); return;
        op_AF: alu_rr(XOR, cpu->A, cpu->A); return;

        op_B0: alu_rr(OR, cpu->A, cpu->B); return;
        op_B1: alu_rr(OR, cpu->A, cpu->C); return;
        op_B2: alu_rr(OR, cpu->A, cpu->D); return;
        op_B3: alu_rr(OR, cpu->A, cpu->E); return;
        op_B4: alu_rr(OR, cpu->A, cpu->H); return;
        op_B5: alu_rr(OR, cpu->A, cpu->L); return;
        op_B6: alu_hl(OR); return;
        op_B7: alu_rr(OR, cpu->A, cpu->A); return;
        op_B8: alu_rr(CP, cpu->A, cpu->B); return;
        op_B9: alu_rr(CP, cpu->A, cpu->C); return;
        op_BA: alu_rr(CP, cpu->A, cpu->D); return;
        op_BB: alu_rr(CP, cpu->A, cpu->E); return;
        op_BC: alu_rr(CP, cpu->A, cpu->H); return;
        op_BD: alu_rr(CP, cpu->A, cpu->L); return;
        op_BE: alu_hl(CP); return;
        op_BF: alu_rr(CP, cpu->A, cpu->A); return;

        op_C0: ret(NZ); return;
        op_C1: pop(cpu->BC); return;
        op_C2: jp(NZ); return;
        op_C3: pc_inc(2); JP(cpu, readShortAndTick(cpu, cpu->PC - 2)); cpu->tickSystem(cpu->ctx, 4); return;
        op_C4: call(NZ); return;
        op_C5: push(cpu->BC); return;
        op_C6: alu_im(ADD); return;
        op_C7: rst(0x00); return;
        op_C8: ret(Z); return;
        op_C9: RET(cpu); return;
        op_CA: jp(Z); return;
        op_CB: pc_inc(1); prefix_CB(cpu, readByteAndTick(cpu, cpu->PC - 1)); return;
        op_CC: call(Z); return;
        op_CD: pc_inc(2); cpu->tickSystem(cpu->ctx, 4); CALL(cpu, readShortAndTick(cpu, cpu->PC - 2)); return;
        op_CE: alu_im(ADC); return;
        op_CF: rst(0x08); return;

        op_D0: ret(NC); return;
        op_D1: pop(cpu->DE); return;
        op_D2: jp(NC); return;
        op_D3: printf("empty opcode!\n"); return;
        op_D4: call(NC); return;
        op_D5: push(cpu->DE); return;
        op_D6: alu_im(SUB); return;
        op_D7: rst(0x10); return;
        op_D8: ret(C); return;
        op_D9: RETI(cpu); return;
        op_DA: jp(C); return;
        op_DB: printf("empty opcode\n"); return;
        op_DC: call(C); return;
        op_DD: printf("empty opcode!\n"); return;
        op_DE: alu_im(SBC); return;
        op_DF: rst(0x18); return;

        op_E0: pc_inc(1); LDH1(cpu, readByteAndTick(cpu, cpu->PC - 1)); return;
        op_E1: pop(cpu->HL); return;
        op_E2: LDH1(cpu, cpu->C); return;
        op_E3: printf("empty opcode!\n"); return;
        op_E4: printf("empty opcode!\n"); return;
        op_E5: push(cpu->HL); return;
        op_E6: alu_im(AND); return;
        op_E7: rst(0x20); return;
        op_E8: pc_inc(1); ADD_SP(cpu, readByteAndTick(cpu, cpu->PC - 1)); cpu->tickSystem(cpu->ctx, 8); return;
        op_E9: JP(cpu, cpu->HL); return;
        op_EA: ld_u16_A(); return;
        op_EB: printf("empty opcode\n"); return;
        op_EC: printf("empty opcode!\n"); return;
        op_ED: printf("empty opcode!\n"); return;
        op_EE: alu_im(XOR); return;
        op_EF: rst(0x28); return;

        op_F0: pc_inc(1); LDH2(cpu, readByteAndTick(cpu, cpu->PC - 1)); return;
        op_F1: pop(cpu->AF); cpu->UNUSED_FLAG = 0; return;
        op_F2: LDH2(cpu, cpu->C); return;
        op_F3: DI(cpu); return;
        op_F4: printf("empty opcode!\n"); return;
        op_F5: push(cpu->AF); return;
        op_F6: alu_im(OR); return;
        op_F7: rst(0x30); return;
        op_F8: pc_inc(1); LD_SP(cpu, readByteAndTick(cpu, cpu->PC - 1)); cpu->tickSystem(cpu->ctx, 4); return;
        op_F9: cpu->tickSystem(cpu->ctx, 4); cpu->SP = cpu->HL; return;
        op_FA: ld_A_u16(); return;
        op_FB: EI(cpu); cpu->EI_DELAY = true; return;
        op_FC: printf("empty opcode!\n"); return;
        op_FD: printf("empty opcode!\n"); return;
        op_FE: alu_im(CP); return;
        op_FF: rst(0x38); return;
    }
}

static inline void prefix_CB(sm83_t* cpu, u8 opcode){
    static void* cb_table[256] = {
        &&cb_00, &&cb_01, &&cb_02, &&cb_03, &&cb_04, &&cb_05, &&cb_06, &&cb_07, &&cb_08, &&cb_09, &&cb_0A, &&cb_0B, &&cb_0C, &&cb_0D, &&cb_0E, &&cb_0F,
        &&cb_10, &&cb_11, &&cb_12, &&cb_13, &&cb_14, &&cb_15, &&cb_16, &&cb_17, &&cb_18, &&cb_19, &&cb_1A, &&cb_1B, &&cb_1C, &&cb_1D, &&cb_1E, &&cb_1F,
        &&cb_20, &&cb_21, &&cb_22, &&cb_23, &&cb_24, &&cb_25, &&cb_26, &&cb_27, &&cb_28, &&cb_29, &&cb_2A, &&cb_2B, &&cb_2C, &&cb_2D, &&cb_2E, &&cb_2F,
        &&cb_30, &&cb_31, &&cb_32, &&cb_33, &&cb_34, &&cb_35, &&cb_36, &&cb_37, &&cb_38, &&cb_39, &&cb_3A, &&cb_3B, &&cb_3C, &&cb_3D, &&cb_3E, &&cb_3F,
        &&cb_40, &&cb_41, &&cb_42, &&cb_43, &&cb_44, &&cb_45, &&cb_46, &&cb_47, &&cb_48, &&cb_49, &&cb_4A, &&cb_4B, &&cb_4C, &&cb_4D, &&cb_4E, &&cb_4F,
        &&cb_50, &&cb_51, &&cb_52, &&cb_53, &&cb_54, &&cb_55, &&cb_56, &&cb_57, &&cb_58, &&cb_59, &&cb_5A, &&cb_5B, &&cb_5C, &&cb_5D, &&cb_5E, &&cb_5F,
        &&cb_60, &&cb_61, &&cb_62, &&cb_63, &&cb_64, &&cb_65, &&cb_66, &&cb_67, &&cb_68, &&cb_69, &&cb_6A, &&cb_6B, &&cb_6C, &&cb_6D, &&cb_6E, &&cb_6F,
        &&cb_70, &&cb_71, &&cb_72, &&cb_73, &&cb_74, &&cb_75, &&cb_76, &&cb_77, &&cb_78, &&cb_79, &&cb_7A, &&cb_7B, &&cb_7C, &&cb_7D, &&cb_7E, &&cb_7F,
        &&cb_80, &&cb_81, &&cb_82, &&cb_83, &&cb_84, &&cb_85, &&cb_86, &&cb_87, &&cb_88, &&cb_89, &&cb_8A, &&cb_8B, &&cb_8C, &&cb_8D, &&cb_8E, &&cb_8F,
        &&cb_90, &&cb_91, &&cb_92, &&cb_93, &&cb_94, &&cb_95, &&cb_96, &&cb_97, &&cb_98, &&cb_99, &&cb_9A, &&cb_9B, &&cb_9C, &&cb_9D, &&cb_9E, &&cb_9F,
        &&cb_A0, &&cb_A1, &&cb_A2, &&cb_A3, &&cb_A4, &&cb_A5, &&cb_A6, &&cb_A7, &&cb_A8, &&cb_A9, &&cb_AA, &&cb_AB, &&cb_AC, &&cb_AD, &&cb_AE, &&cb_AF,
        &&cb_B0, &&cb_B1, &&cb_B2, &&cb_B3, &&cb_B4, &&cb_B5, &&cb_B6, &&cb_B7, &&cb_B8, &&cb_B9, &&cb_BA, &&cb_BB, &&cb_BC, &&cb_BD, &&cb_BE, &&cb_BF,
        &&cb_C0, &&cb_C1, &&cb_C2, &&cb_C3, &&cb_C4, &&cb_C5, &&cb_C6, &&cb_C7, &&cb_C8, &&cb_C9, &&cb_CA, &&cb_CB, &&cb_CC, &&cb_CD, &&cb_CE, &&cb_CF,
        &&cb_D0, &&cb_D1, &&cb_D2, &&cb_D3, &&cb_D4, &&cb_D5, &&cb_D6, &&cb_D7, &&cb_D8, &&cb_D9, &&cb_DA, &&cb_DB, &&cb_DC, &&cb_DD, &&cb_DE, &&cb_DF,
        &&cb_E0, &&cb_E1, &&cb_E2, &&cb_E3, &&cb_E4, &&cb_E5, &&cb_E6, &&cb_E7, &&cb_E8, &&cb_E9, &&cb_EA, &&cb_EB, &&cb_EC, &&cb_ED, &&cb_EE, &&cb_EF,
        &&cb_F0, &&cb_F1, &&cb_F2, &&cb_F3, &&cb_F4, &&cb_F5, &&cb_F6, &&cb_F7, &&cb_F8, &&cb_F9, &&cb_FA, &&cb_FB, &&cb_FC, &&cb_FD, &&cb_FE, &&cb_FF,
    };

    goto *cb_table[opcode];

    {
        cb_00: cb_no_bit(RLC, cpu->B); return;
        cb_01: cb_no_bit(RLC, cpu->C); return;
        cb_02: cb_no_bit(RLC, cpu->D); return;
        cb_03: cb_no_bit(RLC, cpu->E); return;
        cb_04: cb_no_bit(RLC, cpu->H); return;
        cb_05: cb_no_bit(RLC, cpu->L); return;
        cb_06: cb_no_bit_hl(RLC); return;
        cb_07: cb_no_bit(RLC, cpu->A); return;
        cb_08: cb_no_bit(RRC, cpu->B); return;
        cb_09: cb_no_bit(RRC, cpu->C); return;
        cb_0A: cb_no_bit(RRC, cpu->D); return;
        cb_0B: cb_no_bit(RRC, cpu->E); return;
        cb_0C: cb_no_bit(RRC, cpu->H); return;
        cb_0D: cb_no_bit(RRC, cpu->L); return;
        cb_0E: cb_no_bit_hl(RRC); return;
        cb_0F: cb_no_bit(RRC, cpu->A); return;

        cb_10: cb_no_bit(RL, cpu->B); return;
        cb_11: cb_no_bit(RL, cpu->C); return;
        cb_12: cb_no_bit(RL, cpu->D); return;
        cb_13: cb_no_bit(RL, cpu->E); return;
        cb_14: cb_no_bit(RL, cpu->H); return;
        cb_15: cb_no_bit(RL, cpu->L); return;
        cb_16: cb_no_bit_hl(RL); return;
        cb_17: cb_no_bit(RL, cpu->A); return;
        cb_18: cb_no_bit(RR, cpu->B); return;
        cb_19: cb_no_bit(RR, cpu->C); return;
        cb_1A: cb_no_bit(RR, cpu->D); return;
        cb_1B: cb_no_bit(RR, cpu->E); return;
        cb_1C: cb_no_bit(RR, cpu->H); return;
        cb_1D: cb_no_bit(RR, cpu->L); return;
        cb_1E: cb_no_bit_hl(RR); return;
        cb_1F: cb_no_bit(RR, cpu->A); return;

        cb_20: cb_no_bit(SLA, cpu->B); return;
        cb_21: cb_no_bit(SLA, cpu->C); return;
        cb_22: cb_no_bit(SLA, cpu->D); return;
        cb_23: cb_no_bit(SLA, cpu->E); return;
        cb_24: cb_no_bit(SLA, cpu->H); return;
        cb_25: cb_no_bit(SLA, cpu->L); return;
        cb_26: cb_no_bit_hl(SLA); return;
        cb_27: cb_no_bit(SLA, cpu->A); return;
        cb_28: cb_no_bit(SRA, cpu->B); return;
        cb_29: cb_no_bit(SRA, cpu->C); return;
        cb_2A: cb_no_bit(SRA, cpu->D); return;
        cb_2B: cb_no_bit(SRA, cpu->E); return;
        cb_2C: cb_no_bit(SRA, cpu->H); return;
        cb_2D: cb_no_bit(SRA, cpu->L); return;
        cb_2E: cb_no_bit_hl(SRA); return;
        cb_2F: cb_no_bit(SRA, cpu->A); return;

        cb_30: cb_no_bit(SWAP, cpu->B); return;
        cb_31: cb_no_bit(SWAP, cpu->C); return;
        cb_32: cb_no_bit(SWAP, cpu->D); return;
        cb_33: cb_no_bit(SWAP, cpu->E); return;
        cb_34: cb_no_bit(SWAP, cpu->H); return;
        cb_35: cb_no_bit(SWAP, cpu->L); return;
        cb_36: cb_no_bit_hl(SWAP); return;
        cb_37: cb_no_bit(SWAP, cpu->A); return;
        cb_38: cb_no_bit(SRL, cpu->B); return;
        cb_39: cb_no_bit(SRL, cpu->C); return;
        cb_3A: cb_no_bit(SRL, cpu->D); return;
        cb_3B: cb_no_bit(SRL, cpu->E); return;
        cb_3C: cb_no_bit(SRL, cpu->H); return;
        cb_3D: cb_no_bit(SRL, cpu->L); return;
        cb_3E: cb_no_bit_hl(SRL); return;
        cb_3F: cb_no_bit(SRL, cpu->A); return;

        cb_40: cb_bit(BIT, 0, cpu->B); return;
        cb_41: cb_bit(BIT, 0, cpu->C); return;
        cb_42: cb_bit(BIT, 0, cpu->D); return;
        cb_43: cb_bit(BIT, 0, cpu->E); return;
        cb_44: cb_bit(BIT, 0, cpu->H); return;
        cb_45: cb_bit(BIT, 0, cpu->L); return;
        cb_46: cb_bit_hl_fast(BIT, 0); return;
        cb_47: cb_bit(BIT, 0, cpu->A); return;
        cb_48: cb_bit(BIT, 1, cpu->B); return;
        cb_49: cb_bit(BIT, 1, cpu->C); return;
        cb_4A: cb_bit(BIT, 1, cpu->D); return;
        cb_4B: cb_bit(BIT, 1, cpu->E); return;
        cb_4C: cb_bit(BIT, 1, cpu->H); return;
        cb_4D: cb_bit(BIT, 1, cpu->L); return;
        cb_4E: cb_bit_hl_fast(BIT, 1); return;
        cb_4F: cb_bit(BIT, 1, cpu->A); return;

        cb_50: cb_bit(BIT, 2, cpu->B); return;
        cb_51: cb_bit(BIT, 2, cpu->C); return;
        cb_52: cb_bit(BIT, 2, cpu->D); return;
        cb_53: cb_bit(BIT, 2, cpu->E); return;
        cb_54: cb_bit(BIT, 2, cpu->H); return;
        cb_55: cb_bit(BIT, 2, cpu->L); return;
        cb_56: cb_bit_hl_fast(BIT, 2); return;
        cb_57: cb_bit(BIT, 2, cpu->A); return;
        cb_58: cb_bit(BIT, 3, cpu->B); return;
        cb_59: cb_bit(BIT, 3, cpu->C); return;
        cb_5A: cb_bit(BIT, 3, cpu->D); return;
        cb_5B: cb_bit(BIT, 3, cpu->E); return;
        cb_5C: cb_bit(BIT, 3, cpu->H); return;
        cb_5D: cb_bit(BIT, 3, cpu->L); return;
        cb_5E: cb_bit_hl_fast(BIT, 3); return;
        cb_5F: cb_bit(BIT, 3, cpu->A); return;

        cb_60: cb_bit(BIT, 4, cpu->B); return;
        cb_61: cb_bit(BIT, 4, cpu->C); return;
        cb_62: cb_bit(BIT, 4, cpu->D); return;
        cb_63: cb_bit(BIT, 4, cpu->E); return;
        cb_64: cb_bit(BIT, 4, cpu->H); return;
        cb_65: cb_bit(BIT, 4, cpu->L); return;
        cb_66: cb_bit_hl_fast(BIT, 4); return;
        cb_67: cb_bit(BIT, 4, cpu->A); return;
        cb_68: cb_bit(BIT, 5, cpu->B); return;
        cb_69: cb_bit(BIT, 5, cpu->C); return;
        cb_6A: cb_bit(BIT, 5, cpu->D); return;
        cb_6B: cb_bit(BIT, 5, cpu->E); return;
        cb_6C: cb_bit(BIT, 5, cpu->H); return;
        cb_6D: cb_bit(BIT, 5, cpu->L); return;
        cb_6E: cb_bit_hl_fast(BIT, 5); return;
        cb_6F: cb_bit(BIT, 5, cpu->A); return;

        cb_70: cb_bit(BIT, 6, cpu->B); return;
        cb_71: cb_bit(BIT, 6, cpu->C); return;
        cb_72: cb_bit(BIT, 6, cpu->D); return;
        cb_73: cb_bit(BIT, 6, cpu->E); return;
        cb_74: cb_bit(BIT, 6, cpu->H); return;
        cb_75: cb_bit(BIT, 6, cpu->L); return;
        cb_76: cb_bit_hl_fast(BIT, 6); return;
        cb_77: cb_bit(BIT, 6, cpu->A); return;
        cb_78: cb_bit(BIT, 7, cpu->B); return;
        cb_79: cb_bit(BIT, 7, cpu->C); return;
        cb_7A: cb_bit(BIT, 7, cpu->D); return;
        cb_7B: cb_bit(BIT, 7, cpu->E); return;
        cb_7C: cb_bit(BIT, 7, cpu->H); return;
        cb_7D: cb_bit(BIT, 7, cpu->L); return;
        cb_7E: cb_bit_hl_fast(BIT, 7); return;
        cb_7F: cb_bit(BIT, 7, cpu->A); return;

        cb_80: cb_bit(RES, 0, cpu->B); return;
        cb_81: cb_bit(RES, 0, cpu->C); return;
        cb_82: cb_bit(RES, 0, cpu->D); return;
        cb_83: cb_bit(RES, 0, cpu->E); return;
        cb_84: cb_bit(RES, 0, cpu->H); return;
        cb_85: cb_bit(RES, 0, cpu->L); return;
        cb_86: cb_bit_hl(RES, 0); return;
        cb_87: cb_bit(RES, 0, cpu->A); return;
        cb_88: cb_bit(RES, 1, cpu->B); return;
        cb_89: cb_bit(RES, 1, cpu->C); return;
        cb_8A: cb_bit(RES, 1, cpu->D); return;
        cb_8B: cb_bit(RES, 1, cpu->E); return;
        cb_8C: cb_bit(RES, 1, cpu->H); return;
        cb_8D: cb_bit(RES, 1, cpu->L); return;
        cb_8E: cb_bit_hl(RES, 1); return;
        cb_8F: cb_bit(RES, 1, cpu->A); return;

        cb_90: cb_bit(RES, 2, cpu->B); return;
        cb_91: cb_bit(RES, 2, cpu->C); return;
        cb_92: cb_bit(RES, 2, cpu->D); return;
        cb_93: cb_bit(RES, 2, cpu->E); return;
        cb_94: cb_bit(RES, 2, cpu->H); return;
        cb_95: cb_bit(RES, 2, cpu->L); return;
        cb_96: cb_bit_hl(RES, 2); return;
        cb_97: cb_bit(RES, 2, cpu->A); return;
        cb_98: cb_bit(RES, 3, cpu->B); return;
        cb_99: cb_bit(RES, 3, cpu->C); return;
        cb_9A: cb_bit(RES, 3, cpu->D); return;
        cb_9B: cb_bit(RES, 3, cpu->E); return;
        cb_9C: cb_bit(RES, 3, cpu->H); return;
        cb_9D: cb_bit(RES, 3, cpu->L); return;
        cb_9E: cb_bit_hl(RES, 3); return;
        cb_9F: cb_bit(RES, 3, cpu->A); return;

        cb_A0: cb_bit(RES, 4, cpu->B); return;
        cb_A1: cb_bit(RES, 4, cpu->C); return;
        cb_A2: cb_bit(RES, 4, cpu->D); return;
        cb_A3: cb_bit(RES, 4, cpu->E); return;
        cb_A4: cb_bit(RES, 4, cpu->H); return;
        cb_A5: cb_bit(RES, 4, cpu->L); return;
        cb_A6: cb_bit_hl(RES, 4); return;
        cb_A7: cb_bit(RES, 4, cpu->A); return;
        cb_A8: cb_bit(RES, 5, cpu->B); return;
        cb_A9: cb_bit(RES, 5, cpu->C); return;
        cb_AA: cb_bit(RES, 5, cpu->D); return;
        cb_AB: cb_bit(RES, 5, cpu->E); return;
        cb_AC: cb_bit(RES, 5, cpu->H); return;
        cb_AD: cb_bit(RES, 5, cpu->L); return;
        cb_AE: cb_bit_hl(RES, 5); return;
        cb_AF: cb_bit(RES, 5, cpu->A); return;

        cb_B0: cb_bit(RES, 6, cpu->B); return;
        cb_B1: cb_bit(RES, 6, cpu->C); return;
        cb_B2: cb_bit(RES, 6, cpu->D); return;
        cb_B3: cb_bit(RES, 6, cpu->E); return;
        cb_B4: cb_bit(RES, 6, cpu->H); return;
        cb_B5: cb_bit(RES, 6, cpu->L); return;
        cb_B6: cb_bit_hl(RES, 6); return;
        cb_B7: cb_bit(RES, 6, cpu->A); return;
        cb_B8: cb_bit(RES, 7, cpu->B); return;
        cb_B9: cb_bit(RES, 7, cpu->C); return;
        cb_BA: cb_bit(RES, 7, cpu->D); return;
        cb_BB: cb_bit(RES, 7, cpu->E); return;
        cb_BC: cb_bit(RES, 7, cpu->H); return;
        cb_BD: cb_bit(RES, 7, cpu->L); return;
        cb_BE: cb_bit_hl(RES, 7); return;
        cb_BF: cb_bit(RES, 7, cpu->A); return;

        cb_C0: cb_bit(SET, 0, cpu->B); return;
        cb_C1: cb_bit(SET, 0, cpu->C); return;
        cb_C2: cb_bit(SET, 0, cpu->D); return;
        cb_C3: cb_bit(SET, 0, cpu->E); return;
        cb_C4: cb_bit(SET, 0, cpu->H); return;
        cb_C5: cb_bit(SET, 0, cpu->L); return;
        cb_C6: cb_bit_hl(SET, 0); return;
        cb_C7: cb_bit(SET, 0, cpu->A); return;
        cb_C8: cb_bit(SET, 1, cpu->B); return;
        cb_C9: cb_bit(SET, 1, cpu->C); return;
        cb_CA: cb_bit(SET, 1, cpu->D); return;
        cb_CB: cb_bit(SET, 1, cpu->E); return;
        cb_CC: cb_bit(SET, 1, cpu->H); return;
        cb_CD: cb_bit(SET, 1, cpu->L); return;
        cb_CE: cb_bit_hl(SET, 1); return;
        cb_CF: cb_bit(SET, 1, cpu->A); return;

        cb_D0: cb_bit(SET, 2, cpu->B); return;
        cb_D1: cb_bit(SET, 2, cpu->C); return;
        cb_D2: cb_bit(SET, 2, cpu->D); return;
        cb_D3: cb_bit(SET, 2, cpu->E); return;
        cb_D4: cb_bit(SET, 2, cpu->H); return;
        cb_D5: cb_bit(SET, 2, cpu->L); return;
        cb_D6: cb_bit_hl(SET, 2); return;
        cb_D7: cb_bit(SET, 2, cpu->A); return;
        cb_D8: cb_bit(SET, 3, cpu->B); return;
        cb_D9: cb_bit(SET, 3, cpu->C); return;
        cb_DA: cb_bit(SET, 3, cpu->D); return;
        cb_DB: cb_bit(SET, 3, cpu->E); return;
        cb_DC: cb_bit(SET, 3, cpu->H); return;
        cb_DD: cb_bit(SET, 3, cpu->L); return;
        cb_DE: cb_bit_hl(SET, 3); return;
        cb_DF: cb_bit(SET, 3, cpu->A); return;

        cb_E0: cb_bit(SET, 4, cpu->B); return;
        cb_E1: cb_bit(SET, 4, cpu->C); return;
        cb_E2: cb_bit(SET, 4, cpu->D); return;
        cb_E3: cb_bit(SET, 4, cpu->E); return;
        cb_E4: cb_bit(SET, 4, cpu->H); return;
        cb_E5: cb_bit(SET, 4, cpu->L); return;
        cb_E6: cb_bit_hl(SET, 4); return;
        cb_E7: cb_bit(SET, 4, cpu->A); return;
        cb_E8: cb_bit(SET, 5, cpu->B); return;
        cb_E9: cb_bit(SET, 5, cpu->C); return;
        cb_EA: cb_bit(SET, 5, cpu->D); return;
        cb_EB: cb_bit(SET, 5, cpu->E); return;
        cb_EC: cb_bit(SET, 5, cpu->H); return;
        cb_ED: cb_bit(SET, 5, cpu->L); return;
        cb_EE: cb_bit_hl(SET, 5); return;
        cb_EF: cb_bit(SET, 5, cpu->A); return;

        cb_F0: cb_bit(SET, 6, cpu->B); return;
        cb_F1: cb_bit(SET, 6, cpu->C); return;
        cb_F2: cb_bit(SET, 6, cpu->D); return;
        cb_F3: cb_bit(SET, 6, cpu->E); return;
        cb_F4: cb_bit(SET, 6, cpu->H); return;
        cb_F5: cb_bit(SET, 6, cpu->L); return;
        cb_F6: cb_bit_hl(SET, 6); return;
        cb_F7: cb_bit(SET, 6, cpu->A); return;
        cb_F8: cb_bit(SET, 7, cpu->B); return;
        cb_F9: cb_bit(SET, 7, cpu->C); return;
        cb_FA: cb_bit(SET, 7, cpu->D); return;
        cb_FB: cb_bit(SET, 7, cpu->E); return;
        cb_FC: cb_bit(SET, 7, cpu->H); return;
        cb_FD: cb_bit(SET, 7, cpu->L); return;
        cb_FE: cb_bit_hl(SET, 7); return;
        cb_FF: cb_bit(SET, 7, cpu->A); return;
    }
}

// Z80 INSTRUCTIONS

static inline void STOP(sm83_t* cpu){
    printf("entered stop opcode\n");    
}

static inline void JR(sm83_t* cpu, int8_t d){
    cpu->PC += d;
    cpu->tickSystem(cpu->ctx, 4);
}

static inline void JRNZ(sm83_t* cpu, int8_t d){
    if(!cpu->Z_FLAG){
        JR(cpu, d);
    }
}

static inline void JRZ(sm83_t* cpu, int8_t d){
    if(cpu->Z_FLAG){
        JR(cpu, d);
    }
}

static inline void JRNC(sm83_t* cpu, int8_t d){
    if(!cpu->C_FLAG){
        JR(cpu, d);
    }
}

static inline void JRC(sm83_t* cpu, int8_t d){
    if(cpu->C_FLAG){
        JR(cpu, d);
    }
}

static inline void ADD_16(sm83_t* cpu, u16* regDst, u16 regSrc){
    cpu->N_FLAG = false;
    cpu->C_FLAG = calculateCarry(16, *regDst, regSrc, 0);
    cpu->H_FLAG = calculateCarry(12, *regDst, regSrc, 0);

    *regDst += regSrc;
}

static inline void INC_16(sm83_t* cpu, u16* reg){
    *reg += 1;
}

static inline void DEC_16(sm83_t* cpu, u16* reg){
    *reg -= 1;
}

static inline void INC_8(sm83_t* cpu, u8* reg){
    cpu->H_FLAG = calculateCarry(4, *reg, 1, 0);
    cpu->N_FLAG = false;

    *reg += 1;
    cpu->Z_FLAG = *reg == 0;
}

static inline void DEC_8(sm83_t* cpu, u8* reg){
    cpu->H_FLAG = !calculateCarry(4, *reg, -1, 0);
    cpu->N_FLAG = true;

    *reg -= 1;
    cpu->Z_FLAG = *reg == 0;
}

static inline void DAA(sm83_t* cpu){
    // explanation at:
    //      https://forums.nesdev.org/viewtopic.php?t=15944

    if(!cpu->N_FLAG){  // after an addition, adjust if (half-)carry occurred or if result is out of bounds
        if(cpu->C_FLAG || cpu->A > 0x99){
            cpu->A += 0x60;
            cpu->C_FLAG = true;
        }
        
        if(cpu->H_FLAG || (cpu->A & 0x0f) > 0x09)
            cpu->A += 0x6;
    } else {  // after a subtraction, only adjust if (half-)carry occurred
        if(cpu->C_FLAG)
            cpu->A -= 0x60;
        
        if(cpu->H_FLAG)
            cpu->A -= 0x6;
    }

    cpu->Z_FLAG = cpu->A == 0;
    cpu->H_FLAG = false;
}

static inline void CPL(sm83_t* cpu){
    cpu->A = ~(cpu->A);
    cpu->N_FLAG = true;
    cpu->H_FLAG = true;
}

static inline void SCF(sm83_t* cpu){
    cpu->C_FLAG = true;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;
}

static inline void CCF(sm83_t* cpu){
    cpu->C_FLAG = !cpu->C_FLAG;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;
}

static inline void HLT(sm83_t* cpu){
    if(!cpu->IME && (cpu->IE & cpu->IF & 0x1F))
        cpu->HALT_BUG = true;
    else
        cpu->HALTED = true;
}

static inline void RETNZ(sm83_t* cpu){
    if(!cpu->Z_FLAG){
        cpu->tickSystem(cpu->ctx, 4);
        POP(cpu, &cpu->PC);
    }
}

static inline void RETZ(sm83_t* cpu){
    if(cpu->Z_FLAG){
        cpu->tickSystem(cpu->ctx, 4);
        POP(cpu, &cpu->PC);
    }
}

static inline void RETNC(sm83_t* cpu){
    if(!cpu->C_FLAG){
        cpu->tickSystem(cpu->ctx, 4);
        POP(cpu, &cpu->PC);
    }
}

static inline void RETC(sm83_t* cpu){
    if(cpu->C_FLAG){
        cpu->tickSystem(cpu->ctx, 4);
        POP(cpu, &cpu->PC);
    }
}

static inline void LDH1(sm83_t* cpu, u8 n){
    writeByteAndTick(cpu, 0xFF00 + n, cpu->A);
}

static inline void LDH2(sm83_t* cpu, u8 n){
    cpu->A = readByteAndTick(cpu, 0xFF00 + n);
}


static inline void ADD_SP(sm83_t* cpu, int8_t n){
    u16 res = cpu->SP + n;
    cpu->C_FLAG = calculateCarry(8, cpu->SP, n, 0);
    cpu->H_FLAG = calculateCarry(4, cpu->SP, n, 0);

    cpu->SP = res;
    cpu->Z_FLAG = false;
    cpu->N_FLAG = false;
}

static inline void LD_SP(sm83_t* cpu, int8_t n){
    u16 tmp = cpu->SP;
    ADD_SP(cpu, n);
    cpu->HL = cpu->SP;
    cpu->SP = tmp;
}

static inline void POP(sm83_t* cpu, u16* reg){
    *reg = readShortAndTick(cpu, cpu->SP);
    cpu->SP += 2; 
}

static inline void RET(sm83_t* cpu){
    POP(cpu, &cpu->PC);
    cpu->tickSystem(cpu->ctx, 4);
}

static inline void JP(sm83_t* cpu, u16 val){
    cpu->PC = val;
}

static inline void JPNZ(sm83_t* cpu, u16 val){
    if(!cpu->Z_FLAG){
        JP(cpu, val);
        cpu->tickSystem(cpu->ctx, 4);
    }
}

static inline void JPZ(sm83_t* cpu, u16 val){
    if(cpu->Z_FLAG){
        JP(cpu, val);
        cpu->tickSystem(cpu->ctx, 4);
    }
}

static inline void JPNC(sm83_t* cpu, u16 val){
    if(!cpu->C_FLAG){
        JP(cpu, val);
        cpu->tickSystem(cpu->ctx, 4);
    }
}

static inline void JPC(sm83_t* cpu, u16 val){
    if(cpu->C_FLAG){
        JP(cpu, val);
        cpu->tickSystem(cpu->ctx, 4);
    }
}

static inline void DI(sm83_t* cpu){
    cpu->IME = false;
}

static inline void EI(sm83_t* cpu){
    cpu->IME = true;
}

static inline void CALL(sm83_t* cpu, u16 val){
    PUSH(cpu, cpu->PC);
    cpu->PC = val;
}

static inline void CALLNZ(sm83_t* cpu, u16 val){
    if(!cpu->Z_FLAG){
        cpu->tickSystem(cpu->ctx, 4);
        CALL(cpu, val);
    }
}

static inline void CALLZ(sm83_t* cpu, u16 val){
    if(cpu->Z_FLAG){
        cpu->tickSystem(cpu->ctx, 4);
        CALL(cpu, val);
    }
}

static inline void CALLNC(sm83_t* cpu, u16 val){
    if(!cpu->C_FLAG){
        cpu->tickSystem(cpu->ctx, 4);
        CALL(cpu, val);
    }
}

static inline void CALLC(sm83_t* cpu, u16 val){
    if(cpu->C_FLAG){
        cpu->tickSystem(cpu->ctx, 4);
        CALL(cpu, val);
    }
}

static inline void PUSH(sm83_t* cpu, u16 val){
    writeByteAndTick(cpu, cpu->SP - 1, val >> 8);
    writeByteAndTick(cpu, cpu->SP - 2, val & 0xFF);
    cpu->SP -= 2;
}

static inline void RST(sm83_t* cpu, u8 addr){
    CALL(cpu, addr);
}

static inline void RLCA(sm83_t* cpu){
    cpu->C_FLAG = cpu->A & 0b10000000;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;
    cpu->Z_FLAG = false;

    cpu->A = (cpu->A << 1) | (cpu->C_FLAG);
}

static inline void RRCA(sm83_t* cpu){
    cpu->C_FLAG = cpu->A & 0b1;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;
    cpu->Z_FLAG = false;
    cpu->A = (cpu->A >> 1) | (cpu->C_FLAG << 7);
}

static inline void RLA(sm83_t* cpu){
    bool old_carry = cpu->C_FLAG;
    cpu->C_FLAG = cpu->A & 0b10000000;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;
    cpu->Z_FLAG = false;
    cpu->A = (cpu->A << 1) | old_carry;
}

static inline void RRA(sm83_t* cpu){
    bool old_carry = cpu->C_FLAG;
    cpu->C_FLAG = cpu->A & 0b1;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;
    cpu->Z_FLAG = false;
    cpu->A = (cpu->A >> 1) | (old_carry << 7);
}

static inline void RLC(sm83_t* cpu, u8* reg){
    bool msb = *reg >> 7;
    cpu->C_FLAG = msb;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;
    *reg = (*reg << 1) | msb;

    cpu->Z_FLAG = *reg == 0;
}

static inline void RRC(sm83_t* cpu, u8* reg){
    bool lsb = *reg & 0b1;
    cpu->C_FLAG = lsb;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;
    *reg = (lsb << 7) | (*reg >> 1);

    cpu->Z_FLAG = *reg == 0;
}

static inline void RL(sm83_t* cpu, u8* reg){
    bool old_carry = cpu->C_FLAG;
    bool msb = *reg >> 7;
    *reg = (*reg << 1) | old_carry;
    cpu->C_FLAG = msb;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;

    cpu->Z_FLAG = *reg == 0;
}

static inline void RR(sm83_t* cpu, u8* reg){
    bool old_carry = cpu->C_FLAG;
    bool lsb = *reg & 0b1;
    *reg = (*reg >> 1) | (old_carry << 7);
    cpu->C_FLAG = lsb;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;

    cpu->Z_FLAG = *reg == 0;
}

static inline void SLA(sm83_t* cpu, u8* reg){
    bool msb = *reg >> 7;
    cpu->C_FLAG = msb;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;

    *reg = *reg << 1;

    cpu->Z_FLAG = *reg == 0;
}

static inline void SRA(sm83_t* cpu, u8* reg){
    bool sign = *reg >> 7;
    bool lsb = *reg & 0b1;
    cpu->C_FLAG = lsb;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;

    *reg = (*reg >> 1) | (sign << 7);

    cpu->Z_FLAG = *reg == 0;
}

static inline void SWAP(sm83_t* cpu, u8* reg){
    u8 high_nibble = *reg >> 4;
    *reg <<= 4;
    *reg |= high_nibble;

    cpu->Z_FLAG = *reg == 0;
    cpu->N_FLAG = false;
    cpu->H_FLAG = false;
    cpu->C_FLAG = false;
}


static inline void SRL(sm83_t* cpu, u8* reg){
    bool lsb = *reg & 0b1;
    cpu->C_FLAG = lsb;
    cpu->H_FLAG = false;
    cpu->N_FLAG = false;

    *reg = *reg >> 1;

    cpu->Z_FLAG = *reg == 0;
}

static inline void BIT(sm83_t* cpu, u8 bit, u8* reg){
    u8 masked_bit = *reg & (1 << bit);
    cpu->Z_FLAG = masked_bit == 0;
    cpu->H_FLAG = true;
    cpu->N_FLAG = false;
}

static inline void RES(sm83_t* cpu, u8 bit, u8* reg){
    *reg = *reg & (~(u8)(1 << bit));
}

static inline void SET(sm83_t* cpu, u8 bit, u8* reg){
    *reg = * reg | (1 << bit);
}

static inline void ADD(sm83_t* cpu, u8* reg, u8 val){
    u8 res = *reg + val;
    cpu->C_FLAG = calculateCarry(8, *reg, val, 0);

    cpu->H_FLAG = calculateCarry(4, *reg, val, 0);

    *reg = res;
    cpu->Z_FLAG = *reg == 0;
    cpu->N_FLAG = false;
}

static inline void ADC(sm83_t* cpu, u8* reg, u8 val){
    bool carry = cpu->C_FLAG;
    u8 res = *reg + val + carry;

    cpu->C_FLAG = calculateCarry(8, *reg, val, carry);

    cpu->H_FLAG = calculateCarry(4, *reg, val, carry);

    *reg = res;
    cpu->Z_FLAG = *reg == 0;
    cpu->N_FLAG = false;
}

static inline void SUB(sm83_t* cpu, u8* reg, u8 val){
    val = ~val + 1;
    cpu->C_FLAG = !calculateCarry(8, *reg, val - 1, 1);

    cpu->H_FLAG = !calculateCarry(4, *reg, val - 1, 1);

    u8 res = *reg + val;

    *reg = res;
    cpu->Z_FLAG = *reg == 0;
    cpu->N_FLAG = true;
}

static inline void SBC(sm83_t* cpu, u8* reg, u8 val){
    val = ~val + 1;
    bool carry = cpu->C_FLAG;
    cpu->C_FLAG = !calculateCarry(8, *reg, val - 1, !carry);
    cpu->H_FLAG = !calculateCarry(4, *reg, val - 1, !carry);

    u8 res = *reg + val - carry;

    *reg = res;
    cpu->Z_FLAG = *reg == 0;
    cpu->N_FLAG = true;
}

static inline void AND(sm83_t* cpu, u8* reg, u8 val){
    *reg &= val;
    cpu->Z_FLAG = *reg == 0;
    cpu->H_FLAG = true;
    cpu->C_FLAG = false;
    cpu->N_FLAG = false;
}

static inline void XOR(sm83_t* cpu, u8* reg, u8 val){
    *reg ^= val;
    cpu->Z_FLAG = *reg == 0;
    cpu->H_FLAG = false;
    cpu->C_FLAG = false;
    cpu->N_FLAG = false;
}

static inline void OR(sm83_t* cpu, u8* reg, u8 val){
    *reg |= val;
    cpu->Z_FLAG = *reg == 0;
    cpu->H_FLAG = false;
    cpu->C_FLAG = false;
    cpu->N_FLAG = false;
}

static inline void CP(sm83_t* cpu, u8* reg, u8 val){
    u8 copy = *reg;
    SUB(cpu, reg, val);
    *reg = copy;
}

static inline void RETI(sm83_t* cpu){
    RET(cpu);
    EI(cpu);
}

static inline bool calculateCarry(int bit, u16 a, u16 b, bool cy) {
  int32_t result = a + b + cy;
  int32_t carry = result ^ a ^ b;
  return carry & (1 << bit);
}

static inline u8 readByteAndTick(sm83_t* cpu, u16 addr){
    u8 byte = cpu->readByte(cpu->ctx, addr);
    cpu->tickSystem(cpu->ctx, 4);
    return byte;
}

static inline u16 readShortAndTick(sm83_t* cpu, u16 addr){
    return (readByteAndTick(cpu, addr+1) << 8) | readByteAndTick(cpu, addr);
}

static inline void writeByteAndTick(sm83_t* cpu, u16 addr, u8 byte){
    cpu->writeByte(cpu->ctx, addr, byte);
    cpu->tickSystem(cpu->ctx, 4);
}

static inline void writeShortAndTick(sm83_t* cpu, u16 addr, u16 val){
    writeByteAndTick(cpu, addr, val & 0xFF);
    writeByteAndTick(cpu, addr+1, val >> 8);
}

void serialize_sm83_t(sm83_t* cpu, byte_vec_t* state) {
    byte_vec_push(state, cpu->HALTED);
    byte_vec_push(state, cpu->IME);
    byte_vec_push(state, cpu->EI_DELAY);
    byte_vec_push(state, cpu->HALT_BUG);
    byte_vec_push(state, cpu->IE);
    byte_vec_push(state, cpu->IF);

    byte_vec_push_array(state, (u8*)&cpu->AF, sizeof(cpu->AF));
    byte_vec_push_array(state, (u8*)&cpu->BC, sizeof(cpu->BC));
    byte_vec_push_array(state, (u8*)&cpu->DE, sizeof(cpu->DE));
    byte_vec_push_array(state, (u8*)&cpu->HL, sizeof(cpu->HL));
    byte_vec_push_array(state, (u8*)&cpu->SP, sizeof(cpu->SP));
    byte_vec_push_array(state, (u8*)&cpu->PC, sizeof(cpu->PC));
    
    byte_vec_push_array(state, (u8*)&cpu->cycles, sizeof(cpu->cycles));
}

u8* deserialize_sm83_t(sm83_t* cpu, u8* data) {
    cpu->HALTED = *(data++);
    cpu->IME = *(data++);
    cpu->EI_DELAY = *(data++);
    cpu->HALT_BUG = *(data++);
    cpu->IE = *(data++);
    cpu->IF = *(data++);

    memcpy(&cpu->AF, data, sizeof(cpu->AF)); data += sizeof(cpu->AF);
    memcpy(&cpu->BC, data, sizeof(cpu->BC)); data += sizeof(cpu->BC);
    memcpy(&cpu->DE, data, sizeof(cpu->DE)); data += sizeof(cpu->DE);
    memcpy(&cpu->HL, data, sizeof(cpu->HL)); data += sizeof(cpu->HL);
    memcpy(&cpu->SP, data, sizeof(cpu->SP)); data += sizeof(cpu->SP);
    memcpy(&cpu->PC, data, sizeof(cpu->PC)); data += sizeof(cpu->PC);

    memcpy(&cpu->cycles, data, sizeof(cpu->cycles)); data += sizeof(cpu->cycles);
    return data;
}
