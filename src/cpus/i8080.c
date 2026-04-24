#include "cpus/i8080.h"

#include "utils/vec.h"

DEFINE_VEC(byte_vec, u8);

typedef void* (*getArgFunc)(i8080_t*, u16*);
typedef void (*noArgOpcode)(i8080_t*);
typedef void (*oneArgOpcode)(i8080_t*, void*);
typedef void (*twoArgOpcode)(i8080_t*, void*, void*);

typedef struct Opcode {
    char name[15];
    getArgFunc arg1;
    getArgFunc arg2;
    void* instruction;
    int byteLength;
} Opcode;

#define SET_S   0b10000000
#define SET_Z   0b01000000
#define SET_A   0b00010000
#define SET_P   0b00000100
#define SET_C   0b00000001
#define CLEAR_S 0b01111111
#define CLEAR_Z 0b10111111
#define CLEAR_A 0b11101111
#define CLEAR_P 0b11111011
#define CLEAR_C 0b11111110

#define STOPPED cpu->STOPPED
#define INTERRUPT_ENABLED cpu->INTERRUPT_ENABLED
#define PSW_16 cpu->PSW_16
#define B_16 cpu->B_16
#define D_16 cpu->D_16
#define H_16 cpu->H_16
#define A_8 cpu->A_8
#define F_8 cpu->F_8
#define C_8 cpu->C_8
#define B_8 cpu->B_8
#define E_8 cpu->E_8
#define D_8 cpu->D_8
#define L_8 cpu->L_8
#define H_8 cpu->H_8
#define SP cpu->SP
#define PC cpu->PC
#define cycles cpu->cycles

static inline bool i8080_is_mem_ref(const i8080_t* cpu, const u8* reg){
    return reg == &cpu->mem_arg;
}

static inline void i8080_write_reg8(i8080_t* cpu, u8* reg, u8 val){
    if(i8080_is_mem_ref(cpu, reg)){
        cpu->writeMem(cpu->ctx, cpu->mem_addr, val);
        return;
    }
    *reg = val;
}

static void setParity(i8080_t*, u16);
static void setZero(i8080_t*, u16);
static void setSign8Bit(i8080_t*, u8);
static void setSign16Bit(i8080_t*, u16);

static void i8080_infoCPU(i8080_t* i80);
static void i8080_execute(i8080_t* i80, uint16_t* ptr);

static void NOP(i8080_t*);
static void HLT(i8080_t*);
static void OP_IN(i8080_t*, u8);
static void OP_OUT(i8080_t*, u8);
static void DI(i8080_t*);
static void EI(i8080_t*);
static void STA(i8080_t*, u16);
static void STAX(i8080_t*, u16*);
static void MOV(i8080_t*, u8*, u8*);
static void MVI(i8080_t*, u8*, u8);
static void LDA(i8080_t*, u16);
static void LDAX(i8080_t*, u16*);
static void RNZ(i8080_t*);
static void RNC(i8080_t*);
static void RPO(i8080_t*);
static void RP(i8080_t*);
static void JNZ(i8080_t*, u16);
static void JNC(i8080_t*, u16);
static void JPO(i8080_t*, u16);
static void JP(i8080_t*, u16);
static void CNZ(i8080_t*, u16);
static void CNC(i8080_t*, u16);
static void CPO(i8080_t*, u16);
static void CP(i8080_t*, u16);
static void RST(i8080_t*, u8);
static void RZ(i8080_t*);
static void RC(i8080_t*);
static void RPE(i8080_t*);
static void RM(i8080_t*);
static void RET(i8080_t*);
static void JZ(i8080_t*, u16);
static void JC(i8080_t*, u16);
static void JPE(i8080_t*, u16);
static void JM(i8080_t*, u16);
static void JMP(i8080_t*, u16);
static void CZ(i8080_t*, u16);
static void CC(i8080_t*, u16);
static void CPE(i8080_t*, u16);
static void CM(i8080_t*, u16);
static void CALL(i8080_t*, u16);
static void POP(i8080_t*, u16*);
static void PUSH(i8080_t*, u16*);
static void LXI(i8080_t*, u16*, u16);
static void SHLD(i8080_t*, u16);
static void LHLD(i8080_t*, u16);
static void XTHL(i8080_t*);
static void SPHL(i8080_t*);
static void XCHG(i8080_t*);
static void PCHL(i8080_t*);
static void INX(i8080_t*, u16*);
static void DAD(i8080_t*, u16*);
static void DCX(i8080_t*, u16*);
static void INR(i8080_t*, u8*);
static void DCR(i8080_t*, u8*);
static void RLC(i8080_t*);
static void RAL(i8080_t*);
static void RRC(i8080_t*);
static void RAR(i8080_t*);
static void DAA(i8080_t*);
static void STC(i8080_t*);
static void CMC(i8080_t*);
static void CMA(i8080_t*);
static void ADD(i8080_t*, u8*);
static void ADC(i8080_t*, u8*);
static void SUB(i8080_t*, u8*);
static void SBB(i8080_t*, u8*);
static void ANA(i8080_t*, u8*);
static void ORA(i8080_t*, u8*);
static void XRA(i8080_t*, u8*);
static void CMP(i8080_t*, u8*);
static void ADI(i8080_t*, u8);
static void SUI(i8080_t*, u8);
static void ANI(i8080_t*, u8);
static void ORI(i8080_t*, u8);
static void ACI(i8080_t*, u8);
static void SBI(i8080_t*, u8);
static void XRI(i8080_t*, u8);
static void CPI(i8080_t*, u8);
static void* getNULL(i8080_t*, u16*);
static void* getSingleReg8(i8080_t*, u16*);
static void* getSrcReg8(i8080_t*, u16*);
static void* getDstReg8(i8080_t*, u16*);
static void* getSingleReg16(i8080_t*, u16*);
static void* getImm16(i8080_t*, u16*);
static void* getImm8(i8080_t*, u16*);
static void* getRST(i8080_t*, u16*);

