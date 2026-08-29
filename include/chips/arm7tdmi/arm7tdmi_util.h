#ifndef __ALU_H__
#define __ALU_H__

#include "chips/arm7tdmi/arm7tdmi.h"
#include "types.h"

#include <stdio.h>

typedef bool (*condFunc)(arm7tdmi_t* cpu);
typedef void (*dataProcessingFunc)(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s);  

static u8 readByteAndTick(arm7tdmi_t* cpu, u32 addr, bool seq){
    cpu->cycles += 1;
    return cpu->readByte(cpu, addr, seq);
}

static u16 readHalfWordAndTick(arm7tdmi_t* cpu, u32 addr, bool seq){
    cpu->cycles += 1;
    return cpu->readHalfWord(cpu, addr, seq);
}

static u32 readWordAndTick(arm7tdmi_t* cpu, u32 addr, bool seq){
    cpu->cycles += 1;
    return cpu->readWord(cpu, addr, seq);
}

static void writeByteAndTick(arm7tdmi_t* cpu, u32 addr, u8 val, bool seq){
    cpu->cycles += 1;
    cpu->writeByte(cpu, addr, val, seq);
}

static void writeHalfWordAndTick(arm7tdmi_t* cpu, u32 addr, u16 val, bool seq){
    cpu->cycles += 1;
    cpu->writeHalfWord(cpu, addr, val, seq);
}

static void writeWordAndTick(arm7tdmi_t* cpu, u32 addr, u32 val, bool seq){
    cpu->cycles += 1;
    cpu->writeWord(cpu, addr, val, seq);
}

static bool cond_EQ(arm7tdmi_t* cpu){ return cpu->Z_FLAG; }
static bool cond_NE(arm7tdmi_t* cpu){ return !cpu->Z_FLAG; }
static bool cond_CS(arm7tdmi_t* cpu){ return cpu->C_FLAG; }
static bool cond_CC(arm7tdmi_t* cpu){ return !cpu->C_FLAG; }
static bool cond_MI(arm7tdmi_t* cpu){ return cpu->N_FLAG; }
static bool cond_PL(arm7tdmi_t* cpu){ return !cpu->N_FLAG; }
static bool cond_VS(arm7tdmi_t* cpu){ return cpu->V_FLAG; }
static bool cond_VC(arm7tdmi_t* cpu){ return !cpu->V_FLAG; }
static bool cond_HI(arm7tdmi_t* cpu){ return cpu->C_FLAG && !cpu->Z_FLAG; }
static bool cond_LS(arm7tdmi_t* cpu){ return !cpu->C_FLAG || cpu->Z_FLAG; }
static bool cond_GE(arm7tdmi_t* cpu){ return cpu->N_FLAG == cpu->V_FLAG; }
static bool cond_LT(arm7tdmi_t* cpu){ return cpu->N_FLAG != cpu->V_FLAG; }
static bool cond_GT(arm7tdmi_t* cpu){ return !cpu->Z_FLAG && (cpu->N_FLAG == cpu->V_FLAG); }
static bool cond_LE(arm7tdmi_t* cpu){ return cpu->Z_FLAG || (cpu->N_FLAG != cpu->V_FLAG); }
static bool cond_AL(arm7tdmi_t* cpu){ return true; }
static bool cond_Undefined(arm7tdmi_t* cpu){ printf("undefined cond!\n"); arm7tdmi_print(cpu); return true; }

static void alu_AND(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    *rd = op1 & op2;
    if(s){
        cpu->Z_FLAG = (*rd) == 0;
        cpu->N_FLAG = *rd >> 31;
    }
}

static void alu_EOR(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    *rd = op1 ^ op2;
    if(s){
        cpu->N_FLAG = *rd >> 31;
        cpu->Z_FLAG = !(*rd);
    }
}

static void alu_SUB(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    *rd = op1 - op2;
    if(s){
        cpu->Z_FLAG = !(*rd);
        cpu->N_FLAG = *rd >> 31;
        cpu->C_FLAG = !(op1 < op2);
        cpu->V_FLAG = ((op1>>31) != (op2>>31)) && ((op1>>31) != (*rd>>31));
    }
}

static void alu_RSB(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    alu_SUB(cpu, rd, op2, op1, s);
}

static void alu_ADD(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    *rd = op1 + op2;
    if(s){
        cpu->Z_FLAG = !(*rd);
        cpu->C_FLAG = (((u32) (op1) >> 31) + ((u32) (op2) >> 31) > ((u32) (*rd) >> 31));
        cpu->N_FLAG = *rd >> 31;
        cpu->V_FLAG = ((op1>>31) == (op2>>31)) && ((op1>>31) != (*rd>>31));
    }
}

static void alu_ADC(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    *rd = op1 + op2 + cpu->C_FLAG;
    if(s){
        cpu->Z_FLAG = !(*rd);
        cpu->C_FLAG = (((u32) (op1) >> 31) + ((u32) (op2) >> 31) > ((u32) (*rd) >> 31));
        cpu->N_FLAG = *rd >> 31;
        cpu->V_FLAG = ((op1>>31) == (op2>>31)) && ((op1>>31) != (*rd>>31));
    }
}

static void alu_SBC(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    *rd = op1 - op2 - !cpu->C_FLAG;
    if(s){
        cpu->Z_FLAG = !(*rd);
        cpu->N_FLAG = *rd >> 31;
        cpu->C_FLAG = (u64)op1 >= (u64)op2 + (u64)!cpu->C_FLAG;
        cpu->V_FLAG = ((op1>>31) != (op2>>31)) && ((op1>>31) != (*rd>>31));
    }
}

