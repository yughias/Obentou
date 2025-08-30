#include "cpus/m6502.h"

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

typedef void (*opcodePtr)(m6502_t*);

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

static u8 inline m6502_read_byte(m6502_t* m, u16 addr){
    u8 out = m->read(m->ctx, addr);
    m->cycles += 1;
    return out;
}

static void inline m6502_dummy_read(m6502_t* m, u16 addr){
    #ifndef M6502_LO_ACC
        m->read(m->ctx, addr);
    #endif
    m->cycles += 1;
}

#define read_byte(addr) m6502_read_byte(m, addr)
#define dummy_read(addr) m6502_dummy_read(m, addr)
#define write_byte(addr, byte) m->write(m->ctx, addr, byte); m->cycles += 1
#define fetch read_byte(m->pc); m->pc += 1
#define calculate_n(x) (x) & 0x80 ? (m->p |= SET_N) : (m->p &= CLEAR_N)
#define calculate_z(x) !((u8)x) ? (m->p |= SET_Z) : (m->p &= CLEAR_Z)
#define change_c(x) (x) ? (m->p |= SET_C) : (m->p &= CLEAR_C)
#define change_v(x) (x) ? (m->p |= SET_V) : (m->p &= CLEAR_V)
#define write_back(x) if(m->in_mem) { write_byte(m->mem_addr, (x)); } else m->a = (x)
#define get_arg if(m->in_mem) { m->op_arg = read_byte(m->mem_addr); }
#define push(x) write_byte(m->s | 0x100, x); m->s -= 1
#define pop read_byte((++m->s) | 0x100);
#define branch_on(arg, cond) if(cond){ i16 ext_arg = (i16)(i8)arg; u16 new_pc = m->pc + ext_arg; dummy_read(m->pc); if(new_pc >> 8 != m->pc >> 8){ dummy_read((m->pc & 0xFF00) | (new_pc & 0xFF) ); } m->pc = new_pc; } 
#define ld(x) if(m->in_mem) { m->op_arg = read_byte(m->mem_addr); } x = m->op_arg; calculate_n(x); calculate_z(x)

static void BRK(m6502_t* m) { 
    fetch;
    push(m->pc >> 8);
    push(m->pc & 0xFF);
    push(m->p | SET_B);
    u8 lsb = read_byte(0xFFFE);
    u8 msb = read_byte(0xFFFF);
    m->pc = lsb | (msb << 8);
    m->p |= SET_I;
}

static void ORA(m6502_t* m) {
    get_arg;
    m->a |= m->op_arg;
    calculate_n(m->a);
    calculate_z(m->a);
}

static void JAM(m6502_t* m) {  
    dummy_read(m->pc);  
    dummy_read(0xFFFF);
    dummy_read(0xFFFE);
    dummy_read(0xFFFE);
    dummy_read(0xFFFF);
    // ideally it should be an endless loop
    for(int i = 0; i < 5; i++)
        dummy_read(0xFFFF);
}

static void SLO(m6502_t* m) {
    get_arg;
    change_c(m->op_arg & 0x80);
    write_byte(m->mem_addr, m->op_arg);
    m->op_arg <<= 1;
    m->a |= m->op_arg;
    write_byte(m->mem_addr, m->op_arg);
    calculate_n(m->a);
    calculate_z(m->a);
}

static void NOP(m6502_t* m) {
    dummy_read(m->pc);
}

static void NOP1(m6502_t* m) {
    dummy_read(m->mem_addr);
}

static void NOP2(m6502_t* m) {
}

static void ASL(m6502_t* m) { 
    get_arg;
    change_c(m->op_arg & 0x80);
    if(m->in_mem) {
        write_byte(m->mem_addr, m->op_arg);
    } else {
        dummy_read(m->pc);
    }
    m->op_arg <<= 1;
    calculate_n(m->op_arg);
    calculate_z(m->op_arg);
    write_back(m->op_arg);
}

static void PHP(m6502_t* m) { 
    dummy_read(m->pc);
    push(m->p | SET_B | SET_U);
}

static void ANC(m6502_t* m) {
    get_arg;
    m->a &= m->op_arg;
    change_c(m->a & 0x80);
    calculate_n(m->a);
    calculate_z(m->a);
}