static Opcode table[256] = {
//        x0                                              x1                                                  x2                                                x3                                                  x4                                              x5                                                 x6                                               x7                                              x8                                              x9                                                xA                                                 xB                                                 xC                                             xD                                                xE                                               xF     
/* x0 */ {"NOP",     getNULL,       getNULL,    NOP, 1}, {"LXI B,d16",  getSingleReg16, getImm16,   LXI, 3}, {"STAX B",  getSingleReg16, getNULL,    STAX, 1}, {"INX B",   getSingleReg16, getNULL,    INX,    1}, {"INR B",   getSingleReg8, getNULL,    INR, 1}, {"DCR B",    getSingleReg8,  getNULL,    DCR,  1}, {"MVI B,d8", getDstReg8,    getImm8,    MVI, 2}, {"RLC",     getNULL,       getNULL,    RLC, 1}, {"NOP",     getNULL,       getNULL,    NOP, 1}, {"DAD B",   getSingleReg16, getNULL,    DAD,  1}, {"LDAX B",   getSingleReg16, getNULL,    LDAX, 1}, {"DCX B",   getSingleReg16, getNULL,    DCX,   1}, {"INR C",   getSingleReg8, getNULL,    INR, 1}, {"DCR C",    getSingleReg8, getNULL,    DCR,  1}, {"MVI C,d8", getDstReg8,    getImm8,    MVI, 2}, {"RRC",     getNULL,       getNULL,    RRC, 1},
/* x1 */ {"NOP",     getNULL,       getNULL,    NOP, 1}, {"LXI D,d16",  getSingleReg16, getImm16,   LXI, 3}, {"STAX D",  getSingleReg16, getNULL,    STAX, 1}, {"INX D",   getSingleReg16, getNULL,    INX,    1}, {"INR D",   getSingleReg8, getNULL,    INR, 1}, {"DCR D",    getSingleReg8,  getNULL,    DCR,  1}, {"MVI D,d8", getDstReg8,    getImm8,    MVI, 2}, {"RAL",     getNULL,       getNULL,    RAL, 1}, {"NOP",     getNULL,       getNULL,    NOP, 1}, {"DAD D",   getSingleReg16, getNULL,    DAD,  1}, {"LDAX D",   getSingleReg16, getNULL,    LDAX, 1}, {"DCX D",   getSingleReg16, getNULL,    DCX,   1}, {"INR E",   getSingleReg8, getNULL,    INR, 1}, {"DCR E",    getSingleReg8, getNULL,    DCR,  1}, {"MVI E,d8", getDstReg8,    getImm8,    MVI, 2}, {"RAR",     getNULL,       getNULL,    RAR, 1},
/* x2 */ {"NOP",     getNULL,       getNULL,    NOP, 1}, {"LXI H,d16",  getSingleReg16, getImm16,   LXI, 3}, {"SHLD",    getImm16,       getNULL,    SHLD, 3}, {"INX H",   getSingleReg16, getNULL,    INX,    1}, {"INR H",   getSingleReg8, getNULL,    INR, 1}, {"DCR H",    getSingleReg8,  getNULL,    DCR,  1}, {"MVI H,d8", getDstReg8,    getImm8,    MVI, 2}, {"DAA",     getNULL,       getNULL,    DAA, 1}, {"NOP",     getNULL,       getNULL,    NOP, 1}, {"DAD H",   getSingleReg16, getNULL,    DAD,  1}, {"LHLD a16", getImm16,       getNULL,    LHLD, 3}, {"DCX H",   getSingleReg16, getNULL,    DCX,   1}, {"INR L",   getSingleReg8, getNULL,    INR, 1}, {"DCR L",    getSingleReg8, getNULL,    DCR,  1}, {"MVI L,d8", getDstReg8,    getImm8,    MVI, 2}, {"CMA",     getNULL,       getNULL,    CMA, 1},
/* x3 */ {"NOP",     getNULL,       getNULL,    NOP, 1}, {"LXI SP,d16", getSingleReg16, getImm16,   LXI, 3}, {"STA a16", getImm16,       getNULL,    STA,  3}, {"INX SP",  getSingleReg16, getNULL,    INX,    1}, {"INR M",   getSingleReg8, getNULL,    INR, 1}, {"DCR M",    getSingleReg8,  getNULL,    DCR,  1}, {"MVI M,d8", getDstReg8,    getImm8,    MVI, 2}, {"STC",     getNULL,       getNULL,    STC, 1}, {"NOP",     getNULL,       getNULL,    NOP, 1}, {"DAD SP",  getSingleReg16, getNULL,    DAD,  1}, {"LDA a16",  getImm16,       getNULL,    LDA,  3}, {"DCX SP",  getSingleReg16, getNULL,    DCX,   1}, {"INR A",   getSingleReg8, getNULL,    INR, 1}, {"DCR A",    getSingleReg8, getNULL,    DCR,  1}, {"MVI A,d8", getDstReg8,    getImm8,    MVI, 2}, {"CMC",     getNULL,       getNULL,    CMC, 1},
/* x4 */ {"MOV B,B", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV B,C",    getDstReg8,     getSrcReg8, MOV, 1}, {"MOV B,D", getDstReg8,     getSrcReg8, MOV,  1}, {"MOV B,E", getDstReg8,     getSrcReg8, MOV,    1}, {"MOV B,H", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV B,L",  getDstReg8,     getSrcReg8, MOV,  1}, {"MOV B,M",  getDstReg8,    getSrcReg8, MOV, 1}, {"MOV B,A", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV C,B", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV C,C", getDstReg8,     getSrcReg8, MOV,  1}, {"MOV C,D",  getDstReg8,     getSrcReg8, MOV,  1}, {"MOV C,E", getDstReg8,     getSrcReg8, MOV,   1}, {"MOV C,H", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV C,L",  getDstReg8,    getSrcReg8, MOV,  1}, {"MOV C,M",  getDstReg8,    getSrcReg8, MOV, 1}, {"MOV C,A", getDstReg8,    getSrcReg8, MOV, 1},
/* x5 */ {"MOV D,B", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV D,C",    getDstReg8,     getSrcReg8, MOV, 1}, {"MOV D,D", getDstReg8,     getSrcReg8, MOV,  1}, {"MOV D,E", getDstReg8,     getSrcReg8, MOV,    1}, {"MOV D,H", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV D,L",  getDstReg8,     getSrcReg8, MOV,  1}, {"MOV D,M",  getDstReg8,    getSrcReg8, MOV, 1}, {"MOV D,A", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV E,B", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV E,C", getDstReg8,     getSrcReg8, MOV,  1}, {"MOV E,D",  getDstReg8,     getSrcReg8, MOV,  1}, {"MOV E,E", getDstReg8,     getSrcReg8, MOV,   1}, {"MOV E,H", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV E,L",  getDstReg8,    getSrcReg8, MOV,  1}, {"MOV E,M",  getDstReg8,    getSrcReg8, MOV, 1}, {"MOV E,A", getDstReg8,    getSrcReg8, MOV, 1},
/* x6 */ {"MOV H,B", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV H,C",    getDstReg8,     getSrcReg8, MOV, 1}, {"MOV H,D", getDstReg8,     getSrcReg8, MOV,  1}, {"MOV H,E", getDstReg8,     getSrcReg8, MOV,    1}, {"MOV H,H", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV H,L",  getDstReg8,     getSrcReg8, MOV,  1}, {"MOV H,M",  getDstReg8,    getSrcReg8, MOV, 1}, {"MOV H,A", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV L,B", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV L,C", getDstReg8,     getSrcReg8, MOV,  1}, {"MOV L,D",  getDstReg8,     getSrcReg8, MOV,  1}, {"MOV L,E", getDstReg8,     getSrcReg8, MOV,   1}, {"MOV L,H", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV L,L",  getDstReg8,    getSrcReg8, MOV,  1}, {"MOV L,M",  getDstReg8,    getSrcReg8, MOV, 1}, {"MOV L,A", getDstReg8,    getSrcReg8, MOV, 1},
/* x7 */ {"MOV M,B", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV M,C",    getDstReg8,     getSrcReg8, MOV, 1}, {"MOV M,D", getDstReg8,     getSrcReg8, MOV,  1}, {"MOV M,E", getDstReg8,     getSrcReg8, MOV,    1}, {"MOV M,H", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV M,L",  getDstReg8,     getSrcReg8, MOV,  1}, {"HLT",      getNULL,       getNULL,    HLT, 1}, {"MOV M,A", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV A,B", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV A,C", getDstReg8,     getSrcReg8, MOV,  1}, {"MOV A,D",  getDstReg8,     getSrcReg8, MOV,  1}, {"MOV A,E", getDstReg8,     getSrcReg8, MOV,   1}, {"MOV A,H", getDstReg8,    getSrcReg8, MOV, 1}, {"MOV A,L",  getDstReg8,    getSrcReg8, MOV,  1}, {"MOV A,M",  getDstReg8,    getSrcReg8, MOV, 1}, {"MOV A,A", getDstReg8,    getSrcReg8, MOV, 1},
/* x8 */ {"ADD B",   getSrcReg8,    getNULL,    ADD, 1}, {"ADD C",      getSrcReg8,     getNULL,    ADD, 1}, {"ADD D",   getSrcReg8,     getNULL,    ADD,  1}, {"ADD E",   getSrcReg8,     getNULL,    ADD,    1}, {"ADD H",   getSrcReg8,    getNULL,    ADD, 1}, {"ADD L",    getSrcReg8,     getNULL,    ADD,  1}, {"ADD M",    getSrcReg8,    getNULL,    ADD, 1}, {"ADD A",   getSrcReg8,    getNULL,    ADD, 1}, {"ADC B",   getSrcReg8,    getNULL,    ADC, 1}, {"ADC C",   getSrcReg8,     getNULL,    ADC,  1}, {"ADC D",    getSrcReg8,     getNULL,    ADC,  1}, {"ADC E",   getSrcReg8,     getNULL,    ADC,   1}, {"ADC H",   getSrcReg8,    getNULL,    ADC, 1}, {"ADC L",    getSrcReg8,    getNULL,    ADC,  1}, {"ADC M",    getSrcReg8,    getNULL,    ADC, 1}, {"ADC A",   getSrcReg8,    getNULL,    ADC, 1}, 
/* x9 */ {"SUB B",   getSrcReg8,    getNULL,    SUB, 1}, {"SUB C",      getSrcReg8,     getNULL,    SUB, 1}, {"SUB D",   getSrcReg8,     getNULL,    SUB,  1}, {"SUB E",   getSrcReg8,     getNULL,    SUB,    1}, {"SUB H",   getSrcReg8,    getNULL,    SUB, 1}, {"SUB L",    getSrcReg8,     getNULL,    SUB,  1}, {"SUB M",    getSrcReg8,    getNULL,    SUB, 1}, {"SUB A",   getSrcReg8,    getNULL,    SUB, 1}, {"SBB B",   getSrcReg8,    getNULL,    SBB, 1}, {"SBB C",   getSrcReg8,     getNULL,    SBB,  1}, {"SBB D",    getSrcReg8,     getNULL,    SBB,  1}, {"SBB E",   getSrcReg8,     getNULL,    SBB,   1}, {"SBB H",   getSrcReg8,    getNULL,    SBB, 1}, {"SBB L",    getSrcReg8,    getNULL,    SBB,  1}, {"SBB M",    getSrcReg8,    getNULL,    SBB, 1}, {"SBB A",   getSrcReg8,    getNULL,    SBB, 1}, 
/* xA */ {"ANA B",   getSrcReg8,    getNULL,    ANA, 1}, {"ANA C",      getSrcReg8,     getNULL,    ANA, 1}, {"ANA D",   getSrcReg8,     getNULL,    ANA,  1}, {"ANA E",   getSrcReg8,     getNULL,    ANA,    1}, {"ANA H",   getSrcReg8,    getNULL,    ANA, 1}, {"ANA L",    getSrcReg8,     getNULL,    ANA,  1}, {"ANA M",    getSrcReg8,    getNULL,    ANA, 1}, {"ANA A",   getSrcReg8,    getNULL,    ANA, 1}, {"XRA B",   getSrcReg8,    getNULL,    XRA, 1}, {"XRA C",   getSrcReg8,     getNULL,    XRA,  1}, {"XRA D",    getSrcReg8,     getNULL,    XRA,  1}, {"XRA E",   getSrcReg8,     getNULL,    XRA,   1}, {"XRA H",   getSrcReg8,    getNULL,    XRA, 1}, {"XRA L",    getSrcReg8,    getNULL,    XRA,  1}, {"XRA M",    getSrcReg8,    getNULL,    XRA, 1}, {"XRA A",   getSrcReg8,    getNULL,    XRA, 1}, 
/* xB */ {"ORA B",   getSrcReg8,    getNULL,    ORA, 1}, {"ORA C",      getSrcReg8,     getNULL,    ORA, 1}, {"ORA D",   getSrcReg8,     getNULL,    ORA,  1}, {"ORA E",   getSrcReg8,     getNULL,    ORA,    1}, {"ORA H",   getSrcReg8,    getNULL,    ORA, 1}, {"ORA L",    getSrcReg8,     getNULL,    ORA,  1}, {"ORA M",    getSrcReg8,    getNULL,    ORA, 1}, {"ORA A",   getSrcReg8,    getNULL,    ORA, 1}, {"CMP B",   getSrcReg8,    getNULL,    CMP, 1}, {"CMP C",   getSrcReg8,     getNULL,    CMP,  1}, {"CMP D",    getSrcReg8,     getNULL,    CMP,  1}, {"CMP E",   getSrcReg8,     getNULL,    CMP,   1}, {"CMP H",   getSrcReg8,    getNULL,    CMP, 1}, {"CMP L",    getSrcReg8,    getNULL,    CMP,  1}, {"CMP M",    getSrcReg8,    getNULL,    CMP, 1}, {"CMP A",   getSrcReg8,    getNULL,    CMP, 1}, 
/* xC */ {"RNZ",     getNULL,       getNULL,    RNZ, 1}, {"POP B",      getSingleReg16, getNULL,    POP, 1}, {"JNZ a16", getImm16,       getNULL,    JNZ,  3}, {"JMP a16", getImm16,       getNULL,    JMP,    3}, {"CNZ a16", getImm16,      getNULL,    CNZ, 3}, {"PUSH B",   getSingleReg16, getNULL,    PUSH, 1}, {"ADI d8",   getImm8,       getNULL,    ADI, 2}, {"RST 0",   getRST,        getNULL,    RST, 1}, {"RZ",      getNULL,       getNULL,    RZ,  1}, {"RET",     getNULL,        getNULL,    RET,  1}, {"JZ a16",   getImm16,       getNULL,    JZ,   3}, {"JMP a16", getImm16,       getNULL,    JMP,   3}, {"CZ a16",  getImm16,      getNULL,    CZ,  3}, {"CALL a16", getImm16,      getNULL,    CALL, 3}, {"ACI d8",   getImm8,       getNULL,    ACI, 2}, {"RST 1",   getRST,        getNULL,    RST, 1},
/* xD */ {"RNC",     getNULL,       getNULL,    RNC, 1}, {"POP D",      getSingleReg16, getNULL,    POP, 1}, {"JNC a16", getImm16,       getNULL,    JNC,  3}, {"OUT d8",  getImm8,        getNULL,    OP_OUT, 2}, {"CNC a16", getImm16,      getNULL,    CNC, 3}, {"PUSH D",   getSingleReg16, getNULL,    PUSH, 1}, {"SUI d8",   getImm8,       getNULL,    SUI, 2}, {"RST 2",   getRST,        getNULL,    RST, 1}, {"RC",      getNULL,       getNULL,    RC,  1}, {"RET",     getNULL,        getNULL,    RET,  1}, {"JC a16",   getImm16,       getNULL,    JC,   3}, {"IN d8",   getImm8,        getNULL,    OP_IN, 2}, {"CC a16",  getImm16,      getNULL,    CC,  3}, {"CALL a16", getImm16,      getNULL,    CALL, 3}, {"SBI d8",   getImm8,       getNULL,    SBI, 2}, {"RST 3",   getRST,        getNULL,    RST, 1},
/* xE */ {"RPO",     getNULL,       getNULL,    RPO, 1}, {"POP H",      getSingleReg16, getNULL,    POP, 1}, {"JPO a16", getImm16,       getNULL,    JPO,  3}, {"XHTL",    getNULL,        getNULL,    XTHL,   1}, {"CPO a16", getImm16,      getNULL,    CPO, 3}, {"PUSH H",   getSingleReg16, getNULL,    PUSH, 1}, {"ANI d8",   getImm8,       getNULL,    ANI, 2}, {"RST 4",   getRST,        getNULL,    RST, 1}, {"RPE",     getNULL,       getNULL,    RPE, 1}, {"PCHL",    getNULL,        getNULL,    PCHL, 1}, {"JPE a16",  getImm16,       getNULL,    JPE,  3}, {"XCHG",    getNULL,        getNULL,    XCHG,  1}, {"CPE a16", getImm16,      getNULL,    CPE, 3}, {"CALL a16", getImm16,      getNULL,    CALL, 3}, {"XRI d8",   getImm8,       getNULL,    XRI, 2}, {"RST 5",   getRST,        getNULL,    RST, 1},
/* xF */ {"RP",      getNULL,       getNULL,    RP , 1}, {"POP PSW",    getSingleReg16, getNULL,    POP, 1}, {"JP a16",  getImm16,       getNULL,    JP,   3}, {"DI",      getNULL,        getNULL,    DI,     1}, {"CP a16",  getImm16,      getNULL,    CP,  3}, {"PUSH PSW", getSingleReg16, getNULL,    PUSH, 1}, {"ORI d8",   getImm8,       getNULL,    ORI, 2}, {"RST 6",   getRST,        getNULL,    RST, 1}, {"RM",      getNULL,       getNULL,    RM,  1}, {"SPHL",    getNULL,        getNULL,    SPHL, 1}, {"JM a16",   getImm16,       getNULL,    JM,   3}, {"EI",      getNULL,        getNULL,    EI,    1}, {"CM a16",  getImm16,      getNULL,    CM,  3}, {"CALL a16", getImm16,      getNULL,    CALL, 3}, {"CPI d8",   getImm8,       getNULL,    CPI, 2}, {"RST 7",   getRST,        getNULL,    RST, 1}
};

void i8080_initCPU(i8080_t* cpu, void* ctx, readMemPtr readMem, writeMemPtr writeMem, readIOPtr readIO, writeIOPtr writeIO){
    cycles = 0;
    PSW_16 = 0;
    B_16 = 0;
    D_16 = 0;
    H_16 = 0;
    SP = 0;
    PC = 0;
    F_8 |= 0b10;
    STOPPED = false;
    INTERRUPT_ENABLED = false;
    cpu->ctx = ctx;
    cpu->readMem = readMem;
    cpu->writeMem = writeMem;
    cpu->readIO = readIO;
    cpu->writeIO = writeIO;
}

static void i8080_infoCPU(i8080_t* cpu){
    fprintf(stderr, "%llu ", cycles);
    fprintf(stderr, "$%04X\t", PC);
    fprintf(stderr, "%-10s ", table[cpu->readMem(cpu->ctx, PC)].name);
    fprintf(stderr, "A :0x%02X B: 0x%02X C: 0x%02X ", A_8, B_8, C_8);
    fprintf(stderr, "DE: 0x%04X HL: 0x%04X\t", D_16, H_16);
    fprintf(stderr, "S: %d ", (bool)(F_8 & SET_S));
    fprintf(stderr, "Z: %d ", (bool)(F_8 & SET_Z));
    fprintf(stderr, "C: %d ", (bool)(F_8 & SET_C));
    fprintf(stderr, "P: %d ", (bool)(F_8 & SET_P));
    fprintf(stderr, "A: %d ", (bool)(F_8 & SET_A));
    fprintf(stderr, "SP: 0x%04X ", SP);
    fprintf(stderr, "Stack: 0x%04X", (u16)(cpu->readMem(cpu->ctx, SP) | (cpu->readMem(cpu->ctx, SP + 1) << 8)));
    fprintf(stderr, "\n");
}

void i8080_stepCPU(i8080_t* ctx){
    i8080_t* cpu = ctx;
    uint16_t tmp_ptr = PC;
    #ifdef DEBUG
        i8080_infoCPU(ctx);
    #endif
    uint8_t index = cpu->readMem(cpu->ctx, PC);
    PC = PC + table[index].byteLength;
    i8080_execute(ctx, &tmp_ptr);
}

static void i8080_execute(i8080_t* cpu, uint16_t* ptr){
    uint8_t index = cpu->readMem(cpu->ctx, *ptr);
    getArgFunc arg1Getter = table[index].arg1;
    getArgFunc arg2Getter = table[index].arg2; 

    if(arg1Getter == getNULL && arg2Getter == getNULL){
        noArgOpcode opcode = table[index].instruction;
        (*opcode)(cpu);
    } else if(arg2Getter == getNULL){
        void* arg = (*arg1Getter)(cpu, ptr);
        oneArgOpcode opcode = table[index].instruction;
        (*opcode)(cpu, arg);
    } else {
        void* arg1 = (*arg1Getter)(cpu, ptr);
        void* arg2 = (*arg2Getter)(cpu, ptr);
        twoArgOpcode opcode = table[index].instruction;
        (*opcode)(cpu, arg1, arg2);
    }
}

void serialize_i8080_t(i8080_t* cpu, byte_vec_t* vec){
    byte_vec_push(vec, STOPPED);
    byte_vec_push(vec, INTERRUPT_ENABLED);

    byte_vec_push_array(vec, (u8*)&PSW_16, sizeof(PSW_16));
    byte_vec_push_array(vec, (u8*)&B_16,   sizeof(B_16));
    byte_vec_push_array(vec, (u8*)&D_16,   sizeof(D_16));
    byte_vec_push_array(vec, (u8*)&H_16,   sizeof(H_16));

    byte_vec_push_array(vec, (u8*)&SP, sizeof(SP));
    byte_vec_push_array(vec, (u8*)&PC, sizeof(PC));

    byte_vec_push_array(vec, (u8*)&cycles, sizeof(cycles));
}

u8* deserialize_i8080_t(i8080_t* cpu, u8* data, u8* end) {
    if (data + 2 > end) return NULL;

    STOPPED = *(data++);
    INTERRUPT_ENABLED = *(data++);
    
    if (data + sizeof(PSW_16) > end) return NULL;
    memcpy(&PSW_16, data, sizeof(PSW_16)); data += sizeof(PSW_16);

    if (data + sizeof(B_16) > end) return NULL;
    memcpy(&B_16, data, sizeof(B_16)); data += sizeof(B_16);

    if (data + sizeof(D_16) > end) return NULL;
    memcpy(&D_16, data, sizeof(D_16)); data += sizeof(D_16);

    if (data + sizeof(H_16) > end) return NULL;
    memcpy(&H_16, data, sizeof(H_16)); data += sizeof(H_16);

    if (data + sizeof(SP) > end) return NULL;
    memcpy(&SP, data, sizeof(SP)); data += sizeof(SP);

    if (data + sizeof(PC) > end) return NULL;
    memcpy(&PC, data, sizeof(PC)); data += sizeof(PC);

    if (data + sizeof(cycles) > end) return NULL;
    memcpy(&cycles, data, sizeof(cycles)); data += sizeof(cycles);

    return data;
}

void i8080_generateInterrupt(i8080_t* ctx, uint8_t val){
    i8080_t* cpu = ctx;
    if(INTERRUPT_ENABLED){
        RST(ctx, val);
        INTERRUPT_ENABLED = false;
    }
}

static void setParity(i8080_t* cpu, u16 val){
    int counter = 0;
    while(val != 0){
        counter += (val & 0x1);
        val = val >> 1;
    }
    bool parity = (counter % 2 == 0);
    if(parity)
        F_8 |= SET_P;
    else
        F_8 &= CLEAR_P;
}

static void setZero(i8080_t* cpu, u16 val){
    if(val == 0)
        F_8 |= SET_Z;
    else
        F_8 &= CLEAR_Z;
}

static void setSign8Bit(i8080_t* cpu, u8 val){
    if(val & 0x80)
        F_8 |= SET_S;
    else
        F_8 &= CLEAR_S;
}

static void setSign16Bit(i8080_t* cpu, u16 val){
    if(val & 0x8000)
        F_8 |= SET_S;
    else
        F_8 &= CLEAR_S;
}

// CPU instruction set

// Misc/control instructions
static void NOP(i8080_t* cpu){
    cycles += 4;
}

static void HLT(i8080_t* cpu){
    STOPPED = true;
    cycles += 7;
}

static void OP_IN(i8080_t* cpu, uint8_t d8){
    A_8 = cpu->readIO(cpu->ctx, d8);
    cycles += 10;
}

static void OP_OUT(i8080_t* cpu, uint8_t d8){
    cpu->writeIO(cpu->ctx, d8, A_8);
    cycles += 10;
}

static void DI(i8080_t* cpu){
    INTERRUPT_ENABLED = false;
    cycles += 4;
}

static void EI(i8080_t* cpu){
    INTERRUPT_ENABLED = true;
    cycles += 4;
}


// 8bit arithmetic/logical instructions
static void STA(i8080_t* cpu, uint16_t a16){
    cpu->writeMem(cpu->ctx, a16, A_8);
    cycles += 3;
}

static void STAX(i8080_t* cpu, uint16_t* r16){
    cpu->writeMem(cpu->ctx, *r16, A_8);
    cycles += 7;
}

static void MOV(i8080_t* cpu, uint8_t* dst, uint8_t* src){
    i8080_write_reg8(cpu, dst, *src);
    cycles += 7;
}

static void MVI(i8080_t* cpu, uint8_t* r8, uint8_t d8){
    i8080_write_reg8(cpu, r8, d8);
    cycles += 10;
}

static void LDA(i8080_t* cpu, uint16_t a16){
    A_8 = cpu->readMem(cpu->ctx, a16);
    cycles += 13;
}

static void LDAX(i8080_t* cpu, uint16_t* r16){
    A_8 = cpu->readMem(cpu->ctx, *r16);
    cycles += 7;
}


// Jump/calls instructions
static void RNZ(i8080_t* cpu){
    if((F_8 & SET_Z) == 0){
        RET(cpu);
        cycles++;
    } else
        cycles += 5;
}

static void RNC(i8080_t* cpu){
    if((F_8 & SET_C) == 0){
        RET(cpu);
        cycles++;
    } else
        cycles += 5;
}

static void RPO(i8080_t* cpu){
    if((F_8 & SET_P) == 0){
        RET(cpu);
        cycles++;
    } else
        cycles += 5;
}

static void RP(i8080_t* cpu){
    if((F_8 & SET_S) == 0){
        RET(cpu);
        cycles++;
    } else
        cycles += 5;
}

static void JNZ(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_Z) == 0)
        JMP(cpu, a16);
    else
        cycles += 10;
}

static void JNC(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_C) == 0)
        JMP(cpu, a16);
    else
        cycles += 10;
}

