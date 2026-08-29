#include "chips/w65c02.h"

#include <stdio.h>

#define W65C02
#define CPU_TYPE w65c02_t
#define CPU_VAR w
#include "chips/isa_65X02.h"

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