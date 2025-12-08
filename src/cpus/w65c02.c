#include "cpus/w65c02.h"

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
    ZPG_I,
    ABS_X_I
} OPERAND;

typedef void (*opcodePtr)(w65c02_t*);

typedef struct opcode_t {
    opcodePtr func;
    OPERAND operand;
    bool slow;
} opcode_t;


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

static u8 inline w65c02_read_byte(w65c02_t* w, u16 addr){
    u8 out = w->read(w->ctx, addr);
    w->cycles += 1;
    return out;
}

static void inline w65c02_dummy_read(w65c02_t* w, u16 addr){
    #ifndef W65C02_LO_ACC
        w->read(w->ctx, addr);
    #endif
    w->cycles += 1;
}

#define read_byte(addr) w65c02_read_byte(w, addr)
#define dummy_read(addr) w65c02_dummy_read(w, addr)
#define write_byte(addr, byte) w->write(w->ctx, addr, byte); w->cycles += 1
#define fetch read_byte(w->pc); w->pc += 1
#define calculate_n(x) (x) & 0x80 ? (w->p |= SET_N) : (w->p &= CLEAR_N)
#define calculate_z(x) !((u8)x) ? (w->p |= SET_Z) : (w->p &= CLEAR_Z)
#define change_c(x) (x) ? (w->p |= SET_C) : (w->p &= CLEAR_C)
#define change_v(x) (x) ? (w->p |= SET_V) : (w->p &= CLEAR_V)
#define write_back(x) if(w->in_mem) { write_byte(w->mem_addr, (x)); } else w->a = (x)
#define get_arg if(w->in_mem) { w->op_arg = read_byte(w->mem_addr); }
#define push(x) write_byte(w->s | 0x100, x); w->s -= 1
#define pop read_byte((++w->s) | 0x100);
#define branch_on(arg, cond) if(cond){ i16 ext_arg = (i16)(i8)arg; u16 new_pc = w->pc + ext_arg; dummy_read(w->pc); if(new_pc >> 8 != w->pc >> 8){ dummy_read((w->pc & 0xFF00) | (new_pc & 0xFF) ); } w->pc = new_pc; } 
#define ld(x) if(w->in_mem) { w->op_arg = read_byte(w->mem_addr); } x = w->op_arg; calculate_n(x); calculate_z(x)
#define GEN_RMB(x) static void RMB ## x (w65c02_t* w){ w->op_arg = read_byte(w->mem_addr); dummy_read(w->mem_addr); write_byte(w->mem_addr, w->op_arg & ~(1 << x)); }
#define GEN_SMB(x) static void SMB ## x (w65c02_t* w){ w->op_arg = read_byte(w->mem_addr); dummy_read(w->mem_addr); write_byte(w->mem_addr, w->op_arg | (1 << x)); }

#define GEN_BBR(x) static void BBR ## x (w65c02_t* w){ \
    u8 zp = read_byte(w->op_arg); \
    write_byte(w->op_arg, zp); \
    u8 off = fetch; \
    u16 new_pc = (i16)w->pc + (i16)(i8)off; \
    if(!(zp & (1 << x))){ \
        dummy_read((new_pc & 0xFF) | (w->pc & 0xFF00)); \
        w->pc = new_pc; \
    } else { \
        dummy_read((new_pc & 0xFF) | (w->pc & 0xFF00)); \
    } \
} \

#define GEN_BBS(x) static void BBS ## x (w65c02_t* w){ \
    u8 zp = read_byte(w->op_arg); \
    write_byte(w->op_arg, zp); \
    u8 off = fetch; \
    u16 new_pc = (i16)w->pc + (i16)(i8)off; \
    if((zp & (1 << x))){ \
        dummy_read((new_pc & 0xFF) | (w->pc & 0xFF00)); \
        w->pc = new_pc; \
    } else { \
        dummy_read((new_pc & 0xFF) | (w->pc & 0xFF00)); \
    } \
} \