static void JPO(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_P) == 0)
        JMP(cpu, a16);
    else
        cycles += 10;
}

static void JP(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_S) == 0)
        JMP(cpu, a16);
    else
        cycles += 10;
}

static void CNZ(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_Z) == 0)
        CALL(cpu, a16);
    else
        cycles += 11;
}

static void CNC(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_C) == 0)
        CALL(cpu, a16);
    else
        cycles += 11;
}

static void CPO(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_P) == 0)
        CALL(cpu, a16);
    else
        cycles += 11;
}

static void CP(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_S) == 0)
        CALL(cpu, a16);
    else
        cycles += 11;
}

static void RST(i8080_t* cpu, uint8_t addr){
    CALL(cpu, addr);
    cycles -= 6;
};

static void RZ(i8080_t* cpu){
    if((F_8 & SET_Z) != 0){
        RET(cpu);
        cycles++;
    } else
        cycles += 5;
}

static void RC(i8080_t* cpu){
    if((F_8 & SET_C) != 0){
        RET(cpu);
        cycles++;
    } else
        cycles += 5;
}

static void RPE(i8080_t* cpu){
    if((F_8 & SET_P) != 0){
        RET(cpu);
        cycles++;
    } else
        cycles += 5;
}

static void RM(i8080_t* cpu){
    if((F_8 & SET_S) != 0) {
        RET(cpu);
        cycles++;
    } else
        cycles += 5;
}