static void alu_RSC(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    alu_SBC(cpu, rd, op2, op1, s);
}

static void alu_TST(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    u32 copy = *rd;
    alu_AND(cpu, rd, op1, op2, true);
    *rd = copy;
}

static void alu_TEQ(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    u32 copy = *rd;
    alu_EOR(cpu, rd, op1, op2, true);
    *rd = copy;
}

static void alu_CMP(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    u32 copy = *rd;
    alu_SUB(cpu, rd, op1, op2, true);
    *rd = copy;
}

static void alu_CMN(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    u32 copy = *rd;
    alu_ADD(cpu, rd, op1, op2, s);
    *rd = copy;
}

static void alu_ORR(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){    
    *rd = op1 | op2;
     if(s){
        cpu->Z_FLAG = !(*rd);
        cpu->N_FLAG = *rd >> 31;
    }
}

static void alu_MOV(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    *rd = op2;
    if(s){
        cpu->Z_FLAG = !(*rd);
        cpu->N_FLAG = *rd >> 31;
    }    
}

static void alu_BIC(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    alu_AND(cpu, rd, op1, ~op2, s);
}

static void alu_MVN(arm7tdmi_t* cpu, u32* rd, u32 op1, u32 op2, bool s){
    alu_MOV(cpu, rd, op1, ~op2, s);
}

static u32 alu_LSL(arm7tdmi_t* cpu, u32 val, u8 shift_amnt, bool s){
    if(s){
        if(shift_amnt > 32)
            cpu->C_FLAG = false;
        else if(shift_amnt == 32)
            cpu->C_FLAG = val & 1;
        else if(shift_amnt)
            cpu->C_FLAG = val & (1 << (32 - shift_amnt));
    }
    
    if(shift_amnt >= 32)
        return 0;
    else
        return val << shift_amnt;
}

static u32 alu_LSR(arm7tdmi_t* cpu, u32 val, u8 shift_amnt, bool s){
    if(s){
        if(shift_amnt > 32)
            cpu->C_FLAG = false;
        else if(shift_amnt == 32)
            cpu->C_FLAG = val >> 31;
        else if(shift_amnt)
            cpu->C_FLAG = val & (1 << (shift_amnt - 1));
    }
    
    if(shift_amnt >= 32)
        return 0;
    else
        return val >> shift_amnt;
}

static u32 alu_ASR(arm7tdmi_t* cpu, u32 val, u8 shift_amnt, bool s){
    if(s){
        if(shift_amnt >= 32)
            cpu->C_FLAG = val >> 31;
        else if(shift_amnt)
            cpu->C_FLAG = val & (1 << (shift_amnt - 1));
    }

    if(shift_amnt >= 32)
        return val >> 31 ? 0xFFFFFFFF : 0;
    else
        return (i32)val >> shift_amnt;
}

static u32 alu_ROR(arm7tdmi_t* cpu, u32 val, u8 shift_amnt, bool s){
    while(shift_amnt > 32)
        shift_amnt -= 32;

    if(s){
        if(shift_amnt == 32)
            cpu->C_FLAG = val >> 31;
        else if(shift_amnt)
            cpu->C_FLAG = val & (1 << (shift_amnt - 1));
    }

    if(shift_amnt == 32)
        return val;
    else
        return val >> shift_amnt | (val & ((1 << shift_amnt) - 1)) << (32 - shift_amnt);
}

static u32 alu_RRX(arm7tdmi_t* cpu, u32 val, u8 shift_amnt, bool s){
    bool new_flag = val & 1;
    val = (cpu->C_FLAG << 31) | (val >> 1);
    if(s)
        cpu->C_FLAG = new_flag;
    return val;
}

static void alu_SWP(arm7tdmi_t* cpu, u32 opcode){
    bool b = (opcode >> 22) & 1;
    u32* rd = &cpu->r[(opcode >> 12) & 0xF];
    u32 rn = cpu->r[(opcode >> 16) & 0xF];
    u32 rm = cpu->r[opcode & 0xF];

    if(b){
        u8 byte = readByteAndTick(cpu, rn, false);
        writeByteAndTick(cpu, rn, rm, false);
        *rd = byte;
    } else {
        u32 word = readWordAndTick(cpu, rn & ~(0b11), false);
        writeWordAndTick(cpu, rn & ~(0b11), rm, false);
        *rd = word;
        *rd = alu_ROR(cpu, *rd, (rn & 0b11) << 3, false);
    }
}

static const condFunc condFuncs[16] = {
    &cond_EQ, &cond_NE, &cond_CS, &cond_CC,
    &cond_MI, &cond_PL, &cond_VS, &cond_VC,
    &cond_HI, &cond_LS, &cond_GE, &cond_LT,
    &cond_GT, &cond_LE, &cond_AL, &cond_Undefined
};

static void arm_pipeline_refill(arm7tdmi_t* cpu){
    cpu->pipeline_opcode[0] = readWordAndTick(cpu, cpu->r[15], false);
    cpu->r[15] += 4;
    cpu->pipeline_opcode[1] = readWordAndTick(cpu, cpu->r[15], true);
    cpu->fetch_seq = true;
}

static void thumb_pipeline_refill(arm7tdmi_t* cpu){
    cpu->pipeline_opcode[0] = readHalfWordAndTick(cpu, cpu->r[15], false);
    cpu->r[15] += 2;
    cpu->pipeline_opcode[1] = readHalfWordAndTick(cpu, cpu->r[15], true);
    cpu->fetch_seq = true;
}

#endif