#ifndef CPU_TYPE
#  error "isa_65X02.h: CPU_TYPE must be defined (e.g. m6502_t)"
#endif
#ifndef CPU_VAR
#  error "isa_65X02.h: CPU_VAR must be defined (e.g. m)"
#endif

#define CPU CPU_VAR

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
#ifdef W65C02
    ZPG_I,
    ABS_X_I,
#endif
} OPERAND;

typedef void (*opcodePtr)(CPU_TYPE*);

typedef struct opcode_t {
    opcodePtr func;
    OPERAND   operand;
    bool      slow;
} opcode_t;


#define SET_N   (1 << 7)
#define SET_V   (1 << 6)
#define SET_U   (1 << 5)
#define SET_B   (1 << 4)
#define SET_D   (1 << 3)
#define SET_I   (1 << 2)
#define SET_Z   (1 << 1)
#define SET_C   1

#define CLEAR_N (~SET_N)
#define CLEAR_V (~SET_V)
#define CLEAR_B (~SET_B)
#define CLEAR_D (~SET_D)
#define CLEAR_I (~SET_I)
#define CLEAR_Z (~SET_Z)
#define CLEAR_C (~SET_C)


static u8 inline _isa_read_byte(CPU_TYPE* CPU, u16 addr) {
    u8 out = CPU->read(CPU->ctx, addr);
    CPU->cycles += 1;
    return out;
}

static void inline _isa_dummy_read(CPU_TYPE* CPU, u16 addr) {
#ifndef LOW_ACC_EMULATION
    CPU->read(CPU->ctx, addr);
#else
    (void)addr;
#endif
    CPU->cycles += 1;
}

#define read_byte(addr)       _isa_read_byte(CPU, addr)
#define dummy_read(addr)      _isa_dummy_read(CPU, addr)
#define write_byte(addr, byte) CPU->write(CPU->ctx, addr, byte); CPU->cycles += 1
#define fetch                  read_byte(CPU->pc); CPU->pc += 1

#define calculate_n(x)  ((x) & 0x80 ? (CPU->p |= SET_N) : (CPU->p &= CLEAR_N))
#define calculate_z(x)  (!((u8)(x))  ? (CPU->p |= SET_Z) : (CPU->p &= CLEAR_Z))
#define change_c(x)     ((x) ? (CPU->p |= SET_C) : (CPU->p &= CLEAR_C))
#define change_v(x)     ((x) ? (CPU->p |= SET_V) : (CPU->p &= CLEAR_V))

#define write_back(x)   if (CPU->in_mem) { write_byte(CPU->mem_addr, (x)); } else CPU->a = (x)
#define get_arg         if (CPU->in_mem) { CPU->op_arg = read_byte(CPU->mem_addr); }
#define push(x)         write_byte(CPU->s | 0x100, x); CPU->s -= 1
#define pop             read_byte((++CPU->s) | 0x100)
#define ld(x)           if (CPU->in_mem) { CPU->op_arg = read_byte(CPU->mem_addr); } \
                        (x) = CPU->op_arg; calculate_n(x); calculate_z(x)

#define branch_on(arg, cond) \
    if (cond) { \
        i16 ext_arg = (i16)(i8)(arg); \
        u16 new_pc  = CPU->pc + ext_arg; \
        dummy_read(CPU->pc); \
        if (new_pc >> 8 != CPU->pc >> 8) { \
            dummy_read((CPU->pc & 0xFF00) | (new_pc & 0xFF)); \
        } \
        CPU->pc = new_pc; \
    }


#ifdef W65C02
#define GEN_RMB(x) \
    static void RMB ## x (CPU_TYPE* CPU) { \
        CPU->op_arg = read_byte(CPU->mem_addr); \
        dummy_read(CPU->mem_addr); \
        write_byte(CPU->mem_addr, CPU->op_arg & ~(1 << (x))); \
    }