static void RET(i8080_t* cpu){
    POP(cpu, &PC);
}

static void JZ(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_Z) != 0)
        JMP(cpu, a16);
    else
        cycles += 10;
}

static void JC(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_C) != 0)
        JMP(cpu, a16);
    else
        cycles += 10;
}

static void JPE(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_P) != 0)
        JMP(cpu, a16);
    else
        cycles += 10;
}

static void JM(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_S) != 0)
        JMP(cpu, a16);
    else
        cycles += 10;
}

static void JMP(i8080_t* cpu, uint16_t a16){
    PC = a16;
    cycles += 10;
}

static void CZ(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_Z) != 0)
        CALL(cpu, a16);
    else
        cycles += 11;
}

static void CC(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_C) != 0)
        CALL(cpu, a16);
    else
        cycles += 11;
}

static void CPE(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_P) != 0)
        CALL(cpu, a16);
    else
        cycles += 11;
}

static void CM(i8080_t* cpu, uint16_t a16){
    if((F_8 & SET_S) != 0)
        CALL(cpu, a16);
    else
        cycles += 11;
}

static void CALL(i8080_t* cpu, uint16_t a16){
    PUSH(cpu, &PC);
    PC = a16;
    cycles += 7;
}

// 16bit load/store/move instructions
static void POP(i8080_t* cpu, uint16_t* r16){
    *r16 = (u16)(cpu->readMem(cpu->ctx, SP) | (cpu->readMem(cpu->ctx, SP + 1) << 8));
    SP = SP + 2;
    cycles += 10;
}