GEN_RMB(0)
GEN_RMB(1)
GEN_RMB(2)
GEN_RMB(3)
GEN_RMB(4)
GEN_RMB(5)
GEN_RMB(6)
GEN_RMB(7)
GEN_SMB(0)
GEN_SMB(1)
GEN_SMB(2)
GEN_SMB(3)
GEN_SMB(4)
GEN_SMB(5)
GEN_SMB(6)
GEN_SMB(7)

GEN_BBR(0)
GEN_BBR(1)
GEN_BBR(2)
GEN_BBR(3)
GEN_BBR(4)
GEN_BBR(5)
GEN_BBR(6)
GEN_BBR(7)
GEN_BBS(0)
GEN_BBS(1)
GEN_BBS(2)
GEN_BBS(3)
GEN_BBS(4)
GEN_BBS(5)
GEN_BBS(6)
GEN_BBS(7)

static void BRK(w65c02_t* w) { 
    fetch;
    push(w->pc >> 8);
    push(w->pc & 0xFF);
    push(w->p | SET_B);
    u8 lsb = read_byte(0xFFFE);
    u8 msb = read_byte(0xFFFF);
    w->pc = lsb | (msb << 8);
    w->p |= SET_I;
    w->p &= CLEAR_D;
}

static void ORA(w65c02_t* w) {
    get_arg;
    w->a |= w->op_arg;
    calculate_n(w->a);
    calculate_z(w->a);
}

static void NOP(w65c02_t* w) {
    dummy_read(w->pc);
}

static void ASL(w65c02_t* w) { 
    get_arg;
    change_c(w->op_arg & 0x80);
    if(w->in_mem) {
        dummy_read(w->mem_addr);
    } else {
        dummy_read(w->pc);
    }
    w->op_arg <<= 1;
    calculate_n(w->op_arg);
    calculate_z(w->op_arg);
    write_back(w->op_arg);
}

static void PHP(w65c02_t* w) { 
    dummy_read(w->pc);
    push(w->p | SET_B | SET_U);
}

static void BPL(w65c02_t* w) {
    branch_on(w->op_arg, !(w->p & SET_N));
}

static void CLC(w65c02_t* w) { 
    change_c(0);
    dummy_read(w->pc);
}

static void JSR(w65c02_t* w) { 
    u8 l = fetch;
    dummy_read(w->s | 0x100);
    push(w->pc >> 8);
    push(w->pc & 0xFF);
    u8 h = read_byte(w->pc);
    w->pc = (h << 8) | l;
}

static void AND(w65c02_t* w) {
    get_arg;
    w->a &= w->op_arg;
    calculate_n(w->a);
    calculate_z(w->a);
}

static void BIT(w65c02_t* w) {
    get_arg;
    if(w->in_mem){
        w->p &= CLEAR_N & CLEAR_V;
        w->p |= w->op_arg & 0xC0;
    }
    calculate_z(w->op_arg & w->a);
}

static void PLP(w65c02_t* w) {
    dummy_read(w->pc);
    dummy_read(w->s | 0x100);
    w->p = pop;
    w->p |= SET_U;
    w->p &= CLEAR_B;
}

static void BMI(w65c02_t* w) { 
    branch_on(w->op_arg, w->p & SET_N);
}

static void SEC(w65c02_t* w) { 
    change_c(1);
    dummy_read(w->pc);
}

static void RTI(w65c02_t* w) {
    dummy_read(w->pc);
    dummy_read(w->s | 0x100);
    w->p = read_byte((w->s + 1) | 0x100);
    u8 pcl = read_byte((w->s + 2) | 0x100);
    w->s += 3;
    u8 pch = read_byte(w->s | 0x100);
    w->p &= CLEAR_B;
    w->p |= SET_U;
    w->pc = pcl | (pch << 8); 
}

static void EOR(w65c02_t* w) {
    get_arg;
    w->a ^= w->op_arg;
    calculate_n(w->a);
    calculate_z(w->a);
}

static void LSR(w65c02_t* w) { 
    get_arg;
    change_c(w->op_arg & 1);
    if(w->in_mem){
        dummy_read(w->mem_addr);
    } else {
        dummy_read(w->pc);
    }
    w->op_arg >>= 1;
    w->p &= CLEAR_N;
    calculate_z(w->op_arg);
    write_back(w->op_arg);
}