#define GEN_SMB(x) \
    static void SMB ## x (CPU_TYPE* CPU) { \
        CPU->op_arg = read_byte(CPU->mem_addr); \
        dummy_read(CPU->mem_addr); \
        write_byte(CPU->mem_addr, CPU->op_arg | (1 << (x))); \
    }

#define GEN_BBR(x) \
    static void BBR ## x (CPU_TYPE* CPU) { \
        u8 zp  = read_byte(CPU->op_arg); \
        dummy_read(CPU->op_arg); \
        u8 off = fetch; \
        u16 new_pc = (i16)CPU->pc + (i16)(i8)off; \
        if (!(zp & (1 << (x)))) { \
            if ((CPU->pc >> 8) != (new_pc >> 8)) \
                dummy_read(CPU->pc); \
            dummy_read(CPU->pc); \
            CPU->pc = new_pc; \
        } \
    }

#define GEN_BBS(x) \
    static void BBS ## x (CPU_TYPE* CPU) { \
        u8 zp  = read_byte(CPU->op_arg); \
        dummy_read(CPU->op_arg); \
        u8 off = fetch; \
        u16 new_pc = (i16)CPU->pc + (i16)(i8)off; \
        if (zp & (1 << (x))) { \
            if ((CPU->pc >> 8) != (new_pc >> 8)) \
                dummy_read(CPU->pc); \
            dummy_read(CPU->pc); \
            CPU->pc = new_pc; \
        } \
    }

GEN_RMB(0) GEN_RMB(1) GEN_RMB(2) GEN_RMB(3)
GEN_RMB(4) GEN_RMB(5) GEN_RMB(6) GEN_RMB(7)
GEN_SMB(0) GEN_SMB(1) GEN_SMB(2) GEN_SMB(3)
GEN_SMB(4) GEN_SMB(5) GEN_SMB(6) GEN_SMB(7)
GEN_BBR(0) GEN_BBR(1) GEN_BBR(2) GEN_BBR(3)
GEN_BBR(4) GEN_BBR(5) GEN_BBR(6) GEN_BBR(7)
GEN_BBS(0) GEN_BBS(1) GEN_BBS(2) GEN_BBS(3)
GEN_BBS(4) GEN_BBS(5) GEN_BBS(6) GEN_BBS(7)
#endif

#if defined(R2A03) || defined(M6502)
/* The magic byte is CPU- (and sometimes board-) dependent. */
#define MAGIC_BYTE 0xEE
#endif

static void BRK(CPU_TYPE* CPU) {
    fetch;
    push(CPU->pc >> 8);
    push(CPU->pc & 0xFF);
    push(CPU->p | SET_B);
    u8 lsb = read_byte(0xFFFE);
    u8 msb = read_byte(0xFFFF);
    CPU->pc = lsb | (msb << 8);
    CPU->p |= SET_I;
#ifdef W65C02
    CPU->p &= CLEAR_D;
#endif
}

