#include "cpus/h6280.h"

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
    ABS_X_I,
    IMZPG,
    IMABS,
    IMZPX,
    IMABX,
} OPERAND;

typedef void (*opcodePtr)(h6280_t*);

typedef struct opcode_t {
    opcodePtr func;
    char name[8];
    OPERAND operand;
    u8 cycles;
} opcode_t;

#define OP(func, operand, cycles) {func, #func, operand, cycles}


#define SET_N (1 << 7)
#define SET_V (1 << 6)
#define SET_T (1 << 5)
#define SET_B (1 << 4)
#define SET_D (1 << 3)
#define SET_I (1 << 2)
#define SET_Z (1 << 1)
#define SET_C 1

#define CLEAR_N (~SET_N) 
#define CLEAR_V (~SET_V) 
#define CLEAR_T (~SET_T)
#define CLEAR_B (~SET_B) 
#define CLEAR_D (~SET_D) 
#define CLEAR_I (~SET_I) 
#define CLEAR_Z (~SET_Z) 
#define CLEAR_C (~SET_C)

static u32 inline get_addr(h6280_t* h, u16 vaddr){
    u8 seg = vaddr >> 13;
    u16 page = vaddr & 0x1FFF;

    return (h->mpr[seg] << 13) | page;
}

static u8 inline h6280_read_byte(h6280_t* h, u16 vaddr){
    u32 addr = get_addr(h, vaddr);
    u8 out = h->read(h->ctx, addr);
    return out;
}

static void inline h6280_write_byte(h6280_t* h, u16 vaddr, u8 byte){
    u32 addr = get_addr(h, vaddr);
    h->write(h->ctx, addr, byte);
}

#define read_byte(addr) h6280_read_byte(h, addr)
#define write_byte(addr, byte) h6280_write_byte(h, addr, byte)
#define fetch read_byte(h->pc); h->pc += 1
#define calculate_n(x) (x) & 0x80 ? (h->p |= SET_N) : (h->p &= CLEAR_N)
#define calculate_z(x) !((u8)(x)) ? (h->p |= SET_Z) : (h->p &= CLEAR_Z)
#define change_c(x) (x) ? (h->p |= SET_C) : (h->p &= CLEAR_C)
#define change_v(x) (x) ? (h->p |= SET_V) : (h->p &= CLEAR_V)
#define write_back(x) if(h->in_mem) { write_byte(h->mem_addr, (x)); } else h->a = (x)
#define get_arg if(h->in_mem) { h->op_arg = read_byte(h->mem_addr); }
#define push(x) write_byte(h->s | 0x2100, x); h->s -= 1
#define pop read_byte((++h->s) | 0x2100);
#define branch_on(arg, cond) if(cond){ i16 ext_arg = (i16)(i8)arg; u16 new_pc = h->pc + ext_arg; h->pc = new_pc; h->cycles += 2; } 
#define ld(x) if(h->in_mem) { h->op_arg = read_byte(h->mem_addr); } x = h->op_arg; calculate_n(x); calculate_z(x)
#define get_bulk u8 sl = fetch; u8 sh = fetch; u8 dl = fetch; u8 dh = fetch; u8 ll = fetch; u8 lh = fetch; u16 src = (sh << 8) | sl; u16 dst = (dh << 8) | dl; int len = (lh << 8) | ll; if(!len) len = (1 << 16); push(h->y); push(h->a); push(h->x); h->cycles += 17 + 6 * len
#define end_bulk h->x = pop; h->a = pop; h->y = pop
#define GEN_RMB(x) static void RMB ## x (h6280_t* h){ h->op_arg = read_byte(h->mem_addr); write_byte(h->mem_addr, h->op_arg & ~(1 << x)); }
#define GEN_SMB(x) static void SMB ## x (h6280_t* h){ h->op_arg = read_byte(h->mem_addr); write_byte(h->mem_addr, h->op_arg | (1 << x)); }

#define GEN_BBR(x) static void BBR ## x (h6280_t* h){ \
    u8 zp = read_byte(h->op_arg | 0x2000); \
    u8 off = fetch; \
    u16 new_pc = (i16)h->pc + (i16)(i8)off; \
    if(!(zp & (1 << x))) { \
        h->pc = new_pc; \
        h->cycles += 2; \
    } \
} \

