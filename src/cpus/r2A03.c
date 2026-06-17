#include "cpus/r2A03.h"

#include <stdio.h>

#define R2A03
#define CPU_TYPE r2A03_t
#define CPU_VAR r
#include "cpus/isa_65X02.h"

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