static void PHA(w65c02_t* w) { 
    dummy_read(w->pc);
    push(w->a);
}

static void PHX(w65c02_t* w) { 
    dummy_read(w->pc);
    push(w->x);
}

static void PHY(w65c02_t* w) { 
    dummy_read(w->pc);
    push(w->y);
}

static void JMP(w65c02_t* w) { 
    w->pc = w->mem_addr;
}

static void BVC(w65c02_t* w) { 
    branch_on(w->op_arg, !(w->p & SET_V));
}

static void CLI(w65c02_t* w) { 
    w->p &= CLEAR_I;
    dummy_read(w->pc);
}

static void RTS(w65c02_t* w) { 
    dummy_read(w->pc);
    dummy_read(w->s | 0x100);
    u8 pcl = pop;
    u8 pch = pop;
    w->pc = pcl | (pch << 8); 
    dummy_read(w->pc);
    w->pc += 1;
}

static void ADC(w65c02_t* w) {
    get_arg;
    bool carry = w->p & SET_C;
    bool new_c;
    bool new_v;
    if(w->p & SET_D){
        w->slow_op ? dummy_read(0x7F) : dummy_read(w->mem_addr);
        u8 a0 = w->a & 0xF;
        u8 a1 = w->a >> 4;
        u8 b0 = w->op_arg & 0xF;
        u8 b1 = w->op_arg >> 4;
        u8 lo = a0 + b0 + carry;
        u8 hi = a1 + b1;
        bool half_carry = false;
        if(lo > 9){
            half_carry = true;
            hi += 1;
            lo += 6;
        }
        if(hi > 9){
            hi += 6;
        }
        w->a = (hi << 4) | (lo & 0x0F);
        new_c = hi > 9;
        /* 4 bit sign exstension */
        if(a1 & 0x08) a1 |= 0xF0;
        if(b1 & 0x08) b1 |= 0xF0;
        i8 ires = (i8)a1 + (i8)b1 + (i8)half_carry;
        new_v = ires < -8 || ires > 7;
    } else {
        u16 ires = (i16)(i8)w->op_arg + (i16)(i8)w->a + carry;
        u16 ures = w->op_arg + w->a + carry;
        w->a = ures;
        new_c = ures > 0xFF;
        new_v = ((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80));
    }
    change_c(new_c);
    change_v(new_v); 
    calculate_n(w->a);
    calculate_z(w->a);
}

static void PLA(w65c02_t* w) { 
    dummy_read(w->pc);
    dummy_read(w->s | 0x100);
    w->a = pop;
    calculate_n(w->a);
    calculate_z(w->a);
}

static void PLX(w65c02_t* w) { 
    dummy_read(w->pc);
    dummy_read(w->s | 0x100);
    w->x = pop;
    calculate_n(w->x);
    calculate_z(w->x);
}

static void PLY(w65c02_t* w) { 
    dummy_read(w->pc);
    dummy_read(w->s | 0x100);
    w->y = pop;
    calculate_n(w->y);
    calculate_z(w->y);
}


static void BVS(w65c02_t* w) { 
    branch_on(w->op_arg, w->p & SET_V);
}

static void BRA(w65c02_t* w) { 
    branch_on(w->op_arg, true);
}

static void SEI(w65c02_t* w) { 
    w->p |= SET_I;
    dummy_read(w->pc);
}

static void STA(w65c02_t* w) { 
    write_byte(w->mem_addr, (w->a));
}

static void STZ(w65c02_t* w) { 
    write_byte(w->mem_addr, 0);
}

static void DEY(w65c02_t* w) {
    w->y -= 1;
    calculate_n(w->y);
    calculate_z(w->y);
    dummy_read(w->pc);
}

static void TXA(w65c02_t* w) {
    w->a = w->x;
    calculate_n(w->a);
    calculate_z(w->a);
    dummy_read(w->pc);
}

static void TYA(w65c02_t* w) { 
    w->a = w->y;
    calculate_n(w->a);
    calculate_z(w->a);
    dummy_read(w->pc);
}

static void TXS(w65c02_t* w) {
    w->s = w->x;
    dummy_read(w->pc);
}