static void PUSH(i8080_t* cpu, uint16_t* r16){
    SP = SP - 2;
    cpu->writeMem(cpu->ctx, SP, (u8)(*r16));
    cpu->writeMem(cpu->ctx, SP + 1, (u8)(*r16 >> 8));
    cycles += 11;
}

static void LXI(i8080_t* cpu, uint16_t* r16, uint16_t d16){
    *r16 = d16;
    cycles += 10;
}

static void SHLD(i8080_t* cpu, uint16_t a16){
    cpu->writeMem(cpu->ctx, a16, L_8);
    cpu->writeMem(cpu->ctx, a16 + 1, H_8);
    cycles += 16;
}

static void LHLD(i8080_t* cpu, uint16_t a16){
    L_8 = cpu->readMem(cpu->ctx, a16);
    H_8 = cpu->readMem(cpu->ctx, a16 + 1);
    cycles += 16;
}

static void XTHL(i8080_t* cpu){
    uint8_t new_H = cpu->readMem(cpu->ctx, SP + 1);
    uint8_t new_L = cpu->readMem(cpu->ctx, SP);
    cpu->writeMem(cpu->ctx, SP + 1, H_8);
    cpu->writeMem(cpu->ctx, SP, L_8);
    H_8 = new_H;
    L_8 = new_L;
    cycles += 18;
}

