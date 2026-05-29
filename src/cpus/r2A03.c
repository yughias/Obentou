#include "cpus/r2A03.h"

#include <stdio.h>

typedef enum OPERAND {
    ACC_0,
    ABS_0,
    ABS_X,
    ABS_Y,
    IMM_0,
    IMP_0,
    IND_0,
    X_IND,
    IND_Y,
    REL_0,
    ZPG_0,
    ZPG_X,
    ZPG_Y,
} OPERAND;

typedef void (*opcodePtr)(r2A03_t*);

typedef struct opcode_t {
    opcodePtr func;
    OPERAND operand;
    bool slow;
} opcode_t;

// the magic value depends on the cpu
// and even on external factor
#define MAGIC_BYTE 0xEE

#define SET_N (1 << 7)
#define SET_V (1 << 6)
#define SET_U (1 << 5)
#define SET_B (1 << 4)
#define SET_D (1 << 3)
#define SET_I (1 << 2)
#define SET_Z (1 << 1)
#define SET_C 1

#define CLEAR_N (~SET_N) 
#define CLEAR_V (~SET_V) 
#define CLEAR_B (~SET_B) 
#define CLEAR_D (~SET_D) 
#define CLEAR_I (~SET_I) 
#define CLEAR_Z (~SET_Z) 
#define CLEAR_C (~SET_C)

static u8 inline r2A03_read_byte(r2A03_t* r, u16 addr){
    u8 out = r->read(r->ctx, addr);
    r->cycles += 1;
    return out;
}

static void inline r2A03_dummy_read(r2A03_t* r, u16 addr){
    #ifndef M6502_LO_ACC
        r->read(r->ctx, addr);
    #endif
    r->cycles += 1;
}

#define read_byte(addr) r2A03_read_byte(r, addr)
#define dummy_read(addr) r2A03_dummy_read(r, addr)
#define write_byte(addr, byte) r->write(r->ctx, addr, byte); r->cycles += 1
#define fetch read_byte(r->pc); r->pc += 1
#define calculate_n(x) (x) & 0x80 ? (r->p |= SET_N) : (r->p &= CLEAR_N)
#define calculate_z(x) !((u8)x) ? (r->p |= SET_Z) : (r->p &= CLEAR_Z)
#define change_c(x) (x) ? (r->p |= SET_C) : (r->p &= CLEAR_C)
#define change_v(x) (x) ? (r->p |= SET_V) : (r->p &= CLEAR_V)
#define write_back(x) if(r->in_mem) { write_byte(r->mem_addr, (x)); } else r->a = (x)
#define get_arg if(r->in_mem) { r->op_arg = read_byte(r->mem_addr); }
#define push(x) write_byte(r->s | 0x100, x); r->s -= 1
#define pop read_byte((++r->s) | 0x100);
#define branch_on(arg, cond) if(cond){ i16 ext_arg = (i16)(i8)arg; u16 new_pc = r->pc + ext_arg; dummy_read(r->pc); if(new_pc >> 8 != r->pc >> 8){ dummy_read((r->pc & 0xFF00) | (new_pc & 0xFF) ); } r->pc = new_pc; } 
#define ld(x) if(r->in_mem) { r->op_arg = read_byte(r->mem_addr); } x = r->op_arg; calculate_n(x); calculate_z(x)

static void BRK(r2A03_t* r) { 
    fetch;
    push(r->pc >> 8);
    push(r->pc & 0xFF);
    push(r->p | SET_B);
    u8 lsb = read_byte(0xFFFE);
    u8 msb = read_byte(0xFFFF);
    r->pc = lsb | (msb << 8);
    r->p |= SET_I;
}

static void ORA(r2A03_t* r) {
    get_arg;
    r->a |= r->op_arg;
    calculate_n(r->a);
    calculate_z(r->a);
}

static void JAM(r2A03_t* r) {  
    dummy_read(r->pc);  
    dummy_read(0xFFFF);
    dummy_read(0xFFFE);
    dummy_read(0xFFFE);
    dummy_read(0xFFFF);
    // ideally it should be an endless loop
    for(int i = 0; i < 5; i++)
        dummy_read(0xFFFF);
}

static void SLO(r2A03_t* r) {
    get_arg;
    change_c(r->op_arg & 0x80);
    write_byte(r->mem_addr, r->op_arg);
    r->op_arg <<= 1;
    r->a |= r->op_arg;
    write_byte(r->mem_addr, r->op_arg);
    calculate_n(r->a);
    calculate_z(r->a);
}

static void NOP(r2A03_t* r) {
    dummy_read(r->pc);
}