static void BPL(m6502_t* m) {
    branch_on(m->op_arg, !(m->p & SET_N));
}

static void CLC(m6502_t* m) { 
    change_c(0);
    dummy_read(m->pc);
}

static void JSR(m6502_t* m) { 
    u8 l = fetch;
    dummy_read(m->s | 0x100);
    push(m->pc >> 8);
    push(m->pc & 0xFF);
    u8 h = read_byte(m->pc);
    m->pc = (h << 8) | l;
}

static void AND(m6502_t* m) {
    get_arg;
    m->a &= m->op_arg;
    calculate_n(m->a);
    calculate_z(m->a);
}

static void BIT(m6502_t* m) {
    get_arg;
    m->p &= CLEAR_N & CLEAR_V;
    m->p |= m->op_arg & 0xC0;
    calculate_z(m->op_arg & m->a);
}

static void PLP(m6502_t* m) {
    dummy_read(m->pc);
    dummy_read(m->s | 0x100);
    m->p = pop(m);
    m->p |= SET_U;
    m->p &= CLEAR_B;
}

static void BMI(m6502_t* m) { 
    branch_on(m->op_arg, m->p & SET_N);
}

static void SEC(m6502_t* m) { 
    change_c(1);
    dummy_read(m->pc);
}

static void RTI(m6502_t* m) {
    dummy_read(m->pc);
    dummy_read(m->s | 0x100);
    m->p = read_byte((m->s + 1) | 0x100);
    u8 pcl = read_byte((m->s + 2) | 0x100);
    m->s += 3;
    u8 pch = read_byte(m->s | 0x100);
    m->p &= CLEAR_B;
    m->p |= SET_U;
    m->pc = pcl | (pch << 8); 
}

static void EOR(m6502_t* m) {
    get_arg;
    m->a ^= m->op_arg;
    calculate_n(m->a);
    calculate_z(m->a);
}

static void LSR(m6502_t* m) { 
    get_arg;
    change_c(m->op_arg & 1);
    if(m->in_mem){
        write_byte(m->mem_addr, m->op_arg);
    } else {
        dummy_read(m->pc);
    }
    m->op_arg >>= 1;
    m->p &= CLEAR_N;
    calculate_z(m->op_arg);
    write_back(m->op_arg);
}

static void PHA(m6502_t* m) { 
    dummy_read(m->pc);
    push(m->a);
}

static void ALR(m6502_t* m) {
    get_arg;
    m->a &= m->op_arg;
    change_c(m->a & 1);
    m->a >>= 1;
    calculate_n(m->a);
    calculate_z(m->a);
}

static void JMP(m6502_t* m) { 
    m->pc = m->mem_addr;
}

static void BVC(m6502_t* m) { 
    branch_on(m->op_arg, !(m->p & SET_V));
}

static void CLI(m6502_t* m) { 
    m->p &= CLEAR_I;
    dummy_read(m->pc);
}

static void RTS(m6502_t* m) { 
    dummy_read(m->pc);
    dummy_read(m->s | 0x100);
    u8 pcl = pop(m);
    u8 pch = pop(m);
    m->pc = pcl | (pch << 8); 
    dummy_read(m->pc);
    m->pc += 1;
}

static void ADC(m6502_t* m) {
    get_arg;
    u16 ires = (i16)(i8)m->op_arg + (i16)(i8)m->a + (bool)(m->p & SET_C);
    u16 ures = m->op_arg + m->a + (bool)(m->p & SET_C);
    m->a = ures;
    change_c(ures > 0xFF);
    change_v(((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80))); 
    calculate_n(m->a);
    calculate_z(m->a);
}

static void PLA(m6502_t* m) { 
    dummy_read(m->pc);
    dummy_read(m->s | 0x100);
    m->a = pop(m);
    calculate_n(m->a);
    calculate_z(m->a);
}

static void ARR(m6502_t* m) {
    get_arg;

    bool carry = m->p & SET_C;
    u8 tmp = carry << 7;
    m->a &= m->op_arg;
    change_c(m->a & 0x80);
    m->a = (m->a >> 1) | tmp;
    carry = m->p & SET_C;
    change_v(carry ^ ((m->a >> 5) & 1));
    
    calculate_n(m->a);
    calculate_z(m->a);
}