#define GEN_BBS(x) static void BBS ## x (h6280_t* h){ \
    u8 zp = read_byte(h->op_arg | 0x2000); \
    u8 off = fetch; \
    u16 new_pc = (i16)h->pc + (i16)(i8)off; \
    if((zp & (1 << x))) { \
        h->pc = new_pc; \
        h->cycles += 2; \
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

static void BRK(h6280_t* h) { 
    fetch;
    push(h->pc >> 8);
    push(h->pc & 0xFF);
    push(h->p | SET_B);
    u8 lsb = read_byte(0xFFF6);
    u8 msb = read_byte(0xFFF7);
    h->pc = lsb | (msb << 8);
    h->p |= SET_I;
    h->p &= CLEAR_D;
    h->p &= CLEAR_T;
}

static void ORA(h6280_t* h) {
    get_arg;
    u8 val;
    if(h->p & SET_T) {
        val = read_byte(0x2000 | h->x);
        h->cycles += 3;
    } else {
        val = h->a;
    }
    val |= h->op_arg;
    calculate_n(val);
    calculate_z(val);
    if(h->p & SET_T)
        write_byte(0x2000 | h->x, val);
    else
        h->a = val;
}

static void NOP(h6280_t* h) {
}

static void ASL(h6280_t* h) { 
    get_arg;
    change_c(h->op_arg & 0x80);
    h->op_arg <<= 1;
    calculate_n(h->op_arg);
    calculate_z(h->op_arg);
    write_back(h->op_arg);
}

static void PHP(h6280_t* h) { 
    push((h->p | SET_B) & CLEAR_T);
}

static void BPL(h6280_t* h) {
    branch_on(h->op_arg, !(h->p & SET_N));
}

static void CLC(h6280_t* h) { 
    change_c(0);
}

static void JSR(h6280_t* h) {
    u8 lo = fetch;
    push(h->pc >> 8);
    push(h->pc & 0xFF);
    u8 hi = read_byte(h->pc);
    h->pc = (hi << 8) | lo;
}

static void AND(h6280_t* h) {
    get_arg;
    u8 val;
    if(h->p & SET_T) {
        val = read_byte(0x2000 | h->x);
        h->cycles += 3;
    } else {
        val = h->a;
    }
    val &= h->op_arg;
    calculate_n(val);
    calculate_z(val);
    if(h->p & SET_T)
        write_byte(0x2000 | h->x, val);
    else
        h->a = val;
}

static void BIT(h6280_t* h) {
    get_arg;
    if(h->in_mem){
        h->p &= CLEAR_N & CLEAR_V;
        h->p |= h->op_arg & 0xC0;
    }
    calculate_n(h->op_arg);
    change_v(h->op_arg & (1 << 6));
    calculate_z(h->op_arg & h->a);
}

static void PLP(h6280_t* h) {
    h->p = pop;
    h->p &= CLEAR_B;
    h->irq_delay = true;
}

static void BMI(h6280_t* h) { 
    branch_on(h->op_arg, h->p & SET_N);
}

static void SEC(h6280_t* h) { 
    change_c(1);
}

static void RTI(h6280_t* h) {
    h->p = read_byte((h->s + 1) | 0x2100);
    u8 pcl = read_byte((h->s + 2) | 0x2100);
    h->s += 3;
    u8 pch = read_byte(h->s | 0x2100);
    h->p &= CLEAR_B;
    h->pc = pcl | (pch << 8); 
}

static void EOR(h6280_t* h) {
    get_arg;
    u8 val;
    if(h->p & SET_T) {
        val = read_byte(0x2000 | h->x);
        h->cycles += 3;
    } else {
        val = h->a;
    }
    val ^= h->op_arg;
    calculate_n(val);
    calculate_z(val);
    if(h->p & SET_T)
        write_byte(0x2000 | h->x, val);
    else
        h->a = val;
}

static void LSR(h6280_t* h) { 
    get_arg;
    change_c(h->op_arg & 1);
    h->op_arg >>= 1;
    h->p &= CLEAR_N;
    calculate_z(h->op_arg);
    write_back(h->op_arg);
}

static void PHA(h6280_t* h) { 
    push(h->a);
}

static void PHX(h6280_t* h) { 
    push(h->x);
}

static void PHY(h6280_t* h) { 
    push(h->y);
}

static void JMP(h6280_t* h) { 
    h->pc = h->mem_addr;
}

static void BVC(h6280_t* h) { 
    branch_on(h->op_arg, !(h->p & SET_V));
}

static void CLI(h6280_t* h) { 
    h->p &= CLEAR_I;
    h->irq_delay = true;
}

static void RTS(h6280_t* h) { 
    u8 pcl = pop;
    u8 pch = pop;
    h->pc = pcl | (pch << 8); 
    h->pc += 1;
}

static void ADC(h6280_t* h) {
    get_arg;
    u8 acc;
    if(h->p & SET_T){
        acc = read_byte(0x2000 | h->x);
        h->cycles += 3;
    } else { 
        acc = h->a;
    }
    bool carry = h->p & SET_C;
    bool new_c;
    bool new_v;
    if(h->p & SET_D){
        u8 a0 = acc & 0xF;
        u8 a1 = acc >> 4;
        u8 b0 = h->op_arg & 0xF;
        u8 b1 = h->op_arg >> 4;
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
        acc = (hi << 4) | (lo & 0x0F);
        new_c = hi > 9;
        /* 4 bit sign exstension */
        if(a1 & 0x08) a1 |= 0xF0;
        if(b1 & 0x08) b1 |= 0xF0;
        i8 ires = (i8)a1 + (i8)b1 + (i8)half_carry;
        new_v = ires < -8 || ires > 7;
    } else {
        u16 ires = (i16)(i8)h->op_arg + (i16)(i8)acc + carry;
        u16 ures = h->op_arg + acc + carry;
        acc = ures;
        new_c = ures > 0xFF;
        new_v = ((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80));
    }
    change_c(new_c);
    change_v(new_v); 
    calculate_n(acc);
    calculate_z(acc);
    if(h->p & SET_T)
        write_byte(0x2000 | h->x, acc);
    else
        h->a = acc;
}

static void PLA(h6280_t* h) { 
    h->a = pop;
    calculate_n(h->a);
    calculate_z(h->a);
}

static void PLX(h6280_t* h) { 
    h->x = pop;
    calculate_n(h->x);
    calculate_z(h->x);
}

static void PLY(h6280_t* h) { 
    h->y = pop;
    calculate_n(h->y);
    calculate_z(h->y);
}


static void BVS(h6280_t* h) { 
    branch_on(h->op_arg, h->p & SET_V);
}

static void BRA(h6280_t* h) { 
    branch_on(h->op_arg, true);
}

static void SEI(h6280_t* h) { 
    h->p |= SET_I;
    h->irq_delay = true;
}

static void STA(h6280_t* h) { 
    write_byte(h->mem_addr, (h->a));
}

static void STZ(h6280_t* h) { 
    write_byte(h->mem_addr, 0);
}

static void DEY(h6280_t* h) {
    h->y -= 1;
    calculate_n(h->y);
    calculate_z(h->y);
}

static void TXA(h6280_t* h) {
    h->a = h->x;
    calculate_n(h->a);
    calculate_z(h->a);
}

static void TYA(h6280_t* h) { 
    h->a = h->y;
    calculate_n(h->a);
    calculate_z(h->a);
}

static void TXS(h6280_t* h) {
    h->s = h->x;
}

static void LDY(h6280_t* h) { ld(h->y); }
static void LDA(h6280_t* h) { ld(h->a); }
static void LDX(h6280_t* h) { ld(h->x); }

static void TAY(h6280_t* h) {
    h->y = h->a;
    calculate_n(h->a);
    calculate_z(h->a);
}

static void TAX(h6280_t* h) {
    h->x = h->a;
    calculate_n(h->a);
    calculate_z(h->a);
}

static void CLV(h6280_t* h) {
    h->p &= CLEAR_V;
}

static void TSX(h6280_t* h) {
    h->x = h->s;
    calculate_n(h->s);
    calculate_z(h->s);
}

static void CPY(h6280_t* h) {
    get_arg;
    h->op_arg = ~h->op_arg;
    u16 ures = h->y + h->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void CMP(h6280_t* h) { 
    get_arg;
    h->op_arg = ~h->op_arg;
    u16 ures = h->a + h->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void DEC(h6280_t* h) {
    get_arg;
    h->op_arg -= 1;
    calculate_n(h->op_arg);
    calculate_z(h->op_arg);
    write_back(h->op_arg);
}

static void INY(h6280_t* h) { 
    h->y += 1;
    calculate_n(h->y);
    calculate_z(h->y);
}

static void DEX(h6280_t* h) { 
    h->x -= 1;
    calculate_n(h->x);
    calculate_z(h->x);
}

static void BNE(h6280_t* h) {
    branch_on(h->op_arg, !(h->p & SET_Z));
}

static void CLD(h6280_t* h) { 
    h->p &= CLEAR_D;
}

static void CPX(h6280_t* h) {
    get_arg;
    h->op_arg = ~h->op_arg;
    u16 ures = h->x + h->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void SBC(h6280_t* h) { 
    get_arg;
    bool carry = h->p & SET_C;
    bool new_c;
    bool new_v;
    if(h->p & SET_D){
        u8 a0 = h->a & 0xF;
        u8 b0 = h->op_arg & 0xF;
        u16 tmp = a0 - b0 - !carry;
        u16 res = h->a - h->op_arg - !carry;
        u16 bin_res = h->a + ~h->op_arg + carry;
        if(res & 0x8000)
            res -= 0x60;
        if(tmp & 0x8000)
            res -= 0x06;
        new_v = (h->a ^ bin_res) & (~h->op_arg ^bin_res) & 0x80;
        new_c = (u16)res <= (u16)h->a || (res & 0xff0) == 0xff0;
        h->a = res;
    } else {
        h->op_arg = ~h->op_arg;
        u16 ires = (i16)(i8)h->op_arg + (i16)(i8)h->a + carry;
        u16 ures = h->op_arg + h->a + carry;
        h->a = ures;
        new_c = ures > 0xFF;
        new_v = ((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80));
    }
    change_c(new_c);
    change_v(new_v); 
    calculate_n(h->a);
    calculate_z(h->a);
}

static void INC(h6280_t* h) { 
    get_arg;
    h->op_arg += 1;
    calculate_n(h->op_arg);
    calculate_z(h->op_arg);
    write_back(h->op_arg);
}

static void INX(h6280_t* h) { 
    h->x += 1;
    calculate_n(h->x);
    calculate_z(h->x);
}

static void BEQ(h6280_t* h) {
    branch_on(h->op_arg, h->p & SET_Z);
}

static void SED(h6280_t* h) { 
    h->p |= SET_D;
}

static void ROL(h6280_t* h) {
    get_arg;
    bool c = h->op_arg & 0x80;
    h->op_arg = (h->op_arg << 1) | ((bool)(h->p & SET_C));
    change_c(c);
    calculate_n(h->op_arg);
    calculate_z(h->op_arg);
    write_back(h->op_arg);
}

static void ROR(h6280_t* h) {
    get_arg;
    bool c = h->op_arg & 1;
    h->op_arg = (h->op_arg >> 1) | (((bool)(h->p & SET_C)) << 7);
    change_c(c);
    calculate_n(h->op_arg);
    calculate_z(h->op_arg);
    write_back(h->op_arg);
}

static void STY(h6280_t* h) { 
    write_byte(h->mem_addr, (h->y));
}

static void STX(h6280_t* h) { 
    write_byte(h->mem_addr, (h->x));
}

static void BCS(h6280_t* h) {
    branch_on(h->op_arg, h->p & SET_C);
}

static void BCC(h6280_t* h) {
    branch_on(h->op_arg, !(h->p & SET_C));
}

static void TSB(h6280_t* h){
    get_arg;
    calculate_z(h->op_arg & h->a);
    calculate_n(h->op_arg);
    change_v(h->op_arg & (1 << 6));
    write_byte(h->mem_addr, h->op_arg | h->a);
}

static void TRB(h6280_t* h){
    get_arg;
    calculate_z(h->op_arg & h->a);
    calculate_n(h->op_arg);
    change_v(h->op_arg & (1 << 6));
    write_byte(h->mem_addr, h->op_arg & ~h->a);
}

static void WAI(h6280_t* h){
    printf("wai\n");
    h->wait = true;
}

static void STP(h6280_t* h){
    printf("stp\n");
    h->stop = true;
}

static void CSH(h6280_t* h){
    h->hi_speed = true;
}

static void CSL(h6280_t* h){
    h->hi_speed = false;
}

static void TAM(h6280_t* h){   
    for(int i = 0; i < 8; i++){
        if(h->op_arg & (1 << i))
            h->mpr[i] = h->a;
    }
}

static void TMA(h6280_t* h){
    h->a = 0;
    for(int i = 0; i < 8; i++){
        if(h->op_arg & (1 << i))
            h->a |= h->mpr[i];
    }
}

static void TII(h6280_t* h){
    get_bulk;
    for(int i = 0; i < len; i++){
        u8 tmp = read_byte(src++);
        write_byte(dst++, tmp);
    }
    end_bulk;
}

static void TDD(h6280_t* h){
    get_bulk;
    for(int i = 0; i < len; i++){
        u8 tmp = read_byte(src--);
        write_byte(dst--, tmp);
    }
    end_bulk;
}

static void TIN(h6280_t* h){
    get_bulk;
    for(int i = 0; i < len; i++){
        u8 tmp = read_byte(src++);
        write_byte(dst, tmp);
    }
    end_bulk;
}

static void TIA(h6280_t* h){
    get_bulk;
    for(int i = 0; i < len; i++){
        u8 tmp = read_byte(src++);
        write_byte(dst, tmp);
        if(i & 1)
            dst -= 1;
        else
            dst += 1;
    }
    end_bulk;
}

static void TAI(h6280_t* h){
    get_bulk;
    for(int i = 0; i < len; i++){
        u8 tmp = read_byte(src);
        write_byte(dst++, tmp);
        if(i & 1)
            src -= 1;
        else
            src += 1;
    }
    end_bulk;
}

static void CLA(h6280_t* h){
    h->a = 0;   
}

static void CLX(h6280_t* h){
    h->x = 0;
}

static void CLY(h6280_t* h){
    h->y = 0;
}

static void ST0(h6280_t* h){
    h->io_write(h->ctx, 0, h->op_arg);
}

static void ST1(h6280_t* h){
    h->io_write(h->ctx, 2, h->op_arg);
}

static void ST2(h6280_t* h){
    h->io_write(h->ctx, 3, h->op_arg);
}

static void SXY(h6280_t* h){
    u8 tmp = h->y;
    h->y = h->x;
    h->x = tmp;
}


static void SAX(h6280_t* h){
    u8 tmp = h->a;
    h->a = h->x;
    h->x = tmp;
}


static void SAY(h6280_t* h){
    u8 tmp = h->a;
    h->a = h->y;
    h->y = tmp;
}

static void BSR(h6280_t* h){
    push(h->pc >> 8);
    push(h->pc & 0xFF);
    u8 off = fetch;
    h->pc += (i16)(i8)off;
}

static void SET(h6280_t* h){
    h->p |= SET_T;
}

static void TST(h6280_t* h){
    u8 im = read_byte(h->mem_addr);
    u8 tmp = h->op_arg & im;
    calculate_z(tmp);
    calculate_n(im);
    change_v(im & (1 << 6));
}

static opcode_t opcode_table[256] = {
    OP(BRK, IMP_0, 8), OP(ORA, X_IND, 7), OP(SXY, IMP_0, 3), OP(ST0, IMM_0, 5),	OP(TSB, ZPG_0, 6), OP(ORA, ZPG_0, 4), OP(ASL, ZPG_0, 6), OP(RMB0, ZPG_0, 7), OP(PHP, IMP_0, 3), OP(ORA, IMM_0, 2), OP(ASL, ACC_0, 2), OP(NOP, IMM_0, 2), OP(TSB, ABS_0,   7), OP(ORA, ABS_0, 5), OP(ASL, ABS_0, 7), OP(BBR0, REL_0, 6),
    OP(BPL, REL_0, 2), OP(ORA, IND_Y, 7), OP(ORA, ZPG_I, 7), OP(ST1, IMM_0, 5),	OP(TRB, ZPG_0, 6), OP(ORA, ZPG_X, 4), OP(ASL, ZPG_X, 6), OP(RMB1, ZPG_0, 7), OP(CLC, IMP_0, 2), OP(ORA, ABS_Y, 5), OP(INC, ACC_0, 2), OP(NOP, ABS_Y, 2), OP(TRB, ABS_0,   7), OP(ORA, ABS_X, 5), OP(ASL, ABS_X, 7), OP(BBR1, REL_0, 6),
    OP(JSR, IMP_0, 7), OP(AND, X_IND, 7), OP(SAX, IMP_0, 3), OP(ST2, IMM_0, 5),	OP(BIT, ZPG_0, 4), OP(AND, ZPG_0, 4), OP(ROL, ZPG_0, 6), OP(RMB2, ZPG_0, 7), OP(PLP, IMP_0, 4), OP(AND, IMM_0, 2), OP(ROL, ACC_0, 2), OP(NOP, IMM_0, 2), OP(BIT, ABS_0,   5), OP(AND, ABS_0, 5), OP(ROL, ABS_0, 7), OP(BBR2, REL_0, 6),
    OP(BMI, REL_0, 2), OP(AND, IND_Y, 7), OP(AND, ZPG_I, 7), OP(NOP, IND_Y, 2),	OP(BIT, ZPG_X, 4), OP(AND, ZPG_X, 4), OP(ROL, ZPG_X, 6), OP(RMB3, ZPG_0, 7), OP(SEC, IMP_0, 2), OP(AND, ABS_Y, 5), OP(DEC, ACC_0, 2), OP(NOP, ABS_Y, 2), OP(BIT, ABS_X,   5), OP(AND, ABS_X, 5), OP(ROL, ABS_X, 7), OP(BBR3, REL_0, 6),
    OP(RTI, IMP_0, 7), OP(EOR, X_IND, 7), OP(SAY, IMP_0, 3), OP(TMA, IMM_0, 4),	OP(BSR, IMP_0, 8), OP(EOR, ZPG_0, 4), OP(LSR, ZPG_0, 6), OP(RMB4, ZPG_0, 7), OP(PHA, IMP_0, 3), OP(EOR, IMM_0, 2), OP(LSR, ACC_0, 2), OP(NOP, IMM_0, 2), OP(JMP, ABS_0,   4), OP(EOR, ABS_0, 5), OP(LSR, ABS_0, 7), OP(BBR4, REL_0, 6),
    OP(BVC, REL_0, 2), OP(EOR, IND_Y, 7), OP(EOR, ZPG_I, 7), OP(TAM, IMM_0, 5),	OP(CSL, IMP_0, 3), OP(EOR, ZPG_X, 4), OP(LSR, ZPG_X, 6), OP(RMB5, ZPG_0, 7), OP(CLI, IMP_0, 2), OP(EOR, ABS_Y, 5), OP(PHY, IMP_0, 3), OP(NOP, ABS_Y, 2), OP(NOP, ABS_X,   2), OP(EOR, ABS_X, 5), OP(LSR, ABS_X, 7), OP(BBR5, REL_0, 6),
    OP(RTS, IMP_0, 7), OP(ADC, X_IND, 7), OP(CLA, IMP_0, 2), OP(NOP, X_IND, 2),	OP(STZ, ZPG_0, 4), OP(ADC, ZPG_0, 4), OP(ROR, ZPG_0, 6), OP(RMB6, ZPG_0, 7), OP(PLA, IMP_0, 4), OP(ADC, IMM_0, 2), OP(ROR, ACC_0, 2), OP(NOP, IMM_0, 2), OP(JMP, IND_0,   7), OP(ADC, ABS_0, 5), OP(ROR, ABS_0, 7), OP(BBR6, REL_0, 6),
    OP(BVS, REL_0, 2), OP(ADC, IND_Y, 7), OP(ADC, ZPG_I, 7), OP(TII, IMP_0, 0),	OP(STZ, ZPG_X, 4), OP(ADC, ZPG_X, 4), OP(ROR, ZPG_X, 6), OP(RMB7, ZPG_0, 7), OP(SEI, IMP_0, 2), OP(ADC, ABS_Y, 5), OP(PLY, IMP_0, 4), OP(NOP, ABS_Y, 2), OP(JMP, ABS_X_I, 7), OP(ADC, ABS_X, 5), OP(ROR, ABS_X, 7), OP(BBR7, REL_0, 6),
    OP(BRA, REL_0, 2), OP(STA, X_IND, 7), OP(CLX, IMP_0, 2), OP(TST, IMZPG, 7),	OP(STY, ZPG_0, 4), OP(STA, ZPG_0, 4), OP(STX, ZPG_0, 4), OP(SMB0, ZPG_0, 7), OP(DEY, IMP_0, 2), OP(BIT, IMM_0, 2), OP(TXA, IMP_0, 2), OP(NOP, IMM_0, 2), OP(STY, ABS_0,   5), OP(STA, ABS_0, 5), OP(STX, ABS_0, 5), OP(BBS0, REL_0, 6),
    OP(BCC, REL_0, 2), OP(STA, IND_Y, 7), OP(STA, ZPG_I, 7), OP(TST, IMABS, 8),	OP(STY, ZPG_X, 4), OP(STA, ZPG_X, 4), OP(STX, ZPG_Y, 4), OP(SMB1, ZPG_0, 7), OP(TYA, IMP_0, 2), OP(STA, ABS_Y, 5), OP(TXS, IMP_0, 2), OP(NOP, ABS_Y, 2), OP(STZ, ABS_0,   5), OP(STA, ABS_X, 5), OP(STZ, ABS_X, 5), OP(BBS1, REL_0, 6),
    OP(LDY, IMM_0, 2), OP(LDA, X_IND, 7), OP(LDX, IMM_0, 2), OP(TST, IMZPX, 7),	OP(LDY, ZPG_0, 4), OP(LDA, ZPG_0, 4), OP(LDX, ZPG_0, 4), OP(SMB2, ZPG_0, 7), OP(TAY, IMP_0, 2), OP(LDA, IMM_0, 2), OP(TAX, IMP_0, 2), OP(NOP, IMM_0, 2), OP(LDY, ABS_0,   5), OP(LDA, ABS_0, 5), OP(LDX, ABS_0, 5), OP(BBS2, REL_0, 6),
    OP(BCS, REL_0, 2), OP(LDA, IND_Y, 7), OP(LDA, ZPG_I, 7), OP(TST, IMABX, 8),	OP(LDY, ZPG_X, 4), OP(LDA, ZPG_X, 4), OP(LDX, ZPG_Y, 4), OP(SMB3, ZPG_0, 7), OP(CLV, IMP_0, 2), OP(LDA, ABS_Y, 5), OP(TSX, IMP_0, 2), OP(NOP, ABS_Y, 2), OP(LDY, ABS_X,   5), OP(LDA, ABS_X, 5), OP(LDX, ABS_Y, 5), OP(BBS3, REL_0, 6),
    OP(CPY, IMM_0, 2), OP(CMP, X_IND, 7), OP(CLY, IMP_0, 2), OP(TDD, IMP_0, 0),	OP(CPY, ZPG_0, 4), OP(CMP, ZPG_0, 4), OP(DEC, ZPG_0, 6), OP(SMB4, ZPG_0, 7), OP(INY, IMP_0, 2), OP(CMP, IMM_0, 2), OP(DEX, IMP_0, 2), OP(WAI, IMP_0, 2), OP(CPY, ABS_0,   5), OP(CMP, ABS_0, 5), OP(DEC, ABS_0, 7), OP(BBS4, REL_0, 6),
    OP(BNE, REL_0, 2), OP(CMP, IND_Y, 7), OP(CMP, ZPG_I, 7), OP(TIN, IMP_0, 0),	OP(CSH, IMP_0, 3), OP(CMP, ZPG_X, 4), OP(DEC, ZPG_X, 6), OP(SMB5, ZPG_0, 7), OP(CLD, IMP_0, 2), OP(CMP, ABS_Y, 5), OP(PHX, IMP_0, 3), OP(STP, IMP_0, 2), OP(NOP, ABS_X,   2), OP(CMP, ABS_X, 5), OP(DEC, ABS_X, 7), OP(BBS5, REL_0, 6),
    OP(CPX, IMM_0, 2), OP(SBC, X_IND, 7), OP(NOP, IMM_0, 2), OP(TIA, IMP_0, 0),	OP(CPX, ZPG_0, 4), OP(SBC, ZPG_0, 4), OP(INC, ZPG_0, 6), OP(SMB6, ZPG_0, 7), OP(INX, IMP_0, 2), OP(SBC, IMM_0, 2), OP(NOP, IMP_0, 2), OP(NOP, IMM_0, 2), OP(CPX, ABS_0,   5), OP(SBC, ABS_0, 5), OP(INC, ABS_0, 7), OP(BBS6, REL_0, 6),
    OP(BEQ, REL_0, 2), OP(SBC, IND_Y, 7), OP(SBC, ZPG_I, 7), OP(TAI, IMP_0, 0),	OP(SET, IMP_0, 2), OP(SBC, ZPG_X, 4), OP(INC, ZPG_X, 6), OP(SMB7, ZPG_0, 7), OP(SED, IMP_0, 2), OP(SBC, ABS_Y, 5), OP(PLX, IMP_0, 4), OP(NOP, ABS_Y, 2), OP(NOP, ABS_X,   2), OP(SBC, ABS_X, 5), OP(INC, ABS_X, 7), OP(BBS7, REL_0, 6)
};

static char addressing_modes[][8] = {
    "ACC_0",
    "ABS_0",
    "ABS_X",
    "ABS_Y",
    "IMM_0",
    "IMP_0",
    "IND_0",
    "X_IND",
    "IND_Y",
    "REL_0",
    "ZPG_0",
    "ZPG_X",
    "ZPG_Y",
    "ZPG_I",
    "ABS_X",
    "IMZPG",
    "IMABS",
    "IMZPX",
    "IMABX"
};

void h6280_print(h6280_t* h){
    printf("pc: %04X s: %02X p: %02X a: %02X x: %02X y: %02X\n", h->pc, h->s, h->p, h->a, h->x, h->y);
    printf("cycles: %d\n", h->cycles);
    printf(
        "N: %d V: %d T: %d B: %d D: %d I: %d Z: %d C: %d\n",
        (bool)(h->p & SET_N), (bool)(h->p & SET_V), (bool)(h->p & SET_T), (bool)(h->p & SET_B), (bool)(h->p & SET_D),
        (bool)(h->p & SET_I), (bool)(h->p & SET_Z), (bool)(h->p & SET_C)
    );

    for(int i = 0; i < 8; i++){
        printf("MPR %02X: %02X\n", i, h->mpr[i]);
    }
    printf("\n");
}

void h6280_print_next_op(h6280_t* h){
    u8 opcode = read_byte(h->pc);
    const opcode_t* op_info = &opcode_table[opcode];
    printf("%02X %s %s\n", h->pc, op_info->name, addressing_modes[op_info->operand]);
}

void h6280_init(h6280_t* h){
    h->p = SET_I;
    h->s = 0;
    h->a = 0;
    h->x = 0;
    h->y = 0;
    h->pc = 0;

    h->cycles = 0;

    h->mpr[0] = 0xFF;
    h->mpr[1] = 0xF8;
    h->mpr[7] = 0x00;
}

void h6280_reset(h6280_t* h){
    h->a = 0;
    h->x = 0;
    h->y = 0;
    h->s -= 3;
    h->mpr[7] = 0x00;
    u8 pc_lo = h->read(h, 0x1FFE);
    u8 pc_hi = h->read(h, 0x1FFF);
    h->pc = pc_lo | (pc_hi << 8);
    h->p = SET_I;
}

void h6280_nmi(h6280_t* h){
    fetch;
    fetch;
    h->pc -= 2;
    push(h->pc >> 8);
    push(h->pc & 0xFF);
    push(h->p & CLEAR_B);
    u8 lsb = read_byte(0xFFFC);
    u8 msb = read_byte(0xFFFD);
    h->pc = lsb | (msb << 8);
    h->p |= SET_I;
    h->p &= CLEAR_D;
    h->p &= CLEAR_T;
    h->cycles += 7;
}

void h6280_irq(h6280_t* h, u16 vector){
    fetch;
    fetch;
    h->pc -= 2;
    push(h->pc >> 8);
    push(h->pc & 0xFF);
    push(h->p & CLEAR_B);
    u8 lsb = read_byte(vector);
    u8 msb = read_byte(vector + 1);
    h->pc = lsb | (msb << 8);
    h->p |= SET_I;
    h->p &= CLEAR_D;
    h->p &= CLEAR_T;
    h->cycles += 7;
}

bool h6280_interrupt_enabled(h6280_t* h){
    return !(h->p & SET_I) && !h->irq_delay;
}

void h6280_step(h6280_t* h){
    u8 opcode = fetch;

    const opcode_t* op_info = &opcode_table[opcode];
    const opcodePtr op_impl = op_info->func;
    OPERAND type = op_info->operand;
    u8 cycles = op_info->cycles;

    decode:
    switch (type){
        case ACC_0:
        {
            h->in_mem = false;
            h->op_arg = h->a;
            h->mem_addr = h->pc;
        }
        break;

        case ABS_0:
        {
            h->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = fetch;
            h->mem_addr = addr_lo | (addr_hi << 8);
        }
        break;

        case ABS_X:
        {
            h->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = read_byte(h->pc);
            h->mem_addr = addr_lo | (addr_hi << 8);
            h->pc += 1;
            h->mem_addr += h->x;
        }
        break;

        case ABS_Y:
        {
            h->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = read_byte(h->pc);
            h->mem_addr = addr_lo | (addr_hi << 8);
            h->pc += 1;
            h->mem_addr += h->y;
        }
        break;

        case REL_0:
        {
            h->op_arg = fetch;
        }
        break;

        case IND_Y:
        {
            h->in_mem = true;
            u8 ll = read_byte(h->pc);
            u8 addr_lo = read_byte(ll | 0x2000);
            u8 addr_hi = read_byte((u8)(ll + 1) | 0x2000);
            h->mem_addr = addr_lo | (addr_hi << 8);
            h->pc += 1;
            h->mem_addr += h->y;
        }
        break;

        case IMP_0:
        {
        }
        break;

        case IMM_0:
        {
            h->in_mem = false;
            h->op_arg = fetch;
        }
        break;

        case X_IND:
        {
            h->in_mem = true;
            u8 ll = fetch;
            u8 zpg = ll + h->x;
            u8 addr_lo = read_byte(zpg | 0x2000);
            u8 addr_hi = read_byte((u8)(zpg + 1) | 0x2000);
            h->mem_addr = addr_lo | (addr_hi << 8); 
        }
        break;

        case ZPG_0:
        {
            h->in_mem = true;
            h->mem_addr = fetch;
            h->mem_addr |= 0x2000;
        }
        break;

        case ZPG_X:
        {
            h->in_mem = true;
            h->mem_addr = fetch;
            h->mem_addr = (u8)(h->mem_addr + h->x) | 0x2000;
        }
        break;

        case ZPG_Y:
        {
            h->in_mem = true;
            h->mem_addr = fetch;
            h->mem_addr = (u8)(h->mem_addr + h->y) | 0x2000;
        }
        break;

        case IND_0:
        {
            h->in_mem = true;
            u8 addr_lo = fetch;
            u8 addr_hi = fetch;
            h->mem_addr = addr_lo | (addr_hi << 8);
            addr_lo = read_byte(h->mem_addr);
            h->mem_addr += 1;
            addr_hi = read_byte(h->mem_addr);
            h->mem_addr = addr_lo | (addr_hi << 8);
        }
        break;

        case ZPG_I:
        {
            h->in_mem = true;
            u8 zpg = fetch;
            u8 addr_lo = read_byte(zpg++ | 0x2000);
            u8 addr_hi = read_byte(zpg | 0x2000);
            h->mem_addr = addr_lo | (addr_hi << 8);
        }
        break;

        case ABS_X_I:
        {
            h->in_mem = true;
            u8 addr_lo = read_byte(h->pc);
            u8 addr_hi = read_byte(h->pc + 1);
            h->mem_addr = addr_lo | (addr_hi << 8);
            h->pc += 2;
            h->mem_addr += h->x;
            addr_lo = read_byte(h->mem_addr);
            addr_hi = read_byte(h->mem_addr + 1);
            h->mem_addr = addr_lo | (addr_hi << 8);
        }
        break;

        case IMZPG:
        {
            h->op_arg = fetch;
            type = ZPG_0;
            goto decode;
        }
        break;

        case IMABS:
        {
            h->op_arg = fetch;
            type = ABS_0;
            goto decode;
        }
        break;

        case IMZPX:
        {
            h->op_arg = fetch;
            type = ZPG_X;
            goto decode;
        }
        break;

        case IMABX:
        {
            h->op_arg = fetch;
            type = ABS_X;
            goto decode;
        }
        break;

        default:
        printf("MODE %d NOT IMPLEMENTED!\n", type);
        return;
    }

    h->irq_delay = false;
    (*op_impl)(h);
    h->cycles += cycles;

    if(opcode != 0xF4 && opcode != 0x28 && opcode != 0x40)
        h->p &= CLEAR_T;
}