static void NOP1(r2A03_t* r) {
    dummy_read(r->mem_addr);
}

static void NOP2(r2A03_t* r) {
}

static void ASL(r2A03_t* r) { 
    get_arg;
    change_c(r->op_arg & 0x80);
    if(r->in_mem) {
        write_byte(r->mem_addr, r->op_arg);
    } else {
        dummy_read(r->pc);
    }
    r->op_arg <<= 1;
    calculate_n(r->op_arg);
    calculate_z(r->op_arg);
    write_back(r->op_arg);
}

static void PHP(r2A03_t* r) { 
    dummy_read(r->pc);
    push(r->p | SET_B | SET_U);
}

static void ANC(r2A03_t* r) {
    get_arg;
    r->a &= r->op_arg;
    change_c(r->a & 0x80);
    calculate_n(r->a);
    calculate_z(r->a);
}

static void BPL(r2A03_t* r) {
    branch_on(r->op_arg, !(r->p & SET_N));
}

static void CLC(r2A03_t* r) { 
    change_c(0);
    dummy_read(r->pc);
}

static void JSR(r2A03_t* r) { 
    u8 l = fetch;
    dummy_read(r->s | 0x100);
    push(r->pc >> 8);
    push(r->pc & 0xFF);
    u8 h = read_byte(r->pc);
    r->pc = (h << 8) | l;
}

static void AND(r2A03_t* r) {
    get_arg;
    r->a &= r->op_arg;
    calculate_n(r->a);
    calculate_z(r->a);
}

static void BIT(r2A03_t* r) {
    get_arg;
    r->p &= CLEAR_N & CLEAR_V;
    r->p |= r->op_arg & 0xC0;
    calculate_z(r->op_arg & r->a);
}

static void PLP(r2A03_t* r) {
    dummy_read(r->pc);
    dummy_read(r->s | 0x100);
    r->p = pop;
    r->p |= SET_U;
    r->p &= CLEAR_B;
}

static void BMI(r2A03_t* r) { 
    branch_on(r->op_arg, r->p & SET_N);
}

static void SEC(r2A03_t* r) { 
    change_c(1);
    dummy_read(r->pc);
}

static void RTI(r2A03_t* r) {
    dummy_read(r->pc);
    dummy_read(r->s | 0x100);
    r->p = read_byte((r->s + 1) | 0x100);
    u8 pcl = read_byte((r->s + 2) | 0x100);
    r->s += 3;
    u8 pch = read_byte(r->s | 0x100);
    r->p &= CLEAR_B;
    r->p |= SET_U;
    r->pc = pcl | (pch << 8); 
}

static void EOR(r2A03_t* r) {
    get_arg;
    r->a ^= r->op_arg;
    calculate_n(r->a);
    calculate_z(r->a);
}

static void LSR(r2A03_t* r) { 
    get_arg;
    change_c(r->op_arg & 1);
    if(r->in_mem){
        write_byte(r->mem_addr, r->op_arg);
    } else {
        dummy_read(r->pc);
    }
    r->op_arg >>= 1;
    r->p &= CLEAR_N;
    calculate_z(r->op_arg);
    write_back(r->op_arg);
}

static void PHA(r2A03_t* r) { 
    dummy_read(r->pc);
    push(r->a);
}

static void ALR(r2A03_t* r) {
    get_arg;
    r->a &= r->op_arg;
    change_c(r->a & 1);
    r->a >>= 1;
    calculate_n(r->a);
    calculate_z(r->a);
}

static void JMP(r2A03_t* r) { 
    r->pc = r->mem_addr;
}

static void BVC(r2A03_t* r) { 
    branch_on(r->op_arg, !(r->p & SET_V));
}

static void CLI(r2A03_t* r) { 
    r->p &= CLEAR_I;
    dummy_read(r->pc);
}

static void RTS(r2A03_t* r) { 
    dummy_read(r->pc);
    dummy_read(r->s | 0x100);
    u8 pcl = pop;
    u8 pch = pop;
    r->pc = pcl | (pch << 8); 
    dummy_read(r->pc);
    r->pc += 1;
}

static void ADC(r2A03_t* r) {
    get_arg;
    u16 ires = (i16)(i8)r->op_arg + (i16)(i8)r->a + (bool)(r->p & SET_C);
    u16 ures = r->op_arg + r->a + (bool)(r->p & SET_C);
    r->a = ures;
    change_c(ures > 0xFF);
    change_v(((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80))); 
    calculate_n(r->a);
    calculate_z(r->a);
}