static void BVS(m6502_t* m) { 
    branch_on(m->op_arg, m->p & SET_V);
}

static void SEI(m6502_t* m) { 
    m->p |= SET_I;
    dummy_read(m->pc);
}

static void STA(m6502_t* m) { 
    write_byte(m->mem_addr, (m->a));
}

static void SAX(m6502_t* m) {
    u8 tmp = m->a & m->x;
    write_byte(m->mem_addr, tmp);
}

static void DEY(m6502_t* m) {
    m->y -= 1;
    calculate_n(m->y);
    calculate_z(m->y);
    dummy_read(m->pc);
}

static void TXA(m6502_t* m) {
    m->a = m->x;
    calculate_n(m->a);
    calculate_z(m->a);
    dummy_read(m->pc);
}

static void ANE(m6502_t* m) {
    m->a |= MAGIC_BYTE;
    m->a &= m->x;
    m->a &= m->op_arg;
    calculate_n(m->a);
    calculate_z(m->a);
}

static void TYA(m6502_t* m) { 
    m->a = m->y;
    calculate_n(m->a);
    calculate_z(m->a);
    dummy_read(m->pc);
}

static void TXS(m6502_t* m) {
    m->s = m->x;
    dummy_read(m->pc);
}

static void TAS(m6502_t* m) {
    u8 no_y_addr = (m->mem_addr - m->y) & 0xFF;
    bool swap = no_y_addr + m->y > 0xFF;
    u8 hi = m->mem_addr >> 8;
    m->s = m->a & m->x; 
    if(swap){
        hi &= m->s;
        m->mem_addr = (m->mem_addr & 0xFF) | (hi << 8);
    }
    u8 tmp = m->s & (hi + !swap);
    write_byte(m->mem_addr, tmp);
}

static void SHY(m6502_t* m) {
    u8 no_x_addr = (m->mem_addr - m->x) & 0xFF;
    bool swap = no_x_addr + m->x > 0xFF;
    u8 hi = m->mem_addr >> 8;
    if(swap){
        hi &= m->y;
        m->mem_addr = (m->mem_addr & 0xFF) | (hi << 8);
    }
    u8 tmp = m->y & (hi + !swap);
    write_byte(m->mem_addr, tmp);   
}

static void SHX(m6502_t* m) {
    u8 no_y_addr = (m->mem_addr - m->y) & 0xFF;
    bool swap = no_y_addr + m->y > 0xFF;
    u8 hi = m->mem_addr >> 8;
    if(swap){
        hi &= m->x;
        m->mem_addr = (m->mem_addr & 0xFF) | (hi << 8);
    }
    u8 tmp = m->x & (hi + !swap);
    write_byte(m->mem_addr, tmp);   
}

static void LDY(m6502_t* m) { ld(m->y); }
static void LDA(m6502_t* m) { ld(m->a); }
static void LDX(m6502_t* m) { ld(m->x); }

static void TAY(m6502_t* m) {
    m->y = m->a;
    calculate_n(m->a);
    calculate_z(m->a);
    dummy_read(m->pc);
}

static void TAX(m6502_t* m) {
    m->x = m->a;
    calculate_n(m->a);
    calculate_z(m->a);
    dummy_read(m->pc);
}

static void LXA(m6502_t* m) {
    m->a |= MAGIC_BYTE;
    m->a &= m->op_arg;
    m->x = m->a;
    calculate_n(m->a);
    calculate_z(m->a);    
}

static void CLV(m6502_t* m) {
    m->p &= CLEAR_V;
    dummy_read(m->pc);
}

static void TSX(m6502_t* m) {
    m->x = m->s;
    calculate_n(m->s);
    calculate_z(m->s);
    dummy_read(m->pc);
}

static void LAS(m6502_t* m) {
    get_arg;
    m->a = m->op_arg & m->s;
    m->x = m->a;
    m->s = m->a;
    calculate_n(m->a);
    calculate_z(m->a);
}