static void ORA(CPU_TYPE* CPU) {
    get_arg;
    CPU->a |= CPU->op_arg;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void NOP(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
}

static void ASL(CPU_TYPE* CPU) {
    get_arg;
    change_c(CPU->op_arg & 0x80);
    if (CPU->in_mem) {
#ifdef W65C02
        dummy_read(CPU->mem_addr);
#else
        write_byte(CPU->mem_addr, CPU->op_arg);
#endif
    } else {
        dummy_read(CPU->pc);
    }
    CPU->op_arg <<= 1;
    calculate_n(CPU->op_arg);
    calculate_z(CPU->op_arg);
    write_back(CPU->op_arg);
}

static void PHP(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    push(CPU->p | SET_B | SET_U);
}

static void BPL(CPU_TYPE* CPU) {
    branch_on(CPU->op_arg, !(CPU->p & SET_N));
}

static void CLC(CPU_TYPE* CPU) {
    change_c(0);
    dummy_read(CPU->pc);
}

static void JSR(CPU_TYPE* CPU) {
    u8 l = fetch;
    dummy_read(CPU->s | 0x100);
    push(CPU->pc >> 8);
    push(CPU->pc & 0xFF);
    u8 h = read_byte(CPU->pc);
    CPU->pc = (h << 8) | l;
}

static void AND(CPU_TYPE* CPU) {
    get_arg;
    CPU->a &= CPU->op_arg;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void BIT(CPU_TYPE* CPU) {
    get_arg;
#ifdef W65C02
    /* W65C02: N/V from memory only (not immediate/accumulator) */
    if (CPU->in_mem)
#endif
    {
        CPU->p &= CLEAR_N & CLEAR_V;
        CPU->p |= CPU->op_arg & 0xC0;
    }
    calculate_z(CPU->op_arg & CPU->a);
}

static void PLP(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    dummy_read(CPU->s | 0x100);
    CPU->p = pop;
    CPU->p |= SET_U;
    CPU->p &= CLEAR_B;
}

static void BMI(CPU_TYPE* CPU) {
    branch_on(CPU->op_arg, CPU->p & SET_N);
}

static void SEC(CPU_TYPE* CPU) {
    change_c(1);
    dummy_read(CPU->pc);
}

static void RTI(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    dummy_read(CPU->s | 0x100);
    CPU->p   = read_byte((CPU->s + 1) | 0x100);
    u8 pcl    = read_byte((CPU->s + 2) | 0x100);
    CPU->s  += 3;
    u8 pch    = read_byte(CPU->s | 0x100);
    CPU->p  &= CLEAR_B;
    CPU->p  |= SET_U;
    CPU->pc  = pcl | (pch << 8);
}

static void EOR(CPU_TYPE* CPU) {
    get_arg;
    CPU->a ^= CPU->op_arg;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void LSR(CPU_TYPE* CPU) {
    get_arg;
    change_c(CPU->op_arg & 1);
    if (CPU->in_mem) {
#ifdef W65C02
        dummy_read(CPU->mem_addr);
#else
        write_byte(CPU->mem_addr, CPU->op_arg);
#endif
    } else {
        dummy_read(CPU->pc);
    }
    CPU->op_arg >>= 1;
    CPU->p &= CLEAR_N;
    calculate_z(CPU->op_arg);
    write_back(CPU->op_arg);
}

static void PHA(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    push(CPU->a);
}

static void JMP(CPU_TYPE* CPU) {
    CPU->pc = CPU->mem_addr;
}

static void BVC(CPU_TYPE* CPU) {
    branch_on(CPU->op_arg, !(CPU->p & SET_V));
}

static void CLI(CPU_TYPE* CPU) {
    CPU->p &= CLEAR_I;
    dummy_read(CPU->pc);
}

static void RTS(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    dummy_read(CPU->s | 0x100);
    u8 pcl  = pop;
    u8 pch  = pop;
    CPU->pc = pcl | (pch << 8);
    dummy_read(CPU->pc);
    CPU->pc += 1;
}

static void ADC(CPU_TYPE* CPU) {
    get_arg;
    bool carry = CPU->p & SET_C;
    bool new_c, new_v;

#ifndef R2A03
    /* W65C02 AND M6502: full BCD support */
    if (CPU->p & SET_D) {
        #ifdef W65C02
            CPU->slow_op ? dummy_read(0x7F) : dummy_read(CPU->mem_addr);
        #endif
        u8 a0 = CPU->a & 0xF,  a1 = CPU->a >> 4;
        u8 b0 = CPU->op_arg & 0xF, b1 = CPU->op_arg >> 4;
        u8 lo = a0 + b0 + carry;
        u8 hi = a1 + b1;
        bool half_carry = false;
        if (lo > 9) { half_carry = true; hi += 1; lo += 6; }
        #ifdef M6502
        calculate_n(hi << 4);
        calculate_z(CPU->a + CPU->op_arg + carry);
        #endif
        if (hi > 9) { hi += 6; }
        CPU->a = (hi << 4) | (lo & 0x0F);
        new_c   = hi > 9;
        /* 4-bit sign extension for overflow detection */
        if (a1 & 0x08) a1 |= 0xF0;
        if (b1 & 0x08) b1 |= 0xF0;
        i8 ires = (i8)a1 + (i8)b1 + (i8)half_carry;
        new_v = ires < -8 || ires > 7;
        #ifdef M6502
        change_c(new_c);
        change_v(new_v);
        return;
        #endif
    } else
#endif
    {
        /* R2A03 always takes this path; others take it in binary mode. */
        u16 ires = (i16)(i8)CPU->op_arg + (i16)(i8)CPU->a + carry;
        u16 ures = CPU->op_arg + CPU->a + carry;
        CPU->a = (u8)ures;
        new_c  = ures > 0xFF;
        new_v   = ((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80));
    }
    change_c(new_c);
    change_v(new_v);
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void PLA(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    dummy_read(CPU->s | 0x100);
    CPU->a = pop;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void BVS(CPU_TYPE* CPU) {
    branch_on(CPU->op_arg, CPU->p & SET_V);
}

static void SEI(CPU_TYPE* CPU) {
    CPU->p |= SET_I;
    dummy_read(CPU->pc);
}

static void STA(CPU_TYPE* CPU) {
    write_byte(CPU->mem_addr, CPU->a);
}

static void DEY(CPU_TYPE* CPU) {
    CPU->y -= 1;
    calculate_n(CPU->y);
    calculate_z(CPU->y);
    dummy_read(CPU->pc);
}

static void TXA(CPU_TYPE* CPU) {
    CPU->a = CPU->x;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
    dummy_read(CPU->pc);
}

static void TYA(CPU_TYPE* CPU) {
    CPU->a = CPU->y;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
    dummy_read(CPU->pc);
}

static void TXS(CPU_TYPE* CPU) {
    CPU->s = CPU->x;
    dummy_read(CPU->pc);
}

static void LDY(CPU_TYPE* CPU) { ld(CPU->y); }
static void LDA(CPU_TYPE* CPU) { ld(CPU->a); }
static void LDX(CPU_TYPE* CPU) { ld(CPU->x); }

static void TAY(CPU_TYPE* CPU) {
    CPU->y = CPU->a;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
    dummy_read(CPU->pc);
}

static void TAX(CPU_TYPE* CPU) {
    CPU->x = CPU->a;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
    dummy_read(CPU->pc);
}

static void CLV(CPU_TYPE* CPU) {
    CPU->p &= CLEAR_V;
    dummy_read(CPU->pc);
}

static void TSX(CPU_TYPE* CPU) {
    CPU->x = CPU->s;
    calculate_n(CPU->s);
    calculate_z(CPU->s);
    dummy_read(CPU->pc);
}

static void CPY(CPU_TYPE* CPU) {
    get_arg;
    CPU->op_arg = ~CPU->op_arg;
    u16 ures = CPU->y + CPU->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void CMP(CPU_TYPE* CPU) {
    get_arg;
    CPU->op_arg = ~CPU->op_arg;
    u16 ures = CPU->a + CPU->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void DEC(CPU_TYPE* CPU) {
    get_arg;
#ifdef W65C02
    dummy_read(CPU->mem_addr);
    CPU->op_arg -= 1;
    calculate_n(CPU->op_arg);
    calculate_z(CPU->op_arg);
    write_back(CPU->op_arg);
#else
    write_byte(CPU->mem_addr, CPU->op_arg);
    CPU->op_arg -= 1;
    calculate_n(CPU->op_arg);
    calculate_z(CPU->op_arg);
    write_byte(CPU->mem_addr, CPU->op_arg);
#endif
}

static void INY(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    CPU->y += 1;
    calculate_n(CPU->y);
    calculate_z(CPU->y);
}

static void DEX(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    CPU->x -= 1;
    calculate_n(CPU->x);
    calculate_z(CPU->x);
}

static void BNE(CPU_TYPE* CPU) {
    branch_on(CPU->op_arg, !(CPU->p & SET_Z));
}

static void CLD(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    CPU->p &= CLEAR_D;
}

static void CPX(CPU_TYPE* CPU) {
    get_arg;
    CPU->op_arg = ~CPU->op_arg;
    u16 ures = CPU->x + CPU->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void SBC(CPU_TYPE* CPU) {
    get_arg;
    bool carry = CPU->p & SET_C;
    bool new_c, new_v;

#ifdef W65C02
    /* W65C02: full BCD support */
    if (CPU->p & SET_D) {
        dummy_read(CPU->mem_addr);
        u8 a0  = CPU->a & 0xF;
        u8 b0  = CPU->op_arg & 0xF;
        u16 tmp     = a0 - b0 - !carry;
        u16 res     = CPU->a - CPU->op_arg - !carry;
        u16 bin_res = CPU->a + ~CPU->op_arg + carry;
        if (res & 0x8000) res -= 0x60;
        if (tmp & 0x8000) res -= 0x06;
        new_v = (CPU->a ^ bin_res) & (~CPU->op_arg ^ bin_res) & 0x80;
        new_c = (u16)res <= (u16)CPU->a || (res & 0xff0) == 0xff0;
        CPU->a = (u8)res;
    } else
#elif defined(M6502)
    /* M6502: BCD supported, N/Z/V/C set from binary result before decimal adjust */
    if (CPU->p & SET_D) {
        u16 tmp = CPU->a - CPU->op_arg - !carry;
        new_c = tmp < 0x100;
        new_v = ((CPU->a ^ tmp) & 0x80) && ((CPU->a ^ CPU->op_arg) & 0x80);
        calculate_n(tmp);
        calculate_z((u8)tmp);
        u8 lo = (CPU->a & 0x0F) - (CPU->op_arg & 0x0F) - !carry;
        u8 hi = (CPU->a >> 4) - (CPU->op_arg >> 4);
        if ((lo & 0x10) != 0) { lo = ((lo - 6) & 0x0F); hi--; }
        if ((hi & 0x10) != 0) { hi = ((hi - 6) & 0x0F); }
        CPU->a = (hi << 4) | lo;
        change_c(new_c);
        change_v(new_v);
        return;
    } else
#endif
    {
        /* R2A03 always takes this path; others take it in binary mode. */
        CPU->op_arg = ~CPU->op_arg;
        u16 ires = (i16)(i8)CPU->op_arg + (i16)(i8)CPU->a + carry;
        u16 ures = CPU->op_arg + CPU->a + carry;
        CPU->a   = (u8)ures;
        new_c    = ures > 0xFF;
        new_v    = ((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80));
    }
    change_c(new_c);
    change_v(new_v);
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void INC(CPU_TYPE* CPU) {
    get_arg;
#ifdef W65C02
    dummy_read(CPU->mem_addr);
    CPU->op_arg += 1;
    calculate_n(CPU->op_arg);
    calculate_z(CPU->op_arg);
    write_back(CPU->op_arg);
#else
    write_byte(CPU->mem_addr, CPU->op_arg);
    CPU->op_arg += 1;
    calculate_n(CPU->op_arg);
    calculate_z(CPU->op_arg);
    write_byte(CPU->mem_addr, CPU->op_arg);
#endif
}

static void INX(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    CPU->x += 1;
    calculate_n(CPU->x);
    calculate_z(CPU->x);
}

static void BEQ(CPU_TYPE* CPU) {
    branch_on(CPU->op_arg, CPU->p & SET_Z);
}

static void SED(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    CPU->p |= SET_D;
}

static void ROL(CPU_TYPE* CPU) {
    get_arg;
    bool c = CPU->op_arg & 0x80;
    if (CPU->in_mem) {
#ifdef W65C02
        dummy_read(CPU->mem_addr);
#else
        write_byte(CPU->mem_addr, CPU->op_arg);
#endif
    } else {
        dummy_read(CPU->pc);
    }
    CPU->op_arg = (CPU->op_arg << 1) | ((bool)(CPU->p & SET_C));
    change_c(c);
    calculate_n(CPU->op_arg);
    calculate_z(CPU->op_arg);
    write_back(CPU->op_arg);
}

static void ROR(CPU_TYPE* CPU) {
    get_arg;
    bool c = CPU->op_arg & 1;
    if (CPU->in_mem) {
#ifdef W65C02
        dummy_read(CPU->mem_addr);
#else
        write_byte(CPU->mem_addr, CPU->op_arg);
#endif
    } else {
        dummy_read(CPU->pc);
    }
    CPU->op_arg = (CPU->op_arg >> 1) | (((bool)(CPU->p & SET_C)) << 7);
    change_c(c);
    calculate_n(CPU->op_arg);
    calculate_z(CPU->op_arg);
    write_back(CPU->op_arg);
}

static void STY(CPU_TYPE* CPU) {
    write_byte(CPU->mem_addr, CPU->y);
}

static void STX(CPU_TYPE* CPU) {
    write_byte(CPU->mem_addr, CPU->x);
}

static void BCS(CPU_TYPE* CPU) {
    branch_on(CPU->op_arg, CPU->p & SET_C);
}

static void BCC(CPU_TYPE* CPU) {
    branch_on(CPU->op_arg, !(CPU->p & SET_C));
}

#ifdef W65C02

static void BRA(CPU_TYPE* CPU) {
    branch_on(CPU->op_arg, true);
}

static void PHX(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    push(CPU->x);
}

static void PHY(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    push(CPU->y);
}

static void PLX(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    dummy_read(CPU->s | 0x100);
    CPU->x = pop;
    calculate_n(CPU->x);
    calculate_z(CPU->x);
}

static void PLY(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    dummy_read(CPU->s | 0x100);
    CPU->y = pop;
    calculate_n(CPU->y);
    calculate_z(CPU->y);
}

static void STZ(CPU_TYPE* CPU) {
    write_byte(CPU->mem_addr, 0);
}

static void TSB(CPU_TYPE* CPU) {
    get_arg;
    dummy_read(CPU->mem_addr);
    calculate_z(CPU->op_arg & CPU->a);
    write_byte(CPU->mem_addr, CPU->op_arg | CPU->a);
}

static void TRB(CPU_TYPE* CPU) {
    get_arg;
    dummy_read(CPU->mem_addr);
    calculate_z(CPU->op_arg & CPU->a);
    write_byte(CPU->mem_addr, CPU->op_arg & ~CPU->a);
}

static void WAI(CPU_TYPE* CPU) {
    CPU->wait = true;
}

static void STP(CPU_TYPE* CPU) {
    CPU->stop = true;
}

#endif

#if defined(R2A03) || defined(M6502)

static void NOP1(CPU_TYPE* CPU) {
    dummy_read(CPU->mem_addr);
}

static void NOP2(CPU_TYPE* CPU) {
}

#endif

#ifdef M6502

static void JAM(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    dummy_read(CPU->pc);
    CPU->pc -= 1;
}

#endif

#if defined(R2A03) || defined(M6502)

static void SLO(CPU_TYPE* CPU) {
    get_arg;
    change_c(CPU->op_arg & 0x80);
    write_byte(CPU->mem_addr, CPU->op_arg);
    CPU->op_arg <<= 1;
    CPU->a |= CPU->op_arg;
    write_byte(CPU->mem_addr, CPU->op_arg);
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void ANC(CPU_TYPE* CPU) {
    get_arg;
    CPU->a &= CPU->op_arg;
    change_c(CPU->a & 0x80);
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void ALR(CPU_TYPE* CPU) {
    get_arg;
    CPU->a &= CPU->op_arg;
    change_c(CPU->a & 1);
    CPU->a >>= 1;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void ARR(CPU_TYPE* CPU) {
    get_arg;
    bool carry = CPU->p & SET_C;
    u8   tmp   = carry << 7;
    CPU->a &= CPU->op_arg;
    change_c(CPU->a & 0x80);
    CPU->a = (CPU->a >> 1) | tmp;
    carry   = CPU->p & SET_C;
    change_v(carry ^ ((CPU->a >> 5) & 1));
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void SAX(CPU_TYPE* CPU) {
    write_byte(CPU->mem_addr, CPU->a & CPU->x);
}

static void ANE(CPU_TYPE* CPU) {
    CPU->a |= MAGIC_BYTE;
    CPU->a &= CPU->x;
    CPU->a &= CPU->op_arg;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void TAS(CPU_TYPE* CPU) {
    u8   no_y_addr = (CPU->mem_addr - CPU->y) & 0xFF;
    bool swap      = no_y_addr + CPU->y > 0xFF;
    u8   hi        = CPU->mem_addr >> 8;
    CPU->s = CPU->a & CPU->x;
    if (swap) {
        hi &= CPU->s;
        CPU->mem_addr = (CPU->mem_addr & 0xFF) | (hi << 8);
    }
    write_byte(CPU->mem_addr, CPU->s & (hi + !swap));
}

static void SHY(CPU_TYPE* CPU) {
    u8   no_x_addr = (CPU->mem_addr - CPU->x) & 0xFF;
    bool swap      = no_x_addr + CPU->x > 0xFF;
    u8   hi        = CPU->mem_addr >> 8;
    if (swap) {
        hi &= CPU->y;
        CPU->mem_addr = (CPU->mem_addr & 0xFF) | (hi << 8);
    }
    write_byte(CPU->mem_addr, CPU->y & (hi + !swap));
}

static void SHX(CPU_TYPE* CPU) {
    u8   no_y_addr = (CPU->mem_addr - CPU->y) & 0xFF;
    bool swap      = no_y_addr + CPU->y > 0xFF;
    u8   hi        = CPU->mem_addr >> 8;
    if (swap) {
        hi &= CPU->x;
        CPU->mem_addr = (CPU->mem_addr & 0xFF) | (hi << 8);
    }
    write_byte(CPU->mem_addr, CPU->x & (hi + !swap));
}

static void SHA(CPU_TYPE* CPU) {
    u8   no_y_addr = (CPU->mem_addr - CPU->y) & 0xFF;
    bool swap      = no_y_addr + CPU->y > 0xFF;
    u8   hi        = CPU->mem_addr >> 8;
    u8   tmp       = CPU->a & CPU->x;
    if (swap) {
        hi &= tmp;
        CPU->mem_addr = (CPU->mem_addr & 0xFF) | (hi << 8);
    }
    write_byte(CPU->mem_addr, tmp & (hi + !swap));
}

static void LAX(CPU_TYPE* CPU) {
    get_arg;
    CPU->a = CPU->x = CPU->op_arg;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void LXA(CPU_TYPE* CPU) {
    CPU->a |= MAGIC_BYTE;
    CPU->a &= CPU->op_arg;
    CPU->x  = CPU->a;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void LAS(CPU_TYPE* CPU) {
    get_arg;
    CPU->a = CPU->op_arg & CPU->s;
    CPU->x = CPU->a;
    CPU->s = CPU->a;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void SBX(CPU_TYPE* CPU) {
    u8  tmp = CPU->a & CPU->x;
    CPU->op_arg = ~CPU->op_arg;
    u16 ures = tmp + CPU->op_arg + 1;
    CPU->x  = (u8)ures;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void USBC(CPU_TYPE* CPU) {
    SBC(CPU);
}

static void ISC(CPU_TYPE* CPU) {
    get_arg;
    write_byte(CPU->mem_addr, CPU->op_arg);
    CPU->op_arg += 1;
    write_byte(CPU->mem_addr, CPU->op_arg);

    CPU->op_arg = ~CPU->op_arg;
    u16 ires = (i16)(i8)CPU->op_arg + (i16)(i8)CPU->a + (bool)(CPU->p & SET_C);
    u16 ures = CPU->op_arg + CPU->a + (bool)(CPU->p & SET_C);
    CPU->a  = (u8)ures;
    change_c(ures > 0xFF);
    change_v(((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80)));
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void DCP(CPU_TYPE* CPU) {
    get_arg;
    write_byte(CPU->mem_addr, CPU->op_arg);
    CPU->op_arg -= 1;
    write_byte(CPU->mem_addr, CPU->op_arg);

    CPU->op_arg = ~CPU->op_arg;
    u16 ures = CPU->a + CPU->op_arg + 1;
    change_c(ures > 0xFF);
    calculate_n(ures);
    calculate_z(ures);
}

static void RLA(CPU_TYPE* CPU) {
    get_arg;
    bool carry = CPU->p & SET_C;
    change_c(CPU->op_arg & 0x80);
    write_byte(CPU->mem_addr, CPU->op_arg);
    CPU->op_arg = (CPU->op_arg << 1) | carry;
    CPU->a &= CPU->op_arg;
    write_byte(CPU->mem_addr, CPU->op_arg);
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

static void SRE(CPU_TYPE* CPU) {
    get_arg;
    change_c(CPU->op_arg & 1);
    write_byte(CPU->mem_addr, CPU->op_arg);
    CPU->op_arg >>= 1;
    write_byte(CPU->mem_addr, CPU->op_arg);
    CPU->a ^= CPU->op_arg;
    calculate_n(CPU->a);
    calculate_z(CPU->a);
}

#endif

static void RRA(CPU_TYPE* CPU) {
    get_arg;
    bool c = CPU->p & SET_C;
    bool carry = CPU->op_arg & 1;
    write_byte(CPU->mem_addr, CPU->op_arg);
    CPU->op_arg = (CPU->op_arg >> 1) | (c << 7);
    write_byte(CPU->mem_addr, CPU->op_arg);

#ifdef M6502
    bool new_c, new_v;
    if (CPU->p & SET_D) {
        u8 a0 = CPU->a & 0xF,  a1 = CPU->a >> 4;
        u8 b0 = CPU->op_arg & 0xF, b1 = CPU->op_arg >> 4;
        u8 lo = a0 + b0 + carry;
        u8 hi = a1 + b1;
        bool half_carry = false;
        if (lo > 9) { half_carry = true; hi += 1; lo += 6; }
        calculate_n(hi << 4);
        calculate_z(CPU->a + CPU->op_arg + carry);
        if (hi > 9) { hi += 6; }
        CPU->a = (hi << 4) | (lo & 0x0F);
        new_c = hi > 9;
        if (a1 & 0x08) a1 |= 0xF0;
        if (b1 & 0x08) b1 |= 0xF0;
        i8 ires = (i8)a1 + (i8)b1 + (i8)half_carry;
        new_v = ires < -8 || ires > 7;
    } else {
        u16 ires = (i16)(i8)CPU->op_arg + (i16)(i8)CPU->a + carry;
        u16 ures = CPU->op_arg + CPU->a + carry;
        CPU->a = (u8)ures;
        new_c = ures > 0xFF;
        new_v = ((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80));
        calculate_n(CPU->a);
        calculate_z(CPU->a);
    }
    change_c(new_c);
    change_v(new_v);
#else
    u16 ires = (i16)(i8)CPU->op_arg + (i16)(i8)CPU->a + carry;
    u16 ures = CPU->op_arg + CPU->a + carry;
    CPU->a  = (u8)ures;
    change_c(ures > 0xFF);
    change_v(((bool)(ires & 0xFF00)) ^ ((bool)(ires & 0x80)));
    calculate_n(CPU->a);
    calculate_z(CPU->a);
#endif
}

#ifdef R2A03

static void JAM(CPU_TYPE* CPU) {
    dummy_read(CPU->pc);
    dummy_read(0xFFFF);
    dummy_read(0xFFFE);
    dummy_read(0xFFFE);
    dummy_read(0xFFFF);
    for (int i = 0; i < 5; i++)
        dummy_read(0xFFFF);
}

#endif