static void PLA(r2A03_t* r) { 
    dummy_read(r->pc);
    dummy_read(r->s | 0x100);
    r->a = pop;
    calculate_n(r->a);
    calculate_z(r->a);
}

static void ARR(r2A03_t* r) {
    get_arg;

    bool carry = r->p & SET_C;
    u8 tmp = carry << 7;
    r->a &= r->op_arg;
    change_c(r->a & 0x80);
    r->a = (r->a >> 1) | tmp;
    carry = r->p & SET_C;
    change_v(carry ^ ((r->a >> 5) & 1));
    
    calculate_n(r->a);
    calculate_z(r->a);
}

static void BVS(r2A03_t* r) { 
    branch_on(r->op_arg, r->p & SET_V);
}

static void SEI(r2A03_t* r) { 
    r->p |= SET_I;
    dummy_read(r->pc);
}

static void STA(r2A03_t* r) { 
    write_byte(r->mem_addr, (r->a));
}

static void SAX(r2A03_t* r) {
    u8 tmp = r->a & r->x;
    write_byte(r->mem_addr, tmp);
}

static void DEY(r2A03_t* r) {
    r->y -= 1;
    calculate_n(r->y);
    calculate_z(r->y);
    dummy_read(r->pc);
}

static void TXA(r2A03_t* r) {
    r->a = r->x;
    calculate_n(r->a);
    calculate_z(r->a);
    dummy_read(r->pc);
}

static void ANE(r2A03_t* r) {
    r->a |= MAGIC_BYTE;
    r->a &= r->x;
    r->a &= r->op_arg;
    calculate_n(r->a);
    calculate_z(r->a);
}

static void TYA(r2A03_t* r) { 
    r->a = r->y;
    calculate_n(r->a);
    calculate_z(r->a);
    dummy_read(r->pc);
}

static void TXS(r2A03_t* r) {
    r->s = r->x;
    dummy_read(r->pc);
}

static void TAS(r2A03_t* r) {
    u8 no_y_addr = (r->mem_addr - r->y) & 0xFF;
    bool swap = no_y_addr + r->y > 0xFF;
    u8 hi = r->mem_addr >> 8;
    r->s = r->a & r->x; 
    if(swap){
        hi &= r->s;
        r->mem_addr = (r->mem_addr & 0xFF) | (hi << 8);
    }
    u8 tmp = r->s & (hi + !swap);
    write_byte(r->mem_addr, tmp);
}

static void SHY(r2A03_t* r) {
    u8 no_x_addr = (r->mem_addr - r->x) & 0xFF;
    bool swap = no_x_addr + r->x > 0xFF;
    u8 hi = r->mem_addr >> 8;
    if(swap){
        hi &= r->y;
        r->mem_addr = (r->mem_addr & 0xFF) | (hi << 8);
    }
    u8 tmp = r->y & (hi + !swap);
    write_byte(r->mem_addr, tmp);   
}

static void SHX(r2A03_t* r) {
    u8 no_y_addr = (r->mem_addr - r->y) & 0xFF;
    bool swap = no_y_addr + r->y > 0xFF;
    u8 hi = r->mem_addr >> 8;
    if(swap){
        hi &= r->x;
        r->mem_addr = (r->mem_addr & 0xFF) | (hi << 8);
    }
    u8 tmp = r->x & (hi + !swap);
    write_byte(r->mem_addr, tmp);   
}

static void LDY(r2A03_t* r) { ld(r->y); }
static void LDA(r2A03_t* r) { ld(r->a); }
static void LDX(r2A03_t* r) { ld(r->x); }

static void TAY(r2A03_t* r) {
    r->y = r->a;
    calculate_n(r->a);
    calculate_z(r->a);
    dummy_read(r->pc);
}

static void TAX(r2A03_t* r) {
    r->x = r->a;
    calculate_n(r->a);
    calculate_z(r->a);
    dummy_read(r->pc);
}

static void LXA(r2A03_t* r) {
    r->a |= MAGIC_BYTE;
    r->a &= r->op_arg;
    r->x = r->a;
    calculate_n(r->a);
    calculate_z(r->a);    
}

static void CLV(r2A03_t* r) {
    r->p &= CLEAR_V;
    dummy_read(r->pc);
}

static void TSX(r2A03_t* r) {
    r->x = r->s;
    calculate_n(r->s);
    calculate_z(r->s);
    dummy_read(r->pc);
}

static void LAS(r2A03_t* r) {
    get_arg;
    r->a = r->op_arg & r->s;
    r->x = r->a;
    r->s = r->a;
    calculate_n(r->a);
    calculate_z(r->a);
}