static void CPY(m6502_t* m) {
    get_arg;
    m->op_arg = ~m->op_arg;
    u16 ures = m->y + m->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void CMP(m6502_t* m) { 
    get_arg;
    m->op_arg = ~m->op_arg;
    u16 ures = m->a + m->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void DEC(m6502_t* m) {
    get_arg;
    write_byte(m->mem_addr, m->op_arg);
    m->op_arg -= 1;
    calculate_n(m->op_arg);
    calculate_z(m->op_arg);
    write_byte(m->mem_addr, m->op_arg);
}

static void INY(m6502_t* m) { 
    dummy_read(m->pc);
    m->y += 1;
    calculate_n(m->y);
    calculate_z(m->y);
}

static void DEX(m6502_t* m) { 
    dummy_read(m->pc);
    m->x -= 1;
    calculate_n(m->x);
    calculate_z(m->x);
}

static void SBX(m6502_t* m) {
    u8 tmp = m->a & m->x;

    m->op_arg = ~m->op_arg;
    u16 ures = tmp + m->op_arg + 1;
    m->x = ures;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void BNE(m6502_t* m) {
    branch_on(m->op_arg, !(m->p & SET_Z));
}

static void CLD(m6502_t* m) { 
    dummy_read(m->pc);
    m->p &= CLEAR_D;
}

static void CPX(m6502_t* m) {
    get_arg;
    m->op_arg = ~m->op_arg;
    u16 ures = m->x + m->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void SBC(m6502_t* m) { 
    get_arg;
    m->op_arg = ~m->op_arg;
    u16 ires = (i16)(i8)m->op_arg + (i16)(i8)m->a + (bool)(m->p & SET_C);
    u16 ures = m->op_arg + m->a + (bool)(m->p & SET_C);
    m->a = ures;
    change_c(ures > 0xFF);
    change_v(((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80))); 
    calculate_n(m->a);
    calculate_z(m->a);
}

static void INC(m6502_t* m) { 
    get_arg;
    write_byte(m->mem_addr, m->op_arg);
    m->op_arg += 1;
    calculate_n(m->op_arg);
    calculate_z(m->op_arg);
    write_byte(m->mem_addr, m->op_arg);
}

static void INX(m6502_t* m) { 
    dummy_read(m->pc);
    m->x += 1;
    calculate_n(m->x);
    calculate_z(m->x);
}

static void USBC(m6502_t* m) {
    SBC(m);
}

static void BEQ(m6502_t* m) {
    branch_on(m->op_arg, m->p & SET_Z);
}

static void SED(m6502_t* m) { 
    dummy_read(m->pc);
    m->p |= SET_D;
}

static void ISC(m6502_t* m) {
    get_arg;
    write_byte(m->mem_addr, m->op_arg);
    m->op_arg += 1;
    write_byte(m->mem_addr, m->op_arg);

    m->op_arg = ~m->op_arg;
    u16 ires = (i16)(i8)m->op_arg + (i16)(i8)m->a + (bool)(m->p & SET_C);
    u16 ures = m->op_arg + m->a + (bool)(m->p & SET_C);
    m->a = ures;
    change_c(ures > 0xFF);
    change_v(((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80))); 
    calculate_n(m->a);
    calculate_z(m->a);    
}

static void DCP(m6502_t* m) {
    get_arg;
    write_byte(m->mem_addr, m->op_arg);
    m->op_arg -= 1;
    write_byte(m->mem_addr, m->op_arg);

    m->op_arg = ~m->op_arg;
    u16 ures = m->a + m->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void RLA(m6502_t* m) {
    get_arg;
    bool carry = m->p & SET_C;
    change_c(m->op_arg & 0x80);
    write_byte(m->mem_addr, m->op_arg);
    m->op_arg = (m->op_arg << 1) | carry;
    m->a &= m->op_arg;
    write_byte(m->mem_addr, m->op_arg);
    calculate_n(m->a);
    calculate_z(m->a);
}

static void SRE(m6502_t* m) {
    get_arg;
    change_c(m->op_arg & 1);
    write_byte(m->mem_addr, m->op_arg);
    m->op_arg >>= 1;
    write_byte(m->mem_addr, m->op_arg);
    m->a ^= m->op_arg;
    calculate_n(m->a);
    calculate_z(m->a);
}

static void RRA(m6502_t* m) {
    get_arg;
    bool carry = m->p & SET_C;
    change_c(m->op_arg & 1);
    write_byte(m->mem_addr, m->op_arg);
    m->op_arg = (m->op_arg >> 1) | (carry << 7);
    write_byte(m->mem_addr, m->op_arg);

    u16 ires = (i16)(i8)m->op_arg + (i16)(i8)m->a + (bool)(m->p & SET_C);
    u16 ures = m->op_arg + m->a + (bool)(m->p & SET_C);
    m->a = ures;
    change_c(ures > 0xFF);
    change_v(((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80))); 
    calculate_n(m->a);
    calculate_z(m->a);
}

static void ROL(m6502_t* m) {
    get_arg;
    bool c = m->op_arg & 0x80;
    if(m->in_mem){
        write_byte(m->mem_addr, m->op_arg);
    } else {
        dummy_read(m->pc);
    }
    m->op_arg = (m->op_arg << 1) | ((bool)(m->p & SET_C));
    change_c(c);
    calculate_n(m->op_arg);
    calculate_z(m->op_arg);
    write_back(m->op_arg);
}

static void ROR(m6502_t* m) {
    get_arg;
    bool c = m->op_arg & 1;
    if(m->in_mem){
        write_byte(m->mem_addr, m->op_arg);
    } else {
        dummy_read(m->pc);
    }
    m->op_arg = (m->op_arg >> 1) | (((bool)(m->p & SET_C)) << 7);
    change_c(c);
    calculate_n(m->op_arg);
    calculate_z(m->op_arg);
    write_back(m->op_arg);
}

static void SHA(m6502_t* m) {
    u8 no_y_addr = (m->mem_addr - m->y) & 0xFF;
    bool swap = no_y_addr + m->y > 0xFF;
    u8 hi = m->mem_addr >> 8;
    u8 tmp = m->a & m->x; 
    if(swap){
        hi &= tmp;
        m->mem_addr = (m->mem_addr & 0xFF) | (hi << 8);
    }
    tmp &= (hi + !swap);
    write_byte(m->mem_addr, tmp);
}

static void LAX(m6502_t* m) {
    get_arg;
    m->a = m->x = m->op_arg;
    calculate_n(m->a);
    calculate_z(m->a);
}

static void STY(m6502_t* m) { 
    write_byte(m->mem_addr, (m->y));
}

static void STX(m6502_t* m) { 
    write_byte(m->mem_addr, (m->x));
}

static void BCS(m6502_t* m) {
    branch_on(m->op_arg, m->p & SET_C);
}

static void BCC(m6502_t* m) {
    branch_on(m->op_arg, !(m->p & SET_C));
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

void m6502_print(m6502_t* m){
    printf("pc: %04X s: %02X p: %02X a: %02X x: %02X y: %02X\n", m->pc, m->s, m->p, m->a, m->x, m->y);
    printf("cycles: %d\n", m->cycles);
    printf(
        "N: %d V: %d B: %d D: %d I: %d Z: %d C: %d\n\n",
        (bool)(m->p & SET_N), (bool)(m->p & SET_V), (bool)(m->p & SET_B), (bool)(m->p & SET_D),
        (bool)(m->p & SET_I), (bool)(m->p & SET_Z), (bool)(m->p & SET_C)
    );
}

void m6502_init(m6502_t* m){
    m->p = SET_D | SET_U | SET_I;
    m->s = 0;
    m->a = 0;
    m->x = 0;
    m->y = 0;
    m->pc = 0;
}

void m6502_reset(m6502_t* m){
    m->a = 0;
    m->x = 0;
    m->y = 0;
    m->s -= 3;
    u8 pc_lo = m->read(m, 0xFFFC);
    u8 pc_hi = m->read(m, 0xFFFD);
    m->pc = pc_lo | (pc_hi << 8);
    m->p = SET_I;
}

void m6502_nmi(m6502_t* m){
    fetch;
    fetch;
    m->pc -= 2;
    push(m->pc >> 8);
    push(m->pc & 0xFF);
    push(m->p & CLEAR_B);
    u8 lsb = read_byte(0xFFFA);
    u8 msb = read_byte(0xFFFB);
    m->pc = lsb | (msb << 8);
    m->p |= SET_I;
}

void m6502_irq(m6502_t* m){
    fetch;
    fetch;
    m->pc -= 2;
    push(m->pc >> 8);
    push(m->pc & 0xFF);
    push(m->p & CLEAR_B);
    u8 lsb = read_byte(0xFFFE);
    u8 msb = read_byte(0xFFFF);
    m->pc = lsb | (msb << 8);
    m->p |= SET_I;
}

bool m6502_interrupt_enabled(m6502_t* m){
    return !(m->p & SET_I);
}

void m6502_step(m6502_t* m){
    u8 opcode = fetch;

    opcode_t op_info = opcode_table[opcode];
    opcodePtr op_impl = op_info.func;
    OPERAND type = op_info.operand;
    bool op_slow = op_info.slow;

    switch (type){
        case ACC_0:
        {
            m->in_mem = false;
            m->op_arg = m->a;
        }
        break;

        case ABS_0:
        {
            m->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = fetch;
            m->mem_addr = addr_lo | (addr_hi << 8);
        }
        break;

        case ABS_X:
        {
            m->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = fetch;
            m->mem_addr = addr_lo | (addr_hi << 8);
            if(op_slow || (u8)m->mem_addr + m->x > 0xFF){
                u8 tmp_lo = m->mem_addr + m->x;
                dummy_read((m->mem_addr & 0xFF00) | tmp_lo);
            }
            m->mem_addr += m->x;
        }
        break;

        case ABS_Y:
        {
            m->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = fetch;
            m->mem_addr = addr_lo | (addr_hi << 8);
            if(op_slow || (u8)m->mem_addr + m->y > 0xFF){
                u8 tmp_lo = m->mem_addr + m->y;
                dummy_read((m->mem_addr & 0xFF00) | tmp_lo);
            }
            m->mem_addr += m->y;
        }
        break;

        case REL_0:
        {
            m->op_arg = fetch;
        }
        break;

        case IND_Y:
        {
            m->in_mem = true;
            u8 ll = fetch;
            u8 addr_lo = read_byte(ll);
            u8 addr_hi = read_byte((u8)(ll + 1));
            m->mem_addr = addr_lo | (addr_hi << 8);
            if(op_slow || (u8)m->mem_addr + m->y > 0xFF){
                u8 tmp_lo = m->mem_addr + m->y;
                dummy_read((m->mem_addr & 0xFF00) | tmp_lo);
            }
            m->mem_addr += m->y;
        }
        break;

        case IMP_0:
        {
        }
        break;

        case IMM_0:
        {
            m->in_mem = false;
            m->op_arg = fetch;
        }
        break;

        case X_IND:
        {
            m->in_mem = true;
            u8 ll = fetch;
            u8 zpg = ll + m->x;
            dummy_read(ll);
            u8 addr_lo = read_byte(zpg);
            u8 addr_hi = read_byte((u8)(zpg + 1));
            m->mem_addr = addr_lo | (addr_hi << 8); 
        }
        break;

        case ZPG_0:
        {
            m->in_mem = true;
            m->mem_addr = fetch;
        }
        break;

        case ZPG_X:
        {
            m->in_mem = true;
            m->mem_addr = fetch;
            dummy_read(m->mem_addr);
            m->mem_addr = (u8)(m->mem_addr + m->x);
        }
        break;

        case ZPG_Y:
        {
            m->in_mem = true;
            m->mem_addr = fetch;
            dummy_read(m->mem_addr);
            m->mem_addr = (u8)(m->mem_addr + m->y);
        }
        break;

        case IND_0:
        {
            m->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = fetch;
            m->mem_addr = addr_lo | (addr_hi << 8);
            addr_lo = read_byte(m->mem_addr);
            m->mem_addr = (m->mem_addr & 0xFF00) | (((m->mem_addr & 0xFF) + 1) & 0xFF);
            addr_hi = read_byte(m->mem_addr);
            m->mem_addr = addr_lo | (addr_hi << 8);
        }
        break;

        default:
        printf("MODE %d NOT IMPLEMENTED!\n", type);
        return;
    }

    (*op_impl)(m);
}