static void SPHL(i8080_t* cpu){
    SP = H_16;
    cycles += 5;
}

static void XCHG(i8080_t* cpu){
    uint16_t tmp = D_16;
    D_16 = H_16;
    H_16 = tmp;
    cycles += 5;
}

static void PCHL(i8080_t* cpu){
    PC = H_16;
    cycles += 5;
}

// 16bit arithmetic/logical instructions
static void INX(i8080_t* cpu, uint16_t* r16){
    (*r16)++;
    cycles += 5;
}

static void DAD(i8080_t* cpu, uint16_t* r16){
    bool carry;

    H_16 += *r16;
    carry = H_16 < *r16;

    if(carry)
        F_8 |= SET_C;
    else
        F_8 &= CLEAR_C;
    cycles += 10;
}

static void DCX(i8080_t* cpu, uint16_t* r16){
    (*r16)--;
    cycles += 5;
}

// 8bit arithmetic/logical instructions
static void INR(i8080_t* cpu, uint8_t* m8){
    u8 val = *m8 + 1;
    i8080_write_reg8(cpu, m8, val);
    
    setSign8Bit(cpu, val);
    setZero(cpu, val);
    setParity(cpu, val);
    cycles += 5;
}

static void DCR(i8080_t* cpu, uint8_t* m8){
    u8 val = *m8 - 1;
    i8080_write_reg8(cpu, m8, val);
    
    setSign8Bit(cpu, val);
    setZero(cpu, val);
    setParity(cpu, val);
    cycles += 5;
}