static void LDY(w65c02_t* w) { ld(w->y); }
static void LDA(w65c02_t* w) { ld(w->a); }
static void LDX(w65c02_t* w) { ld(w->x); }

static void TAY(w65c02_t* w) {
    w->y = w->a;
    calculate_n(w->a);
    calculate_z(w->a);
    dummy_read(w->pc);
}

static void TAX(w65c02_t* w) {
    w->x = w->a;
    calculate_n(w->a);
    calculate_z(w->a);
    dummy_read(w->pc);
}

static void CLV(w65c02_t* w) {
    w->p &= CLEAR_V;
    dummy_read(w->pc);
}

static void TSX(w65c02_t* w) {
    w->x = w->s;
    calculate_n(w->s);
    calculate_z(w->s);
    dummy_read(w->pc);
}

static void CPY(w65c02_t* w) {
    get_arg;
    w->op_arg = ~w->op_arg;
    u16 ures = w->y + w->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void CMP(w65c02_t* w) { 
    get_arg;
    w->op_arg = ~w->op_arg;
    u16 ures = w->a + w->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void DEC(w65c02_t* w) {
    get_arg;
    dummy_read(w->mem_addr);
    w->op_arg -= 1;
    calculate_n(w->op_arg);
    calculate_z(w->op_arg);
    write_back(w->op_arg);
}

static void INY(w65c02_t* w) { 
    dummy_read(w->pc);
    w->y += 1;
    calculate_n(w->y);
    calculate_z(w->y);
}

static void DEX(w65c02_t* w) { 
    dummy_read(w->pc);
    w->x -= 1;
    calculate_n(w->x);
    calculate_z(w->x);
}

static void BNE(w65c02_t* w) {
    branch_on(w->op_arg, !(w->p & SET_Z));
}

static void CLD(w65c02_t* w) { 
    dummy_read(w->pc);
    w->p &= CLEAR_D;
}

static void CPX(w65c02_t* w) {
    get_arg;
    w->op_arg = ~w->op_arg;
    u16 ures = w->x + w->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void SBC(w65c02_t* w) { 
    get_arg;
    bool carry = w->p & SET_C;
    bool new_c;
    bool new_v;
    if(w->p & SET_D){
        dummy_read(w->mem_addr);
        u8 a0 = w->a & 0xF;
        u8 b0 = w->op_arg & 0xF;
        u16 tmp = a0 - b0 - !carry;
        u16 res = w->a - w->op_arg - !carry;
        u16 bin_res = w->a + ~w->op_arg + carry;
        if(res & 0x8000)
            res -= 0x60;
        if(tmp & 0x8000)
            res -= 0x06;
        new_v = (w->a ^ bin_res) & (~w->op_arg ^bin_res) & 0x80;
        new_c = (u16)res <= (u16)w->a || (res & 0xff0) == 0xff0;
        w->a = res;
    } else {
        w->op_arg = ~w->op_arg;
        u16 ires = (i16)(i8)w->op_arg + (i16)(i8)w->a + carry;
        u16 ures = w->op_arg + w->a + carry;
        w->a = ures;
        new_c = ures > 0xFF;
        new_v = ((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80));
    }
    change_c(new_c);
    change_v(new_v); 
    calculate_n(w->a);
    calculate_z(w->a);
}

static void INC(w65c02_t* w) { 
    get_arg;
    dummy_read(w->mem_addr);
    w->op_arg += 1;
    calculate_n(w->op_arg);
    calculate_z(w->op_arg);
    write_back(w->op_arg);
}

static void INX(w65c02_t* w) { 
    dummy_read(w->pc);
    w->x += 1;
    calculate_n(w->x);
    calculate_z(w->x);
}

static void BEQ(w65c02_t* w) {
    branch_on(w->op_arg, w->p & SET_Z);
}

static void SED(w65c02_t* w) { 
    dummy_read(w->pc);
    w->p |= SET_D;
}

static void ROL(w65c02_t* w) {
    get_arg;
    bool c = w->op_arg & 0x80;
    if(w->in_mem){
        dummy_read(w->mem_addr);
    } else {
        dummy_read(w->pc);
    }
    w->op_arg = (w->op_arg << 1) | ((bool)(w->p & SET_C));
    change_c(c);
    calculate_n(w->op_arg);
    calculate_z(w->op_arg);
    write_back(w->op_arg);
}

static void ROR(w65c02_t* w) {
    get_arg;
    bool c = w->op_arg & 1;
    if(w->in_mem){
        dummy_read(w->mem_addr);
    } else {
        dummy_read(w->pc);
    }
    w->op_arg = (w->op_arg >> 1) | (((bool)(w->p & SET_C)) << 7);
    change_c(c);
    calculate_n(w->op_arg);
    calculate_z(w->op_arg);
    write_back(w->op_arg);
}

static void STY(w65c02_t* w) { 
    write_byte(w->mem_addr, (w->y));
}

static void STX(w65c02_t* w) { 
    write_byte(w->mem_addr, (w->x));
}

static void BCS(w65c02_t* w) {
    branch_on(w->op_arg, w->p & SET_C);
}

static void BCC(w65c02_t* w) {
    branch_on(w->op_arg, !(w->p & SET_C));
}

static void TSB(w65c02_t* w){
    get_arg;
    dummy_read(w->mem_addr);
    calculate_z(w->op_arg & w->a);
    write_byte(w->mem_addr, w->op_arg | w->a);
}

static void TRB(w65c02_t* w){
    get_arg;
    dummy_read(w->mem_addr);
    calculate_z(w->op_arg & w->a);
    write_byte(w->mem_addr, w->op_arg & ~w->a);
}

static void WAI(w65c02_t* w){
    w->wait = true;
}

static void STP(w65c02_t* w){
    w->stop = true;
}

static opcode_t opcode_table[256] =  {
    {BRK, IMP_0}, {ORA, X_IND   }, {NOP, IMP_0}, {NOP, X_IND},	{TSB, ZPG_0}, {ORA, ZPG_0}, {ASL, ZPG_0}, {RMB0, ZPG_0}, {PHP, IMP_0}, {ORA, IMM_0,  }, {ASL, ACC_0}, {NOP, IMM_0}, {TSB, ABS_0  }, {ORA, ABS_0,  }, {ASL, ABS_0,  }, {BBR0, REL_0},
    {BPL, REL_0}, {ORA, IND_Y   }, {ORA, ZPG_I}, {NOP, IND_Y},	{TRB, ZPG_0}, {ORA, ZPG_X}, {ASL, ZPG_X}, {RMB1, ZPG_0}, {CLC, IMP_0}, {ORA, ABS_Y,  }, {INC, ACC_0}, {NOP, ABS_Y}, {TRB, ABS_0  }, {ORA, ABS_X,  }, {ASL, ABS_X,  }, {BBR1, REL_0},
    {JSR, IMP_0}, {AND, X_IND   }, {NOP, IMP_0}, {NOP, X_IND},	{BIT, ZPG_0}, {AND, ZPG_0}, {ROL, ZPG_0}, {RMB2, ZPG_0}, {PLP, IMP_0}, {AND, IMM_0,  }, {ROL, ACC_0}, {NOP, IMM_0}, {BIT, ABS_0  }, {AND, ABS_0,  }, {ROL, ABS_0,  }, {BBR2, REL_0},
    {BMI, REL_0}, {AND, IND_Y   }, {AND, ZPG_I}, {NOP, IND_Y},	{BIT, ZPG_X}, {AND, ZPG_X}, {ROL, ZPG_X}, {RMB3, ZPG_0}, {SEC, IMP_0}, {AND, ABS_Y,  }, {DEC, ACC_0}, {NOP, ABS_Y}, {BIT, ABS_X  }, {AND, ABS_X,  }, {ROL, ABS_X,  }, {BBR3, REL_0},
    {RTI, IMP_0}, {EOR, X_IND   }, {NOP, IMP_0}, {NOP, X_IND},	{NOP, ZPG_0}, {EOR, ZPG_0}, {LSR, ZPG_0}, {RMB4, ZPG_0}, {PHA, IMP_0}, {EOR, IMM_0,  }, {LSR, ACC_0}, {NOP, IMM_0}, {JMP, ABS_0  }, {EOR, ABS_0,  }, {LSR, ABS_0,  }, {BBR4, REL_0},
    {BVC, REL_0}, {EOR, IND_Y   }, {EOR, ZPG_I}, {NOP, IND_Y},	{NOP, ZPG_X}, {EOR, ZPG_X}, {LSR, ZPG_X}, {RMB5, ZPG_0}, {CLI, IMP_0}, {EOR, ABS_Y,  }, {PHY, IMP_0}, {NOP, ABS_Y}, {NOP, ABS_X  }, {EOR, ABS_X,  }, {LSR, ABS_X,  }, {BBR5, REL_0},
    {RTS, IMP_0}, {ADC, X_IND   }, {NOP, IMP_0}, {NOP, X_IND},	{STZ, ZPG_0}, {ADC, ZPG_0}, {ROR, ZPG_0}, {RMB6, ZPG_0}, {PLA, IMP_0}, {ADC, IMM_0, 1}, {ROR, ACC_0}, {NOP, IMM_0}, {JMP, IND_0  }, {ADC, ABS_0,  }, {ROR, ABS_0,  }, {BBR6, REL_0},
    {BVS, REL_0}, {ADC, IND_Y   }, {ADC, ZPG_I}, {NOP, IND_Y},	{STZ, ZPG_X}, {ADC, ZPG_X}, {ROR, ZPG_X}, {RMB7, ZPG_0}, {SEI, IMP_0}, {ADC, ABS_Y,  }, {PLY, IMP_0}, {NOP, ABS_Y}, {JMP, ABS_X_I}, {ADC, ABS_X,  }, {ROR, ABS_X,  }, {BBR7, REL_0},
    {BRA, REL_0}, {STA, X_IND   }, {NOP, IMM_0}, {NOP, X_IND},	{STY, ZPG_0}, {STA, ZPG_0}, {STX, ZPG_0}, {SMB0, ZPG_0}, {DEY, IMP_0}, {BIT, IMM_0,  }, {TXA, IMP_0}, {NOP, IMM_0}, {STY, ABS_0  }, {STA, ABS_0,  }, {STX, ABS_0,  }, {BBS0, REL_0},
    {BCC, REL_0}, {STA, IND_Y, 1}, {STA, ZPG_I}, {NOP, IND_Y},	{STY, ZPG_X}, {STA, ZPG_X}, {STX, ZPG_Y}, {SMB1, ZPG_0}, {TYA, IMP_0}, {STA, ABS_Y, 1}, {TXS, IMP_0}, {NOP, ABS_Y}, {STZ, ABS_0  }, {STA, ABS_X, 1}, {STZ, ABS_X, 1}, {BBS1, REL_0},
    {LDY, IMM_0}, {LDA, X_IND   }, {LDX, IMM_0}, {NOP, X_IND},	{LDY, ZPG_0}, {LDA, ZPG_0}, {LDX, ZPG_0}, {SMB2, ZPG_0}, {TAY, IMP_0}, {LDA, IMM_0,  }, {TAX, IMP_0}, {NOP, IMM_0}, {LDY, ABS_0  }, {LDA, ABS_0,  }, {LDX, ABS_0,  }, {BBS2, REL_0},
    {BCS, REL_0}, {LDA, IND_Y,  }, {LDA, ZPG_I}, {NOP, IND_Y},	{LDY, ZPG_X}, {LDA, ZPG_X}, {LDX, ZPG_Y}, {SMB3, ZPG_0}, {CLV, IMP_0}, {LDA, ABS_Y,  }, {TSX, IMP_0}, {NOP, ABS_Y}, {LDY, ABS_X  }, {LDA, ABS_X,  }, {LDX, ABS_Y,  }, {BBS3, REL_0},
    {CPY, IMM_0}, {CMP, X_IND   }, {NOP, IMM_0}, {NOP, X_IND},	{CPY, ZPG_0}, {CMP, ZPG_0}, {DEC, ZPG_0}, {SMB4, ZPG_0}, {INY, IMP_0}, {CMP, IMM_0,  }, {DEX, IMP_0}, {WAI, IMP_0}, {CPY, ABS_0  }, {CMP, ABS_0,  }, {DEC, ABS_0,  }, {BBS4, REL_0},
    {BNE, REL_0}, {CMP, IND_Y   }, {CMP, ZPG_I}, {NOP, IND_Y},	{NOP, ZPG_X}, {CMP, ZPG_X}, {DEC, ZPG_X}, {SMB5, ZPG_0}, {CLD, IMP_0}, {CMP, ABS_Y,  }, {PHX, IMP_0}, {STP, IMP_0}, {NOP, ABS_X  }, {CMP, ABS_X,  }, {DEC, ABS_X, 1}, {BBS5, REL_0},
    {CPX, IMM_0}, {SBC, X_IND   }, {NOP, IMM_0}, {NOP, X_IND},	{CPX, ZPG_0}, {SBC, ZPG_0}, {INC, ZPG_0}, {SMB6, ZPG_0}, {INX, IMP_0}, {SBC, IMM_0,  }, {NOP, IMP_0}, {NOP, IMM_0}, {CPX, ABS_0  }, {SBC, ABS_0,  }, {INC, ABS_0,  }, {BBS6, REL_0},
    {BEQ, REL_0}, {SBC, IND_Y   }, {SBC, ZPG_I}, {NOP, IND_Y},	{NOP, ZPG_X}, {SBC, ZPG_X}, {INC, ZPG_X}, {SMB7, ZPG_0}, {SED, IMP_0}, {SBC, ABS_Y,  }, {PLX, IMP_0}, {NOP, ABS_Y}, {NOP, ABS_X  }, {SBC, ABS_X,  }, {INC, ABS_X, 1}, {BBS7, REL_0}
};

void w65c02_print(w65c02_t* w){
    printf("pc: %04X s: %02X p: %02X a: %02X x: %02X y: %02X\n", w->pc, w->s, w->p, w->a, w->x, w->y);
    printf("cycles: %d\n", w->cycles);
    printf(
        "N: %d V: %d B: %d D: %d I: %d Z: %d C: %d\n\n",
        (bool)(w->p & SET_N), (bool)(w->p & SET_V), (bool)(w->p & SET_B), (bool)(w->p & SET_D),
        (bool)(w->p & SET_I), (bool)(w->p & SET_Z), (bool)(w->p & SET_C)
    );
}

void w65c02_init(w65c02_t* w){
    w->p = SET_D | SET_U | SET_I;
    w->s = 0;
    w->a = 0;
    w->x = 0;
    w->y = 0;
    w->pc = 0;

    w->cycles = 0;
}

void w65c02_reset(w65c02_t* w){
    w->a = 0;
    w->x = 0;
    w->y = 0;
    w->s -= 3;
    u8 pc_lo = w->read(w, 0xFFFC);
    u8 pc_hi = w->read(w, 0xFFFD);
    w->pc = pc_lo | (pc_hi << 8);
    w->p = SET_I;
}

void w65c02_nmi(w65c02_t* w){
    fetch;
    fetch;
    w->pc -= 2;
    push(w->pc >> 8);
    push(w->pc & 0xFF);
    push(w->p & CLEAR_B);
    u8 lsb = read_byte(0xFFFA);
    u8 msb = read_byte(0xFFFB);
    w->pc = lsb | (msb << 8);
    w->p |= SET_I;
}

void w65c02_irq(w65c02_t* w){
    fetch;
    fetch;
    w->pc -= 2;
    push(w->pc >> 8);
    push(w->pc & 0xFF);
    push(w->p & CLEAR_B);
    u8 lsb = read_byte(0xFFFE);
    u8 msb = read_byte(0xFFFF);
    w->pc = lsb | (msb << 8);
    w->p |= SET_I;
}

bool w65c02_interrupt_enabled(w65c02_t* w){
    return !(w->p & SET_I);
}

void w65c02_step(w65c02_t* w){
    u8 opcode = fetch;

    const opcode_t op_info = opcode_table[opcode];
    const opcodePtr op_impl = op_info.func;
    const OPERAND type = op_info.operand;
    w->slow_op = op_info.slow;

    switch (type){
        case ACC_0:
        {
            w->in_mem = false;
            w->op_arg = w->a;
            w->mem_addr = w->pc;
        }
        break;

        case ABS_0:
        {
            w->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = fetch;
            w->mem_addr = addr_lo | (addr_hi << 8);
        }
        break;

        case ABS_X:
        {
            w->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = read_byte(w->pc);
            w->mem_addr = addr_lo | (addr_hi << 8);
            if(w->slow_op || (u8)w->mem_addr + w->x > 0xFF){
                dummy_read(w->pc);
            }
            w->pc += 1;
            w->mem_addr += w->x;
        }
        break;

        case ABS_Y:
        {
            w->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = read_byte(w->pc);
            w->mem_addr = addr_lo | (addr_hi << 8);
            if(w->slow_op || (u8)w->mem_addr + w->y > 0xFF){
                dummy_read(w->pc);
            }
            w->pc += 1;
            w->mem_addr += w->y;
        }
        break;

        case REL_0:
        {
            w->op_arg = fetch;
        }
        break;

        case IND_Y:
        {
            w->in_mem = true;
            u8 ll = read_byte(w->pc);
            u8 addr_lo = read_byte(ll);
            u8 addr_hi = read_byte((u8)(ll + 1));
            w->mem_addr = addr_lo | (addr_hi << 8);
            if(w->slow_op || (u8)w->mem_addr + w->y > 0xFF){
                dummy_read(w->pc);
            }
            w->pc += 1;
            w->mem_addr += w->y;
        }
        break;

        case IMP_0:
        {
        }
        break;

        case IMM_0:
        {
            w->in_mem = false;
            w->op_arg = fetch;
        }
        break;

        case X_IND:
        {
            w->in_mem = true;
            u8 ll = fetch;
            u8 zpg = ll + w->x;
            dummy_read(ll);
            u8 addr_lo = read_byte(zpg);
            u8 addr_hi = read_byte((u8)(zpg + 1));
            w->mem_addr = addr_lo | (addr_hi << 8); 
        }
        break;

        case ZPG_0:
        {
            w->in_mem = true;
            w->mem_addr = fetch;
        }
        break;

        case ZPG_X:
        {
            w->in_mem = true;
            w->mem_addr = fetch;
            dummy_read(w->mem_addr);
            w->mem_addr = (u8)(w->mem_addr + w->x);
        }
        break;

        case ZPG_Y:
        {
            w->in_mem = true;
            w->mem_addr = fetch;
            dummy_read(w->mem_addr);
            w->mem_addr = (u8)(w->mem_addr + w->y);
        }
        break;

        case IND_0:
        {
            w->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = fetch;
            w->mem_addr = addr_lo | (addr_hi << 8);
            addr_lo = read_byte(w->mem_addr);
            u8 tmp_hi = w->mem_addr >> 8;
            w->mem_addr += 1;
            u8 tmp_lo = w->mem_addr;
            dummy_read((tmp_hi << 8) | tmp_lo);
            addr_hi = read_byte(w->mem_addr);
            w->mem_addr = addr_lo | (addr_hi << 8);
        }
        break;

        case ZPG_I:
        {
            w->in_mem = true;
            u8 zpg = fetch;
            u8 addr_lo = read_byte(zpg++);
            u8 addr_hi = read_byte(zpg);
            w->mem_addr = addr_lo | (addr_hi << 8);
        }
        break;

        case ABS_X_I:
        {
            w->in_mem = true;
            u8 addr_lo = read_byte(w->pc);
            u8 addr_hi = read_byte(w->pc + 1);
            w->mem_addr = addr_lo | (addr_hi << 8);
            dummy_read(w->pc);
            w->pc += 2;
            w->mem_addr += w->x;
            addr_lo = read_byte(w->mem_addr);
            addr_hi = read_byte(w->mem_addr + 1);
            w->mem_addr = addr_lo | (addr_hi << 8);
        }
        break;

        default:
        printf("MODE %d NOT IMPLEMENTED!\n", type);
        return;
    }

    (*op_impl)(w);
}