static void CPY(r2A03_t* r) {
    get_arg;
    r->op_arg = ~r->op_arg;
    u16 ures = r->y + r->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void CMP(r2A03_t* r) { 
    get_arg;
    r->op_arg = ~r->op_arg;
    u16 ures = r->a + r->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void DEC(r2A03_t* r) {
    get_arg;
    write_byte(r->mem_addr, r->op_arg);
    r->op_arg -= 1;
    calculate_n(r->op_arg);
    calculate_z(r->op_arg);
    write_byte(r->mem_addr, r->op_arg);
}

static void INY(r2A03_t* r) { 
    dummy_read(r->pc);
    r->y += 1;
    calculate_n(r->y);
    calculate_z(r->y);
}

static void DEX(r2A03_t* r) { 
    dummy_read(r->pc);
    r->x -= 1;
    calculate_n(r->x);
    calculate_z(r->x);
}

static void SBX(r2A03_t* r) {
    u8 tmp = r->a & r->x;

    r->op_arg = ~r->op_arg;
    u16 ures = tmp + r->op_arg + 1;
    r->x = ures;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void BNE(r2A03_t* r) {
    branch_on(r->op_arg, !(r->p & SET_Z));
}

static void CLD(r2A03_t* r) { 
    dummy_read(r->pc);
    r->p &= CLEAR_D;
}

static void CPX(r2A03_t* r) {
    get_arg;
    r->op_arg = ~r->op_arg;
    u16 ures = r->x + r->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void SBC(r2A03_t* r) { 
    get_arg;
    r->op_arg = ~r->op_arg;
    u16 ires = (i16)(i8)r->op_arg + (i16)(i8)r->a + (bool)(r->p & SET_C);
    u16 ures = r->op_arg + r->a + (bool)(r->p & SET_C);
    r->a = ures;
    change_c(ures > 0xFF);
    change_v(((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80))); 
    calculate_n(r->a);
    calculate_z(r->a);
}

static void INC(r2A03_t* r) { 
    get_arg;
    write_byte(r->mem_addr, r->op_arg);
    r->op_arg += 1;
    calculate_n(r->op_arg);
    calculate_z(r->op_arg);
    write_byte(r->mem_addr, r->op_arg);
}

static void INX(r2A03_t* r) { 
    dummy_read(r->pc);
    r->x += 1;
    calculate_n(r->x);
    calculate_z(r->x);
}

static void USBC(r2A03_t* r) {
    SBC(r);
}

static void BEQ(r2A03_t* r) {
    branch_on(r->op_arg, r->p & SET_Z);
}

static void SED(r2A03_t* r) { 
    dummy_read(r->pc);
    r->p |= SET_D;
}

static void ISC(r2A03_t* r) {
    get_arg;
    write_byte(r->mem_addr, r->op_arg);
    r->op_arg += 1;
    write_byte(r->mem_addr, r->op_arg);

    r->op_arg = ~r->op_arg;
    u16 ires = (i16)(i8)r->op_arg + (i16)(i8)r->a + (bool)(r->p & SET_C);
    u16 ures = r->op_arg + r->a + (bool)(r->p & SET_C);
    r->a = ures;
    change_c(ures > 0xFF);
    change_v(((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80))); 
    calculate_n(r->a);
    calculate_z(r->a);    
}

static void DCP(r2A03_t* r) {
    get_arg;
    write_byte(r->mem_addr, r->op_arg);
    r->op_arg -= 1;
    write_byte(r->mem_addr, r->op_arg);

    r->op_arg = ~r->op_arg;
    u16 ures = r->a + r->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void RLA(r2A03_t* r) {
    get_arg;
    bool carry = r->p & SET_C;
    change_c(r->op_arg & 0x80);
    write_byte(r->mem_addr, r->op_arg);
    r->op_arg = (r->op_arg << 1) | carry;
    r->a &= r->op_arg;
    write_byte(r->mem_addr, r->op_arg);
    calculate_n(r->a);
    calculate_z(r->a);
}

static void SRE(r2A03_t* r) {
    get_arg;
    change_c(r->op_arg & 1);
    write_byte(r->mem_addr, r->op_arg);
    r->op_arg >>= 1;
    write_byte(r->mem_addr, r->op_arg);
    r->a ^= r->op_arg;
    calculate_n(r->a);
    calculate_z(r->a);
}

static void RRA(r2A03_t* r) {
    get_arg;
    bool carry = r->p & SET_C;
    change_c(r->op_arg & 1);
    write_byte(r->mem_addr, r->op_arg);
    r->op_arg = (r->op_arg >> 1) | (carry << 7);
    write_byte(r->mem_addr, r->op_arg);

    u16 ires = (i16)(i8)r->op_arg + (i16)(i8)r->a + (bool)(r->p & SET_C);
    u16 ures = r->op_arg + r->a + (bool)(r->p & SET_C);
    r->a = ures;
    change_c(ures > 0xFF);
    change_v(((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80))); 
    calculate_n(r->a);
    calculate_z(r->a);
}

static void ROL(r2A03_t* r) {
    get_arg;
    bool c = r->op_arg & 0x80;
    if(r->in_mem){
        write_byte(r->mem_addr, r->op_arg);
    } else {
        dummy_read(r->pc);
    }
    r->op_arg = (r->op_arg << 1) | ((bool)(r->p & SET_C));
    change_c(c);
    calculate_n(r->op_arg);
    calculate_z(r->op_arg);
    write_back(r->op_arg);
}

static void ROR(r2A03_t* r) {
    get_arg;
    bool c = r->op_arg & 1;
    if(r->in_mem){
        write_byte(r->mem_addr, r->op_arg);
    } else {
        dummy_read(r->pc);
    }
    r->op_arg = (r->op_arg >> 1) | (((bool)(r->p & SET_C)) << 7);
    change_c(c);
    calculate_n(r->op_arg);
    calculate_z(r->op_arg);
    write_back(r->op_arg);
}

static void SHA(r2A03_t* r) {
    u8 no_y_addr = (r->mem_addr - r->y) & 0xFF;
    bool swap = no_y_addr + r->y > 0xFF;
    u8 hi = r->mem_addr >> 8;
    u8 tmp = r->a & r->x; 
    if(swap){
        hi &= tmp;
        r->mem_addr = (r->mem_addr & 0xFF) | (hi << 8);
    }
    tmp &= (hi + !swap);
    write_byte(r->mem_addr, tmp);
}

static void LAX(r2A03_t* r) {
    get_arg;
    r->a = r->x = r->op_arg;
    calculate_n(r->a);
    calculate_z(r->a);
}

static void STY(r2A03_t* r) { 
    write_byte(r->mem_addr, (r->y));
}

static void STX(r2A03_t* r) { 
    write_byte(r->mem_addr, (r->x));
}

static void BCS(r2A03_t* r) {
    branch_on(r->op_arg, r->p & SET_C);
}

static void BCC(r2A03_t* r) {
    branch_on(r->op_arg, !(r->p & SET_C));
}

static opcode_t opcode_table[256] = {
    {BRK,  IMP_0, 0}, {ORA, X_IND, 0}, {JAM,   IMP_0, 0}, {SLO, X_IND, 0},	{NOP1, ZPG_0, 0}, {ORA, ZPG_0, 0}, {ASL, ZPG_0, 0}, {SLO, ZPG_0, 0},	{PHP, IMP_0, 0}, {ORA,  IMM_0, 0}, {ASL, ACC_0, 0}, {ANC , IMM_0, 0}, {NOP1, ABS_0, 0}, {ORA, ABS_0, 0}, {ASL, ABS_0, 0}, {SLO, ABS_0, 0},
    {BPL,  REL_0, 0}, {ORA, IND_Y, 0}, {JAM,   IMP_0, 0}, {SLO, IND_Y, 1},	{NOP1, ZPG_X, 0}, {ORA, ZPG_X, 0}, {ASL, ZPG_X, 0}, {SLO, ZPG_X, 0},	{CLC, IMP_0, 0}, {ORA,  ABS_Y, 0}, {NOP, IMP_0, 0}, {SLO , ABS_Y, 1}, {NOP1, ABS_X, 0}, {ORA, ABS_X, 0}, {ASL, ABS_X, 1}, {SLO, ABS_X, 1},
    {JSR,  IMP_0, 0}, {AND, X_IND, 0}, {JAM,   IMP_0, 0}, {RLA, X_IND, 0},	{BIT,  ZPG_0, 0}, {AND, ZPG_0, 0}, {ROL, ZPG_0, 0}, {RLA, ZPG_0, 0},	{PLP, IMP_0, 0}, {AND,  IMM_0, 0}, {ROL, ACC_0, 0}, {ANC , IMM_0, 0}, {BIT,  ABS_0, 0}, {AND, ABS_0, 0}, {ROL, ABS_0, 0}, {RLA, ABS_0, 0},
    {BMI,  REL_0, 0}, {AND, IND_Y, 0}, {JAM,   IMP_0, 0}, {RLA, IND_Y, 1},	{NOP1, ZPG_X, 0}, {AND, ZPG_X, 0}, {ROL, ZPG_X, 0}, {RLA, ZPG_X, 0},	{SEC, IMP_0, 0}, {AND,  ABS_Y, 0}, {NOP, IMP_0, 0}, {RLA , ABS_Y, 1}, {NOP1, ABS_X, 0}, {AND, ABS_X, 0}, {ROL, ABS_X, 1}, {RLA, ABS_X, 1},
    {RTI,  IMP_0, 0}, {EOR, X_IND, 0}, {JAM,   IMP_0, 0}, {SRE, X_IND, 0},	{NOP1, ZPG_0, 0}, {EOR, ZPG_0, 0}, {LSR, ZPG_0, 0}, {SRE, ZPG_0, 0},	{PHA, IMP_0, 0}, {EOR,  IMM_0, 0}, {LSR, ACC_0, 0}, {ALR , IMM_0, 0}, {JMP,  ABS_0, 0}, {EOR, ABS_0, 0}, {LSR, ABS_0, 0}, {SRE, ABS_0, 0},
    {BVC,  REL_0, 0}, {EOR, IND_Y, 0}, {JAM,   IMP_0, 0}, {SRE, IND_Y, 1},	{NOP1, ZPG_X, 0}, {EOR, ZPG_X, 0}, {LSR, ZPG_X, 0}, {SRE, ZPG_X, 0},	{CLI, IMP_0, 0}, {EOR,  ABS_Y, 0}, {NOP, IMP_0, 0}, {SRE , ABS_Y, 1}, {NOP1, ABS_X, 0}, {EOR, ABS_X, 0}, {LSR, ABS_X, 1}, {SRE, ABS_X, 1},
    {RTS,  IMP_0, 0}, {ADC, X_IND, 0}, {JAM,   IMP_0, 0}, {RRA, X_IND, 0},	{NOP1, ZPG_0, 0}, {ADC, ZPG_0, 0}, {ROR, ZPG_0, 0}, {RRA, ZPG_0, 0},	{PLA, IMP_0, 0}, {ADC,  IMM_0, 0}, {ROR, ACC_0, 0}, {ARR , IMM_0, 0}, {JMP,  IND_0, 0}, {ADC, ABS_0, 0}, {ROR, ABS_0, 0}, {RRA, ABS_0, 0},
    {BVS,  REL_0, 0}, {ADC, IND_Y, 0}, {JAM,   IMP_0, 0}, {RRA, IND_Y, 1},	{NOP1, ZPG_X, 0}, {ADC, ZPG_X, 0}, {ROR, ZPG_X, 0}, {RRA, ZPG_X, 0},	{SEI, IMP_0, 0}, {ADC,  ABS_Y, 0}, {NOP, IMP_0, 0}, {RRA , ABS_Y, 1}, {NOP1, ABS_X, 0}, {ADC, ABS_X, 0}, {ROR, ABS_X, 1}, {RRA, ABS_X, 1},
    {NOP2, IMM_0, 0}, {STA, X_IND, 1}, {NOP2,  IMM_0, 0}, {SAX, X_IND, 0},	{STY,  ZPG_0, 0}, {STA, ZPG_0, 1}, {STX, ZPG_0, 0}, {SAX, ZPG_0, 0},	{DEY, IMP_0, 0}, {NOP2, IMM_0, 0}, {TXA, IMP_0, 0}, {ANE , IMM_0, 0}, {STY,  ABS_0, 0}, {STA, ABS_0, 0}, {STX, ABS_0, 0}, {SAX, ABS_0, 0},
    {BCC,  REL_0, 0}, {STA, IND_Y, 1}, {JAM,   IMP_0, 0}, {SHA, IND_Y, 1},	{STY,  ZPG_X, 0}, {STA, ZPG_X, 1}, {STX, ZPG_Y, 0}, {SAX, ZPG_Y, 0},	{TYA, IMP_0, 0}, {STA,  ABS_Y, 1}, {TXS, IMP_0, 0}, {TAS , ABS_Y, 1}, {SHY,  ABS_X, 1}, {STA, ABS_X, 1}, {SHX, ABS_Y, 1}, {SHA, ABS_Y, 1},
    {LDY,  IMM_0, 0}, {LDA, X_IND, 0}, {LDX,   IMM_0, 0}, {LAX, X_IND, 0},	{LDY,  ZPG_0, 0}, {LDA, ZPG_0, 0}, {LDX, ZPG_0, 0}, {LAX, ZPG_0, 0},	{TAY, IMP_0, 0}, {LDA,  IMM_0, 0}, {TAX, IMP_0, 0}, {LXA , IMM_0, 0}, {LDY,  ABS_0, 0}, {LDA, ABS_0, 0}, {LDX, ABS_0, 0}, {LAX, ABS_0, 0},
    {BCS,  REL_0, 0}, {LDA, IND_Y, 0}, {JAM,   IMP_0, 0}, {LAX, IND_Y, 0},	{LDY,  ZPG_X, 0}, {LDA, ZPG_X, 0}, {LDX, ZPG_Y, 0}, {LAX, ZPG_Y, 0},	{CLV, IMP_0, 0}, {LDA,  ABS_Y, 0}, {TSX, IMP_0, 0}, {LAS , ABS_Y, 0}, {LDY,  ABS_X, 0}, {LDA, ABS_X, 0}, {LDX, ABS_Y, 0}, {LAX, ABS_Y, 0},
    {CPY,  IMM_0, 0}, {CMP, X_IND, 0}, {NOP2,  IMM_0, 0}, {DCP, X_IND, 0},	{CPY,  ZPG_0, 0}, {CMP, ZPG_0, 0}, {DEC, ZPG_0, 0}, {DCP, ZPG_0, 0},	{INY, IMP_0, 0}, {CMP,  IMM_0, 0}, {DEX, IMP_0, 0}, {SBX , IMM_0, 0}, {CPY,  ABS_0, 0}, {CMP, ABS_0, 0}, {DEC, ABS_0, 0}, {DCP, ABS_0, 0},
    {BNE,  REL_0, 0}, {CMP, IND_Y, 0}, {JAM,   IMP_0, 0}, {DCP, IND_Y, 1},	{NOP1, ZPG_X, 0}, {CMP, ZPG_X, 0}, {DEC, ZPG_X, 0}, {DCP, ZPG_X, 0},	{CLD, IMP_0, 0}, {CMP,  ABS_Y, 0}, {NOP, IMP_0, 0}, {DCP , ABS_Y, 1}, {NOP1, ABS_X, 0}, {CMP, ABS_X, 0}, {DEC, ABS_X, 1}, {DCP, ABS_X, 1},
    {CPX,  IMM_0, 0}, {SBC, X_IND, 0}, {NOP2,  IMM_0, 0}, {ISC, X_IND, 0},	{CPX,  ZPG_0, 0}, {SBC, ZPG_0, 0}, {INC, ZPG_0, 0}, {ISC, ZPG_0, 0},	{INX, IMP_0, 0}, {SBC,  IMM_0, 0}, {NOP, IMP_0, 0}, {USBC, IMM_0, 0}, {CPX,  ABS_0, 0}, {SBC, ABS_0, 0}, {INC, ABS_0, 0}, {ISC, ABS_0, 0},
    {BEQ,  REL_0, 0}, {SBC, IND_Y, 0}, {JAM,   IMP_0, 0}, {ISC, IND_Y, 1},	{NOP1, ZPG_X, 0}, {SBC, ZPG_X, 0}, {INC, ZPG_X, 0}, {ISC, ZPG_X, 0},	{SED, IMP_0, 0}, {SBC,  ABS_Y, 0}, {NOP, IMP_0, 0}, {ISC , ABS_Y, 1}, {NOP1, ABS_X, 0}, {SBC, ABS_X, 0}, {INC, ABS_X, 1}, {ISC, ABS_X, 1}
};

void r2A03_print(r2A03_t* r){
    printf("pc: %04X s: %02X p: %02X a: %02X x: %02X y: %02X\n", r->pc, r->s, r->p, r->a, r->x, r->y);
    printf("cycles: %d\n", r->cycles);
    printf(
        "N: %d V: %d B: %d D: %d I: %d Z: %d C: %d\n\n",
        (bool)(r->p & SET_N), (bool)(r->p & SET_V), (bool)(r->p & SET_B), (bool)(r->p & SET_D),
        (bool)(r->p & SET_I), (bool)(r->p & SET_Z), (bool)(r->p & SET_C)
    );
}

void r2A03_init(r2A03_t* r){
    r->p = SET_D | SET_U | SET_I;
    r->s = 0;
    r->a = 0;
    r->x = 0;
    r->y = 0;
    r->pc = 0;
}

void r2A03_reset(r2A03_t* r){
    r->a = 0;
    r->x = 0;
    r->y = 0;
    r->s -= 3;
    u8 pc_lo = read_byte(0xFFFC);
    u8 pc_hi = read_byte(0xFFFD);
    r->pc = pc_lo | (pc_hi << 8);
    r->p = SET_I;
}

void r2A03_nmi(r2A03_t* r){
    fetch;
    fetch;
    r->pc -= 2;
    push(r->pc >> 8);
    push(r->pc & 0xFF);
    push(r->p & CLEAR_B);
    u8 lsb = read_byte(0xFFFA);
    u8 msb = read_byte(0xFFFB);
    r->pc = lsb | (msb << 8);
    r->p |= SET_I;
}

void r2A03_irq(r2A03_t* r){
    fetch;
    fetch;
    r->pc -= 2;
    push(r->pc >> 8);
    push(r->pc & 0xFF);
    push(r->p & CLEAR_B);
    u8 lsb = read_byte(0xFFFE);
    u8 msb = read_byte(0xFFFF);
    r->pc = lsb | (msb << 8);
    r->p |= SET_I;
}

bool r2A03_interrupt_enabled(r2A03_t* r){
    return !(r->p & SET_I);
}

void r2A03_step(r2A03_t* r){
    u8 opcode = fetch;

    opcode_t op_info = opcode_table[opcode];
    opcodePtr op_impl = op_info.func;
    OPERAND type = op_info.operand;
    bool op_slow = op_info.slow;

    switch (type){
        case ACC_0:
        {
            r->in_mem = false;
            r->op_arg = r->a;
        }
        break;

        case ABS_0:
        {
            r->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = fetch;
            r->mem_addr = addr_lo | (addr_hi << 8);
        }
        break;

        case ABS_X:
        {
            r->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = fetch;
            r->mem_addr = addr_lo | (addr_hi << 8);
            if(op_slow || (u8)r->mem_addr + r->x > 0xFF){
                u8 tmp_lo = r->mem_addr + r->x;
                dummy_read((r->mem_addr & 0xFF00) | tmp_lo);
            }
            r->mem_addr += r->x;
        }
        break;

        case ABS_Y:
        {
            r->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = fetch;
            r->mem_addr = addr_lo | (addr_hi << 8);
            if(op_slow || (u8)r->mem_addr + r->y > 0xFF){
                u8 tmp_lo = r->mem_addr + r->y;
                dummy_read((r->mem_addr & 0xFF00) | tmp_lo);
            }
            r->mem_addr += r->y;
        }
        break;

        case REL_0:
        {
            r->op_arg = fetch;
        }
        break;

        case IND_Y:
        {
            r->in_mem = true;
            u8 ll = fetch;
            u8 addr_lo = read_byte(ll);
            u8 addr_hi = read_byte((u8)(ll + 1));
            r->mem_addr = addr_lo | (addr_hi << 8);
            if(op_slow || (u8)r->mem_addr + r->y > 0xFF){
                u8 tmp_lo = r->mem_addr + r->y;
                dummy_read((r->mem_addr & 0xFF00) | tmp_lo);
            }
            r->mem_addr += r->y;
        }
        break;

        case IMP_0:
        {
        }
        break;

        case IMM_0:
        {
            r->in_mem = false;
            r->op_arg = fetch;
        }
        break;

        case X_IND:
        {
            r->in_mem = true;
            u8 ll = fetch;
            u8 zpg = ll + r->x;
            dummy_read(ll);
            u8 addr_lo = read_byte(zpg);
            u8 addr_hi = read_byte((u8)(zpg + 1));
            r->mem_addr = addr_lo | (addr_hi << 8); 
        }
        break;

        case ZPG_0:
        {
            r->in_mem = true;
            r->mem_addr = fetch;
        }
        break;

        case ZPG_X:
        {
            r->in_mem = true;
            r->mem_addr = fetch;
            dummy_read(r->mem_addr);
            r->mem_addr = (u8)(r->mem_addr + r->x);
        }
        break;

        case ZPG_Y:
        {
            r->in_mem = true;
            r->mem_addr = fetch;
            dummy_read(r->mem_addr);
            r->mem_addr = (u8)(r->mem_addr + r->y);
        }
        break;

        case IND_0:
        {
            r->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = fetch;
            r->mem_addr = addr_lo | (addr_hi << 8);
            addr_lo = read_byte(r->mem_addr);
            r->mem_addr = (r->mem_addr & 0xFF00) | (((r->mem_addr & 0xFF) + 1) & 0xFF);
            addr_hi = read_byte(r->mem_addr);
            r->mem_addr = addr_lo | (addr_hi << 8);
        }
        break;

        default:
        printf("MODE %d NOT IMPLEMENTED!\n", type);
        return;
    }

    (*op_impl)(r);
}