static void RLC(i8080_t* cpu){
    bool carry = A_8 >> 7;

    A_8 = (A_8 << 1) | carry;

    if(carry)
        F_8 |= SET_C;
    else
        F_8 &= CLEAR_C;
    cycles += 4;
}

static void RAL(i8080_t* cpu){
    bool in_carry = F_8 & SET_C;
    bool out_carry = A_8 >> 7;

    A_8 = (A_8 << 1) | in_carry;

    if(out_carry)
        F_8 |= SET_C;
    else
        F_8 &= CLEAR_C;
    cycles += 4;
}

static void RRC(i8080_t* cpu){
    bool carry = A_8 & 1;
    A_8 = (A_8 >> 1) | (carry << 7);

    if(carry)
        F_8 |= SET_C;
    else
        F_8 &= CLEAR_C;
    cycles += 4;
}

static void RAR(i8080_t* cpu){
    bool in_carry = F_8 & SET_C;
    bool out_carry = A_8 & 1;

    A_8 = (A_8 >> 1) | (in_carry << 7);

    if(out_carry)
        F_8 |= SET_C;
    else
        F_8 &= CLEAR_C;
    cycles += 4;
}

static void DAA(i8080_t* cpu){
    uint8_t old_lower = A_8 & 0x0F;
    uint8_t old_higher = (A_8 & 0xF0) >> 4;

    if( ((A_8 & 0xF) > 9) || (F_8 & SET_A))
        A_8 += 0x6;

    if(old_lower > (A_8 & 0xF))
        F_8 |= SET_A;
    else
        F_8 &= CLEAR_A;

    if( (((A_8 & 0xF0) >> 4) > 9) || (F_8 & SET_C))
        A_8 += 0x60;

    if(old_higher > ((A_8 & 0xF0) >> 4))
        F_8 |= SET_C;

    setSign8Bit(cpu, A_8);
    setParity(cpu, A_8);
    setSign8Bit(cpu, A_8);
    setZero(cpu, A_8);
    cycles += 4;
}

static void STC(i8080_t* cpu){
    F_8 |= SET_C;
    cycles += 4;
}

static void CMC(i8080_t* cpu){
    bool carry = (F_8 & SET_C);
    if(carry)
        F_8 &= CLEAR_C;
    else
        F_8 |= SET_C;
    cycles += 4;
}

static void CMA(i8080_t* cpu){
    A_8 = ~(A_8);
    cycles += 4;
}

static void ADD(i8080_t* cpu, uint8_t* m8){
    uint8_t old_lower = (A_8) & 0xF;
    bool out_carry;

    A_8 += *m8;
    out_carry = A_8 < *m8;

    if( ((A_8) & 0xF) < old_lower)
        F_8 |= SET_A;
    else
        F_8 &= CLEAR_A;

    if(out_carry)
        F_8 |= SET_C;
    else
        F_8 &= CLEAR_C;

    setSign8Bit(cpu, A_8);
    setZero(cpu, A_8);
    setParity(cpu, A_8);
    cycles += 4;
}

static void ADC(i8080_t* cpu, uint8_t* m8){
    uint8_t old_lower = A_8 & 0xF;
    bool out_carry = false;
    bool in_carry = F_8 & SET_C;
    
    A_8 += *m8 + in_carry;
    out_carry = A_8 < *m8;

    if( ((A_8) & 0xF) < old_lower)
        F_8 |= SET_A;
    else
        F_8 &= CLEAR_A;

    if(out_carry)
        F_8 |= SET_C;
    else
        F_8 &= CLEAR_C;

    setSign8Bit(cpu, A_8);
    setZero(cpu, A_8);
    setParity(cpu, A_8);
    cycles += 4;
}

static void SUB(i8080_t* cpu, uint8_t* m8){
    uint8_t tmp = (~*m8) + 1;
    ADD(cpu, &tmp);
    CMC(cpu);
    cycles -= 4;
}

static void SBB(i8080_t* cpu, uint8_t* m8){
    uint8_t tmp = (~*m8) + 1;
    ADC(cpu, &tmp);
    CMC(cpu);
    cycles -= 4;   
}

static void ANA(i8080_t* cpu, uint8_t* m8){
    A_8 &= *m8;

    F_8 &= CLEAR_C;
    if((A_8 & 0x8) | (*m8 & 0x08))
        F_8 |= SET_A;
    else
        F_8 &= CLEAR_A;
    setSign8Bit(cpu, A_8);
    setZero(cpu, A_8);
    setParity(cpu, A_8);
    cycles += 4;
}

static void ORA(i8080_t* cpu, uint8_t* m8){
    A_8 |= *m8;

    F_8 &= CLEAR_A;
    F_8 &= CLEAR_C;
    setSign8Bit(cpu, A_8);
    setZero(cpu, A_8);
    setParity(cpu, A_8);
    cycles += 4;
}

static void XRA(i8080_t* cpu, uint8_t* m8){
    A_8 ^= *m8;

    F_8 &= CLEAR_A;
    F_8 &= CLEAR_C;
    setSign8Bit(cpu, A_8);
    setZero(cpu, A_8);
    setParity(cpu, A_8);
    cycles += 4;
}

static void CMP(i8080_t* cpu, uint8_t* m8){
    uint8_t tmp = A_8;
    SUB(cpu, m8);
    A_8 = tmp;
}

static void ADI(i8080_t* cpu, uint8_t d8){
    ADD(cpu, &d8);
    cycles += 3;
}

static void SUI(i8080_t* cpu, uint8_t d8){
    SUB(cpu, &d8);
    cycles += 3;
}

static void ANI(i8080_t* cpu, uint8_t d8){
    ANA(cpu, &d8);
    cycles += 3;
}

static void ORI(i8080_t* cpu, uint8_t d8){
    ORA(cpu, &d8);
    cycles += 3;
}

static void ACI(i8080_t* cpu, uint8_t d8){
    ADC(cpu, &d8);
    cycles += 3;
}

static void SBI(i8080_t* cpu, uint8_t d8){
    SBB(cpu, &d8);
    cycles += 3;
}

static void XRI(i8080_t* cpu, uint8_t d8){
    XRA(cpu, &d8);
    cycles += 3;
}

static void CPI(i8080_t* cpu, uint8_t d8){
    CMP(cpu, &d8);
    cycles += 3;
}

// arguments getter functions
static void* getNULL(i8080_t* cpu, uint16_t* ptr){
    return (void*)0;
}

static void* getSingleReg8(i8080_t* cpu, uint16_t* ptr){
    uint8_t reg = (cpu->readMem(cpu->ctx, *ptr) & 0b111000) >> 3;
    if(reg == 0b000)
        return (void*)&B_8;
    if(reg == 0b001)
        return (void*)&C_8;
    if(reg == 0b010)
        return (void*)&D_8;
    if(reg == 0b011)
        return (void*)&E_8;
    if(reg == 0b100)
        return (void*)&H_8;
    if(reg == 0b101)
        return (void*)&L_8;
    if(reg == 0b110){
        cpu->mem_addr = H_16;
        cpu->mem_arg = cpu->readMem(cpu->ctx, cpu->mem_addr);
        return (void*)&cpu->mem_arg;
    }
    if(reg == 0b111)
        return (void*)&A_8;
    fprintf(stderr, "ERROR\n");
    return NULL;
}

static void* getSrcReg8(i8080_t* cpu, uint16_t* ptr){
    uint8_t reg = (cpu->readMem(cpu->ctx, *ptr) & 0b111);
    if(reg == 0b000)
        return (void*)&B_8;
    if(reg == 0b001)
        return (void*)&C_8;
    if(reg == 0b010)
        return (void*)&D_8;
    if(reg == 0b011)
        return (void*)&E_8;
    if(reg == 0b100)
        return (void*)&H_8;
    if(reg == 0b101)
        return (void*)&L_8;
    if(reg == 0b110){
        cpu->mem_addr = H_16;
        cpu->mem_arg = cpu->readMem(cpu->ctx, cpu->mem_addr);
        return (void*)&cpu->mem_arg;
    }
    if(reg == 0b111)
        return (void*)&A_8;
    fprintf(stderr, "ERROR\n");
    return NULL;
}

static void* getDstReg8(i8080_t* cpu, uint16_t* ptr){
    return getSingleReg8(cpu, ptr);
}

static void* getSingleReg16(i8080_t* cpu, uint16_t* ptr){
    uint8_t rp = cpu->readMem(cpu->ctx, *ptr) >> 4;
    if((rp & 0b11) == 0b00)
        return (void*)&B_16;
    if((rp & 0b11) == 0b01)
        return (void*)&D_16;
    if((rp & 0b11) == 0b10)
        return (void*)&H_16;
    if((rp & 0b11) == 0b11){
        if((rp & 0b1100) == 0b0000)
            return (void*)&SP;
        if((rp & 0b1100) == 0b1100)
            return (void*)&PSW_16;
    }
    fprintf(stderr, "ERROR\n");
    return NULL;
}

static void* getImm16(i8080_t* cpu, uint16_t* ptr){
    // WARNING: IT IS VERY STRANGE BUT IT WORKS
    // UINPTR_T SIZE = 4 BYTE
    // UINT16_T SIZE = 2 BYTE
    uintptr_t d16 = cpu->readMem(cpu->ctx, *ptr + 1) | (cpu->readMem(cpu->ctx, *ptr + 2) << 8);
    return (void*)d16;
}

static void* getImm8(i8080_t* cpu, uint16_t* ptr){
    // WARNING: IT IS VERY STRANGE BUT IT WORKS
    // UINPTR_T SIZE = 4 BYTE
    // UINT8_T SIZE = 1 BYTE
    uintptr_t d8 = cpu->readMem(cpu->ctx, *ptr + 1);
    return (void*)d8;
}

static void* getRST(i8080_t* cpu, uint16_t* ptr){
    uintptr_t exp = cpu->readMem(cpu->ctx, *ptr) & 0b111000;
    return (void*)exp;
}
