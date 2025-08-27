#include "cpus/z80.h"

#include <stdio.h>
#include <string.h>

typedef void (*rotateFunc)(z80_t*, uint8_t*);
typedef void (*aluFunc)(z80_t*, uint8_t*, uint8_t);
typedef void (*blockFunc)(z80_t*);

static void useDDRegisterTable(z80_t*, uint8_t**, uint16_t**, uint16_t**);
static void useFDRegisterTable(z80_t*, uint8_t**, uint16_t**, uint16_t**);
static void restoreRegisterTable(z80_t*, uint8_t**, uint16_t**, uint16_t**);
static void adjustDDorFFOpcode(z80_t*, uint8_t**, uint16_t**, uint16_t**);

static void NOP(z80_t*);
static void EX(z80_t*, uint16_t*, uint16_t*);
static void DJNZ(z80_t*, int8_t);
static void JR(z80_t*, int8_t);
static void JRNZ(z80_t*, int8_t);
static void JRZ(z80_t*, int8_t);
static void JRNC(z80_t*, int8_t);
static void JRC(z80_t*, int8_t);
static void LD_8(z80_t*, uint8_t*, uint8_t);
static void LD_16(z80_t*, uint16_t*, uint16_t);
static void ADD_16(z80_t*, uint16_t*, uint16_t*);
static void INC_16(z80_t*, uint16_t*);
static void DEC_16(z80_t*, uint16_t*);
static void INC_8(z80_t*, uint8_t*);
static void DEC_8(z80_t*, uint8_t*);
static void DAA(z80_t*);
static void CPL(z80_t*);
static void SCF(z80_t*);
static void CCF(z80_t*);
static void HLT(z80_t*);
static void RETNZ(z80_t*);
static void RETZ(z80_t*);
static void RETNC(z80_t*);
static void RETC(z80_t*);
static void RETPO(z80_t*);
static void RETPE(z80_t*);
static void RETP(z80_t*);
static void RETM(z80_t*);
static void POP(z80_t*, uint16_t*);
static void RET(z80_t*);
static void EXX(z80_t*);
static void JP(z80_t*, uint16_t);
static void JPNZ(z80_t*, uint16_t);
static void JPZ(z80_t*, uint16_t);
static void JPNC(z80_t*, uint16_t);
static void JPC(z80_t*, uint16_t);
static void JPPO(z80_t*, uint16_t);
static void JPPE(z80_t*, uint16_t);
static void JPP(z80_t*, uint16_t);
static void JPM(z80_t*, uint16_t);
static void EI(z80_t*);
static void DI(z80_t*);
static void CALL(z80_t*, uint16_t);
static void CALLNZ(z80_t*, uint16_t);
static void CALLZ(z80_t*, uint16_t);
static void CALLNC(z80_t*, uint16_t);
static void CALLC(z80_t*, uint16_t);
static void CALLPO(z80_t*, uint16_t);
static void CALLPE(z80_t*, uint16_t);
static void CALLP(z80_t*, uint16_t);
static void CALLM(z80_t*, uint16_t);
static void PUSH(z80_t*, uint16_t);
static void RST(z80_t*, uint8_t);
static void RLC(z80_t*, uint8_t*);
static void RRC(z80_t*, uint8_t*);
static void RLCA(z80_t*);
static void RRCA(z80_t*);
static void RLA(z80_t*);
static void RRA(z80_t*);
static void RLC(z80_t*, uint8_t*);
static void RRC(z80_t*, uint8_t*);
static void RL(z80_t*, uint8_t*);
static void RR(z80_t*, uint8_t*);
static void SLA(z80_t*, uint8_t*);
static void SRA(z80_t*, uint8_t*);
static void SLL(z80_t*, uint8_t*);
static void SRL(z80_t*, uint8_t*);
static void BIT(z80_t*, uint8_t, uint8_t*);
static void SET(z80_t*, uint8_t, uint8_t*);
static void RES(z80_t*, uint8_t, uint8_t*);
static void ADD(z80_t*, uint8_t*, uint8_t);
static void ADC(z80_t*, uint8_t*, uint8_t);
static void SUB(z80_t*, uint8_t*, uint8_t);
static void SBC(z80_t*, uint8_t*, uint8_t);
static void AND(z80_t*, uint8_t*, uint8_t);
static void XOR(z80_t*, uint8_t*, uint8_t);
static void OR(z80_t*, uint8_t*, uint8_t);
static void CP(z80_t*, uint8_t*, uint8_t);
static void ADC_16(z80_t*, uint16_t*, uint16_t);
static void SBC_16(z80_t*, uint16_t*, uint16_t);
static void NEG(z80_t*, uint8_t*);
static void RETI(z80_t*);
static void RETN(z80_t*);
static void IM(z80_t*, uint8_t);
static void RRD(z80_t*);
static void RLD(z80_t*);

static void LDI(z80_t*);
static void LDD(z80_t*);
static void LDIR(z80_t*);
static void LDDR(z80_t*);
static void CPI(z80_t*);
static void CPD(z80_t*);
static void CPIR(z80_t*);
static void CPDR(z80_t*);
static void INI(z80_t*);
static void IND(z80_t*);
static void INIR(z80_t*);
static void INDR(z80_t*);
static void OUTI(z80_t*);
static void OUTD(z80_t*);
static void OTIR(z80_t*);
static void OTDR(z80_t*);

#define SET_S   0b10000000
#define SET_Z   0b01000000
#define SET_Y   0b00100000
#define SET_H   0b00010000
#define SET_X   0b00001000
#define SET_P   0b00000100
#define SET_N   0b00000010
#define SET_C   0b00000001

#define CLEAR_S 0b01111111
#define CLEAR_Z 0b10111111
#define CLEAR_Y 0b11011111
#define CLEAR_H 0b11101111
#define CLEAR_X 0b11110111
#define CLEAR_P 0b11111011
#define CLEAR_N 0b11111101
#define CLEAR_C 0b11111110

#define SET_FLAG(f)          z80->F |= SET_ ## f
#define CLEAR_FLAG(f)        z80->F &= CLEAR_ ## f
#define CHANGE_FLAG(f, val)  z80->F ^= (-!!(val) ^ z80->F) & SET_ ## f

static void incrementR(uint8_t*);
static void setParity(z80_t*, uint8_t);
static void setZero(z80_t*, uint16_t);
static void setSign8Bit(z80_t*, uint8_t);
static void setSign16Bit(z80_t*, uint16_t);
static bool calculateCarry(int, uint16_t, uint16_t, bool);

static uint16_t readHalfWord(z80_t*, uint16_t);
static void writeHalfWord(z80_t*, uint16_t, uint16_t);

void z80_init(z80_t* z80){
    z80->AF = 0xFFFF;
    z80->SP = 0xFFFF;
    z80->PC = 0;

    z80->AF_ = 0;
    z80->BC_ = 0;
    z80->DE_ = 0;
    z80->HL_ = 0;
    z80->I = 0;
    z80->R = 0;
    z80->Q = false;
    z80->WZ = 0;
    z80->HALTED = false;
    z80->INTERRUPT_MODE    = 1;
    z80->IFF1 = false;
    z80->IFF2 = false;
    z80->INTERRUPT_PENDING = false;
    z80->INTERRUPT_DELAY = false;
    z80->INTERRUPT_VECT = 0x0000;
    z80->cycles = 0;
}

static void restoreRegisterTable(z80_t* z80, uint8_t** r, uint16_t** rp, uint16_t** rp2){
    r[0] = &z80->B;
    r[1] = &z80->C;
    r[2] = &z80->D;
    r[3] = &z80->E;
    r[4] = &z80->H;
    r[5] = &z80->L;
    r[6] = &z80->aux_reg;
    r[7] = &z80->A;
    rp[0] = &z80->BC;
    rp[1] = &z80->DE;
    rp[2] = &z80->HL;
    rp[3] = &z80->SP;
    rp2[0] = &z80->BC;
    rp2[1] = &z80->DE;
    rp2[2] = &z80->HL;
    rp2[3] = &z80->AF;
}

static void useDDRegisterTable(z80_t* z80, uint8_t** r, uint16_t** rp, uint16_t** rp2){
    r[4] = &z80->IXH;
    r[5] = &z80->IXL;
    rp[2] = &z80->IX;
    rp2[2] = &z80->IX;
}

static void useFDRegisterTable(z80_t* z80, uint8_t** r, uint16_t** rp, uint16_t** rp2){
    r[4] = &z80->IYH;
    r[5] = &z80->IYL;
    rp[2] = &z80->IY;
    rp2[2] = &z80->IY;
}

static void adjustDDorFFOpcode(z80_t* z80, uint8_t** r, uint16_t** rp, uint16_t** rp2){
    z80->PC += 1;
    r[4] = &z80->H;
    r[5] = &z80->L;
}

void z80_print(z80_t* z80){
    fprintf(stderr, "PC: %04X, AF: %04X, BC: %04X, DE: %04X, HL: %04X, SP: %04X, "
         "IX: %04X, IY: %04X, AF_: %04X, BC_: %04X, DE_: %04X, HL_: %04X\n"
         "I: %02X, R: %02X IM: %02X\n",
      z80->PC, z80->AF, z80->BC, z80->DE, z80->HL, z80->SP, z80->IX, z80->IY,
      z80->AF_, z80->BC_, z80->DE_, z80->HL_,
      z80->I, z80->R, z80->INTERRUPT_MODE
    );

    fprintf(stderr, "S: %d ", (bool)(z80->F & SET_S));
    fprintf(stderr, "Z: %d ", (bool)(z80->F & SET_Z));
    fprintf(stderr, "C: %d ", (bool)(z80->F & SET_C));
    fprintf(stderr, "P: %d ", (bool)(z80->F & SET_P));
    fprintf(stderr, "H: %d ", (bool)(z80->F & SET_H));
    fprintf(stderr, "N: %d ", (bool)(z80->F & SET_N));
    fprintf(stderr, "X: %d ", (bool)(z80->F & SET_X));
    fprintf(stderr, "Y: %d\n", (bool)(z80->F & SET_Y));
    fprintf(stderr, "Q: %X\n", z80->Q);
    fprintf(stderr, "WZ: %04X\n", z80->WZ);
    fprintf(stderr, "IFF1 %d IFF2: %d\n", z80->IFF1, z80->IFF2);
    fprintf(stderr, "INTERRUPT DELAY: %d\n", z80->INTERRUPT_DELAY);
    fprintf(stderr, "OPCODE: 0x%02X %02X %02X\n", z80->readMemory(z80, z80->PC), z80->readMemory(z80, z80->PC+1), z80->readMemory(z80, z80->PC+2));
    fprintf(stderr, "cycles: %u\n\n", z80->cycles);
}

static void processInterrupt(z80_t* z80){
    z80->IFF1 = false;
    z80->IFF2 = false;
    z80->INTERRUPT_PENDING = false;
    z80->HALTED = false;
    switch(z80->INTERRUPT_MODE){
        case 1:
        RST(z80, 0x07);
        z80->cycles += 13;
        break;
        
        case 2:
        {
            uint16_t interruptAddress = readHalfWord(z80, (z80->I << 8) | z80->INTERRUPT_VECT);
            CALL(z80, interruptAddress);
            z80->cycles += 19;
        }
        break;
    }
}

void z80_nmi(z80_t* z80){
    CALL(z80, 0x66);
    z80->cycles += 11;
    z80->IFF1 = false;
}

void z80_step(z80_t* z80){
    if(!z80->INTERRUPT_DELAY && z80->IFF1 && z80->INTERRUPT_PENDING){
        processInterrupt(z80);
        return;
    }

    
    z80->INTERRUPT_DELAY = false;

    if(z80->HALTED){
        z80->cycles += 4;
        return;
    }

    // increase 7 bits of R register
    incrementR(&(z80->R));
        
    #ifdef DEBUG
        infoCPU(z80);
    #endif

    const static rotateFunc rot[8]  = { RLC, RRC, RL,   RR,   SLA, SRA,   SLL,  SRL   };
    const static aluFunc    alu[8]  = { ADD, ADC, SUB,  SBC,  AND,  XOR,  OR,   CP    };
    const static blockFunc  bli[16] = { LDI, LDD, LDIR, LDDR, CPI,  CPD,  CPIR, CPDR,
                                        INI, IND, INIR, INDR, OUTI, OUTD, OTIR, OTDR  };
    const static uint8_t    im[8]   = {   0,   0,    1,    2,    0,    0,    1,    2  };

    uint8_t* r[8];
    uint16_t* rp[4];
    uint16_t* rp2[4];

    uint8_t opcode = z80->readMemory(z80, z80->PC);
    uint8_t x;
    uint8_t y;
    uint8_t z;
    uint8_t q;
    uint8_t p;

    uint8_t  val8;
    uint16_t val16;
    uint16_t nn;
    uint16_t old_PC;
    uint8_t old_F = z80->F;
    bool prefixDD = false;
    bool prefixFD = false;
    aluFunc function;

    restoreRegisterTable(z80, r, rp, rp2);
    switch(opcode){
        case 0xDD:
        case 0xFD:
        z80->cycles += 4;
        incrementR(&z80->R);
        switch(opcode){
            case 0xDD:
            useDDRegisterTable(z80, r, rp, rp2);
            prefixDD = true;
            break;

            case 0xFD:
            useFDRegisterTable(z80, r, rp, rp2);
            prefixFD = true;
            break;
        }
        z80->PC += 1;
        break;
    }

    opcode = z80->readMemory(z80, z80->PC);

    switch(opcode){
        case 0xCB:
        if(prefixDD || prefixFD)
            z80->PC += 1;
        else
            incrementR(&z80->R);

        z80->PC += 1;
        opcode = z80->readMemory(z80, z80->PC);
        x = opcode >> 6;
        y = (opcode >> 3) & 0b111;
        z = opcode & 0b111;
        q = y & 0b1;
        p = (y >> 1) & 0b11;

        z80->PC += 1;
        if(prefixDD || prefixFD){
            nn = *rp[2] + (int8_t)z80->readMemory(z80, z80->PC - 2);
            z80->WZ = nn;
            z80->aux_reg = z80->readMemory(z80, nn);
        } else {
            nn = z80->HL;
            if(z == 6)
                z80->aux_reg = z80->readMemory(z80, nn);
        }
        switch(x){
            case 0:
            {
                rotateFunc function = rot[y];
                if(prefixDD || prefixFD){
                    r[4] = &z80->H;
                    r[5] = &z80->L;
                    (*function)(z80, &z80->aux_reg);
                    *r[z] = z80->aux_reg;
                } else
                    (*function)(z80, r[z]);
            }
            break;

            case 1:
            if(prefixDD || prefixFD){
                BIT(z80, y, &z80->aux_reg);
                CHANGE_FLAG(X, (nn >> 8) & (1 << 3));
                CHANGE_FLAG(Y, (nn >> 8) & (1 << 5));
            } else {
                BIT(z80, y, r[z]);
                if(z == 6){
                    CHANGE_FLAG(X, z80->WZ & (1 << 11));
                    CHANGE_FLAG(Y, z80->WZ & (1 << 13));
                } 
            }
            break;

            case 2:
            if(prefixDD || prefixFD){
                RES(z80, y, &z80->aux_reg);
                r[4] = &z80->H;
                r[5] = &z80->L;
                *r[z] = z80->aux_reg;
            } else
                RES(z80, y, r[z]);
            break;

            case 3:
            if(prefixDD || prefixFD){
                SET(z80, y, &z80->aux_reg);
                r[4] = &z80->H;
                r[5] = &z80->L;
                *r[z] = z80->aux_reg;
            } else
                SET(z80, y, r[z]);
            break;
        }
        if(prefixDD || prefixFD){
            if(x == 1)
                z80->cycles += 16;
            else {
                z80->writeMemory(z80, nn, *r[z]);
                z80->cycles += 19;
            }
        } else if(z == 6 && !prefixDD && !prefixDD){
            if(x == 1)
                z80->cycles += 12;
            else {
                z80->writeMemory(z80, nn, z80->aux_reg);
                z80->cycles += 15;
            }
        } else
            z80->cycles += 8;
        break;

        case 0xED:
        z80->PC = z80->PC + 1;
        incrementR(&z80->R);
        opcode = z80->readMemory(z80, z80->PC);
        x = opcode >> 6;
        y = (opcode >> 3) & 0b111;
        z = opcode & 0b111;
        q = y & 0b1;
        p = (y >> 1) & 0b11;

        switch(x){
            case 0:
            case 3:
            z80->PC += 1;
            NOP(z80);
            break;

            case 1:
            switch(z){
                case 0:
                z80->PC += 1;
                z80->WZ = z80->BC + 1;
                if(y != 6){
                    *r[y] = z80->readIO(z80, z80->BC);
                    setSign8Bit(z80, *r[y]);
                    setParity(z80, *r[y]);
                    setZero(z80, *r[y]);
                    CHANGE_FLAG(X, *r[y] & (1 << 3));
                    CHANGE_FLAG(Y, *r[y] & (1 << 5));
                    CLEAR_FLAG(H);
                    CLEAR_FLAG(N);
                } else {
                    uint8_t copy = z80->A;
                    z80->A = z80->readIO(z80, z80->BC);
                    setSign8Bit(z80, z80->A);
                    setParity(z80, z80->A);
                    setZero(z80, z80->A);
                    CHANGE_FLAG(X, z80->A & (1 << 3));
                    CHANGE_FLAG(Y, z80->A & (1 << 5));
                    CLEAR_FLAG(H);
                    CLEAR_FLAG(N);
                    z80->A = copy;
                }
                z80->cycles += 12;
                break;

                case 1:
                z80->PC += 1;
                z80->WZ = z80->BC + 1;
                if(y != 6){
                    z80->writeIO(z80, z80->BC, *r[y]);
                } else {
                    z80->writeIO(z80, z80->BC, 0);
                }
                z80->cycles += 12;
                break;

                case 2:
                z80->PC += 1;
                z80->WZ = z80->HL + 1;
                if(q == 0)
                    SBC_16(z80, &z80->HL, *rp[p]);
                else
                    ADC_16(z80, &z80->HL, *rp[p]);
                z80->cycles += 15;
                break;

                case 3:
                nn = readHalfWord(z80, z80->PC+1);
                z80->PC += 3;
                if(q == 0)
                    writeHalfWord(z80, nn, *rp[p]);
                else
                    *rp[p] = readHalfWord(z80, nn);
                z80->WZ = nn + 1;
                z80->cycles += 20;
                break;

                case 4:
                z80->PC += 1;
                NEG(z80, &z80->A);
                z80->cycles += 8;
                break;

                case 5:
                z80->PC += 1;
                if(y != 1)
                    RETN(z80);
                else
                    RETI(z80);
                z80->cycles += 14;
                break;

                case 6:
                z80->PC += 1;
                IM(z80, im[y]);
                z80->cycles += 8;
                break;

                case 7:
                z80->PC += 1;
                switch(y){
                    case 0:
                    z80->I = z80->A;
                    z80->cycles += 9;
                    break;

                    case 1:
                    z80->R = z80->A;
                    z80->cycles += 9;
                    break;

                    case 2:
                    z80->A = z80->I;
                    setSign8Bit(z80, z80->A);
                    setZero(z80, z80->A);
                    CLEAR_FLAG(H);
                    CLEAR_FLAG(N);
                    CHANGE_FLAG(P, z80->IFF2);
                    CHANGE_FLAG(X, z80->A & (1 << 3));
                    CHANGE_FLAG(Y, z80->A & (1 << 5));
                    z80->cycles += 9;
                    break;

                    case 3:
                    z80->A = z80->R;
                    setSign8Bit(z80, z80->A);
                    setZero(z80, z80->A);
                    CLEAR_FLAG(H);
                    CLEAR_FLAG(N);
                    CHANGE_FLAG(P, z80->IFF2);
                    CHANGE_FLAG(X, z80->A & (1 << 3));
                    CHANGE_FLAG(Y, z80->A & (1 << 5));
                    z80->cycles += 9;
                    break;

                    case 4:
                    z80->WZ = z80->HL + 1;
                    RRD(z80);
                    z80->cycles += 18;
                    break;

                    case 5:
                    z80->WZ = z80->HL + 1;
                    RLD(z80);
                    z80->cycles += 18;
                    break;

                    case 6:
                    NOP(z80);
                    z80->cycles += 4;
                    break;

                    case 7:
                    NOP(z80);
                    z80->cycles += 4;
                    break;
                }
                break;
            }
            break;

            case 2:
            z80->PC += 1;
            blockFunc function = bli[(y-4)+z*4];
            (*function)(z80);
            break;
        }
        break;

        default:
        x = opcode >> 6;
        y = (opcode >> 3) & 0b111;
        z = opcode & 0b111;
        q = y & 0b1;
        p = (y >> 1) & 0b11;

        switch(x){
            case 0:
            switch(z){
                case 0:
                switch(y){
                    case 0:
                    z80->PC += 1;
                    NOP(z80);
                    break;

                    case 1:
                    z80->PC += 1;
                    EX(z80, &z80->AF, &z80->AF_);
                    z80->cycles += 4;
                    break;

                    case 2:
                    z80->PC += 2;
                    DJNZ(z80, z80->readMemory(z80, z80->PC-1));
                    break;

                    case 3:
                    z80->PC += 2;
                    JR(z80, z80->readMemory(z80, z80->PC-1));
                    z80->cycles += 12;
                    break;

                    case 4:
                    z80->PC += 2;
                    JRNZ(z80, z80->readMemory(z80, z80->PC-1));
                    break;

                    case 5:
                    z80->PC += 2;
                    JRZ(z80, z80->readMemory(z80, z80->PC-1));
                    break;

                    case 6:
                    z80->PC += 2;
                    JRNC(z80, z80->readMemory(z80, z80->PC-1));
                    break;

                    case 7:
                    z80->PC += 2;
                    JRC(z80, z80->readMemory(z80, z80->PC-1));
                    break;
                }
                break;

                case 1:
                switch(q){
                    case 0:
                    nn = readHalfWord(z80, z80->PC+1);
                    z80->PC += 3;
                    *rp[p] = nn;
                    z80->cycles += 10;
                    break;

                    case 1:
                    z80->PC += 1;
                    z80->WZ = *rp[2] + 1;
                    ADD_16(z80, rp[2], rp[p]);
                    z80->cycles += 11;
                    break;
                }
                break;

                case 2:
                switch(q){
                    case 0:
                    switch(p){
                        case 0:
                        z80->PC += 1;
                        z80->writeMemory(z80, z80->BC, z80->A);
                        z80->W = z80->A;
                        z80->Z = z80->C + 1;
                        z80->cycles += 7;
                        break;

                        case 1:
                        z80->PC += 1;
                        z80->writeMemory(z80, z80->DE, z80->A);
                        z80->W = z80->A;
                        z80->Z = z80->E + 1;
                        z80->cycles += 7;
                        break;

                        case 2:
                        nn = readHalfWord(z80, z80->PC+1);
                        z80->PC += 3;
                        writeHalfWord(z80, nn, *rp[2]);
                        z80->WZ = nn + 1;
                        z80->cycles += 16;
                        break;

                        case 3:
                        nn = readHalfWord(z80, z80->PC+1);
                        z80->PC += 3;
                        z80->writeMemory(z80, nn, z80->A);
                        z80->W = z80->A;
                        z80->Z = nn + 1;
                        z80->cycles += 13;
                        break;
                    }
                    break;

                    case 1:
                    switch(p){
                        case 0:
                        z80->PC += 1;
                        z80->A = z80->readMemory(z80, z80->BC);
                        z80->WZ = z80->BC + 1;
                        z80->cycles += 7;
                        break;

                        case 1:
                        z80->PC += 1;
                        z80->A = z80->readMemory(z80, z80->DE);
                        z80->WZ = z80->DE + 1;
                        z80->cycles += 7;
                        break;

                        case 2:
                        nn = readHalfWord(z80, z80->PC+1);
                        z80->WZ = nn + 1;
                        val16 = readHalfWord(z80, nn);
                        z80->PC += 3;
                        *rp[2] = val16;
                        z80->cycles += 16;
                        break;

                        case 3:
                        nn = readHalfWord(z80, z80->PC+1);
                        val8 = z80->readMemory(z80, nn);
                        z80->PC += 3;
                        z80->A = val8;
                        z80->WZ = nn + 1;
                        z80->cycles += 13;
                        break;
                    }
                    break;
                }
                break;

                case 3:
                switch(q){
                    case 0:
                    z80->PC += 1;
                    INC_16(z80, rp[p]);
                    break;

                    case 1:
                    z80->PC += 1;
                    DEC_16(z80, rp[p]);
                    break;
                }
                z80->cycles += 6;
                break;

                case 4:
                z80->PC += 1;
                if(y == 6 && (prefixDD || prefixFD)){
                    nn = *rp[2] + (int8_t)z80->readMemory(z80, z80->PC);
                    z80->WZ = nn;
                    z80->PC += 1;
                    z80->cycles += 19;
                } else if(y == 6 && !prefixDD && !prefixFD) {
                    nn = z80->HL;
                    z80->cycles += 11;
                } else 
                    z80->cycles += 4;
                if(y == 6)
                    z80->aux_reg = z80->readMemory(z80, nn);
                INC_8(z80, r[y]);
                if(y == 6)
                    z80->writeMemory(z80, nn, z80->aux_reg);
                break;

                case 5:
                z80->PC += 1;
                if(y == 6 && (prefixDD || prefixFD)){
                    nn = *rp[2] + (int8_t)z80->readMemory(z80, z80->PC);
                    z80->WZ = nn;
                    z80->PC += 1;
                    z80->cycles += 19;
                } else if(y == 6 && !prefixDD && !prefixFD){
                    nn = z80->HL;
                    z80->cycles += 11;
                } else 
                    z80->cycles += 4;
                if(y == 6)
                    z80->aux_reg = z80->readMemory(z80, nn);
                DEC_8(z80, r[y]);
                if(y == 6)
                    z80->writeMemory(z80, nn, z80->aux_reg);
                break;

                case 6:
                if(y == 6 && (prefixDD || prefixFD)){
                    nn = *rp[2] + (int8_t)z80->readMemory(z80, z80->PC+1);
                    z80->WZ = nn;
                    adjustDDorFFOpcode(z80, r, rp, rp2);
                    z80->cycles += 15;
                } else if(y == 6 && !prefixDD && !prefixFD) {
                    nn = z80->HL;
                    z80->cycles += 10;
                } else
                    z80->cycles += 7;
                val8 = z80->readMemory(z80, z80->PC+1);
                z80->PC += 2;
                *r[y] = val8;
                if(y == 6)
                    z80->writeMemory(z80, nn, z80->aux_reg);
                break;

                case 7:
                z80->PC += 1;
                switch(y){
                    case 0:
                    RLCA(z80);
                    break;

                    case 1:
                    RRCA(z80);
                    break;

                    case 2:
                    RLA(z80);
                    break;

                    case 3:
                    RRA(z80);
                    break;

                    case 4:
                    DAA(z80);
                    break;
                    
                    case 5:
                    CPL(z80);
                    break;

                    case 6:
                    SCF(z80);
                    break;

                    case 7:
                    CCF(z80);
                    break;
                }
                z80->cycles += 4;
                break;
            }
            break;

            case 1:
            z80->PC += 1;
            if(z == 6 && y == 6){
                HLT(z80);
                z80->cycles += 4;
            } else {
                if((z == 6 || y == 6) && (prefixDD || prefixFD)){
                    nn = *rp[2] + (int8_t)z80->readMemory(z80, z80->PC);
                    z80->WZ = nn;
                    adjustDDorFFOpcode(z80, r, rp, rp2);
                    z80->cycles += 15;
                } else if((z == 6 || y == 6) && !prefixDD && !prefixFD){
                    nn = z80->HL;
                    z80->cycles += 7;
                } else
                    z80->cycles += 4;
                if(z == 6)
                    z80->aux_reg = z80->readMemory(z80, nn);
                *r[y] = *r[z];
                if(y == 6)
                    z80->writeMemory(z80, nn, z80->aux_reg);
            }
            break;

            case 2:
            if((prefixDD || prefixFD) && z == 6){
                z80->PC += 1;
                nn = *rp[2] + (int8_t)z80->readMemory(z80, z80->PC);
                z80->WZ = nn;
                z80->cycles += 15;
            } else if(!prefixDD && !prefixFD && z == 6){
                nn = z80->HL;
                z80->cycles += 7;
            } else
                z80->cycles += 4;
            if(z == 6)
                z80->aux_reg = z80->readMemory(z80, nn);
            function = alu[y];
            z80->PC += 1;
            (*function)(z80, &z80->A, *r[z]);
            if(z == 6)
                z80->writeMemory(z80, nn, z80->aux_reg);
            break;

            case 3:
            switch(z){
                case 0:
                z80->PC += 1;
                old_PC = z80->PC;
                switch(y){
                    case 0:
                    RETNZ(z80);
                    break;

                    case 1:
                    RETZ(z80);
                    break;

                    case 2:
                    RETNC(z80);
                    break;

                    case 3:
                    RETC(z80);
                    break;

                    case 4:
                    RETPO(z80);
                    break;

                    case 5:
                    RETPE(z80);
                    break;

                    case 6:
                    RETP(z80);
                    break;

                    case 7:
                    RETM(z80);
                    break;
                }
                if(old_PC == z80->PC)
                    z80->cycles += 5;
                else
                    z80->cycles += 11;
                break;

                case 1:
                z80->PC += 1;
                switch(q){
                    case 0:
                    POP(z80, rp2[p]);
                    z80->cycles += 10;
                    break;

                    case 1:
                    switch(p){
                        case 0:
                        RET(z80);
                        z80->cycles += 10;
                        break;

                        case 1:
                        EXX(z80);
                        z80->cycles += 4;
                        break;

                        case 2:
                        z80->PC = *rp[2];
                        z80->cycles += 4;
                        break;

                        case 3:
                        z80->SP = *rp[2];
                        z80->cycles += 6;
                        break;
                    }
                    break;
                }
                break;

                case 2:
                val16 = readHalfWord(z80, z80->PC+1);
                z80->PC += 3;
                switch(y){
                    case 0:
                    JPNZ(z80, val16);
                    break;

                    case 1:
                    JPZ(z80, val16);
                    break;

                    case 2:
                    JPNC(z80, val16);
                    break;

                    case 3:
                    JPC(z80, val16);
                    break;
                    
                    case 4:
                    JPPO(z80, val16);
                    break;

                    case 5:
                    JPPE(z80, val16);
                    break;

                    case 6:
                    JPP(z80, val16);
                    break;

                    case 7:
                    JPM(z80, val16);
                    break;
                }
                z80->cycles += 10;
                break;
            
                case 3:
                switch(y){
                    case 0:
                    val16 = readHalfWord(z80, z80->PC+1);
                    z80->PC += 3;
                    JP(z80, val16);
                    z80->cycles += 10;
                    break;
                    
                    case 2:
                    val8 = z80->readMemory(z80, z80->PC+1);
                    z80->PC += 2;
                    z80->writeIO(z80, val8, z80->A);
                    z80->W = z80->A;
                    z80->Z = val8 + 1;
                    z80->cycles += 11;
                    break;
                    
                    case 3:
                    val16 = (z80->A << 8) | z80->readMemory(z80, z80->PC+1);
                    z80->WZ = val16 + 1;
                    z80->PC += 2;
                    z80->A = z80->readIO(z80, val16);
                    z80->cycles += 11;
                    break;

                    case 4:
                    z80->PC += 1;
                    val16 = readHalfWord(z80, z80->SP);
                    z80->WZ = val16;
                    EX(z80, &val16, rp[2]);
                    writeHalfWord(z80, z80->SP, val16);
                    z80->cycles += 19;
                    break;

                    case 5:
                    z80->PC += 1;
                    EX(z80, &z80->DE, &z80->HL);
                    z80->cycles += 4;
                    break;

                    case 6:
                    z80->PC += 1;
                    DI(z80);
                    z80->cycles += 4;
                    break;

                    case 7:
                    z80->PC += 1;
                    EI(z80);
                    z80->cycles += 4;
                    break;
                }
                break;

                case 4:
                val16 = readHalfWord(z80, z80->PC+1);
                z80->PC += 3;
                old_PC = z80->PC;
                switch(y){
                    case 0:
                    CALLNZ(z80, val16);
                    break;
                    
                    case 1:
                    CALLZ(z80, val16);
                    break;
                    
                    case 2:
                    CALLNC(z80, val16);
                    break;
                    
                    case 3:
                    CALLC(z80, val16);
                    break;
                    
                    case 4:
                    CALLPO(z80, val16);
                    break;
                    
                    case 5:
                    CALLPE(z80, val16);
                    break;
                    
                    case 6:
                    CALLP(z80, val16);
                    break;
                    
                    case 7:
                    CALLM(z80, val16);
                    break;
                }
                if(old_PC == z80->PC)
                    z80->cycles += 10;
                else
                    z80->cycles += 17;
                break;

                case 5:
                switch(q){
                    case 0:
                    z80->PC += 1;
                    PUSH(z80, *rp2[p]);
                    z80->cycles += 11;
                    break;

                    case 1:
                    if(p == 0){
                        val16 = readHalfWord(z80, z80->PC+1);
                        z80->PC += 3;
                        CALL(z80, val16);
                        z80->cycles += 17;
                    }
                    break;
                }
                break;

                case 6:
                val8 = z80->readMemory(z80, z80->PC+1);
                function = alu[y];
                z80->PC += 2;
                (*function)(z80, &z80->A, val8);
                z80->cycles += 7;
                break;

                case 7:
                z80->PC += 1;
                RST(z80, y);
                z80->cycles += 11;
                break;
            }
            break;
        }
        break;
    }
    
    z80->Q = z80->F != old_F;
}

// Z80 INSTRUCTIONS

static void NOP(z80_t* z80){
    z80->cycles += 4;
}

static void EX(z80_t* z80, uint16_t* regA, uint16_t* regB){
    uint16_t tmp = *regA;
    *regA = *regB;
    *regB = tmp;
}

static void DJNZ(z80_t* z80, int8_t d){
    z80->B -= 1;   
    if(z80->B != 0){
        JR(z80, d);
        z80->cycles += 13;
    } else
        z80->cycles += 8;  
}

static void JR(z80_t* z80, int8_t d){
    z80->PC += d;
    z80->WZ = z80->PC;
}

static void JRNZ(z80_t* z80, int8_t d){
    z80->cycles += 7;
    if(!(z80->F & SET_Z)){
        JR(z80, d);
        z80->cycles += 5;
    }
}

static void JRZ(z80_t* z80, int8_t d){
    z80->cycles += 7;
    if((z80->F & SET_Z)){
        JR(z80, d);
        z80->cycles += 5;
    }
}

static void JRNC(z80_t* z80, int8_t d){
    z80->cycles += 7;
    if(!(z80->F & SET_C)){
        JR(z80, d);
        z80->cycles += 5;
    }
}

static void JRC(z80_t* z80, int8_t d){
    z80->cycles += 7;
    if(z80->F & SET_C){
        JR(z80, d);
        z80->cycles += 5;
    }
}

static void ADD_16(z80_t* z80, uint16_t* regDst, uint16_t* regSrc){
    CLEAR_FLAG(N);
    bool carry = calculateCarry(16, *regDst, *regSrc, 0);
    CHANGE_FLAG(C, carry);

    bool aux_carry = calculateCarry(12, *regDst, *regSrc, 0);
    CHANGE_FLAG(H, aux_carry);

    *regDst += *regSrc;

    CHANGE_FLAG(Y, (*regDst >> 8) & 0b100000);
    CHANGE_FLAG(X, (*regDst >> 8) & 0b1000);
}

static void INC_16(z80_t* z80, uint16_t* reg){
    *reg += 1;
}

static void DEC_16(z80_t* z80, uint16_t* reg){
    *reg -= 1;
}

static void INC_8(z80_t* z80, uint8_t* reg){
    bool aux_carry = calculateCarry(4, *reg, 1, 0);
    CHANGE_FLAG(H, aux_carry);
    CHANGE_FLAG(P, *reg == 0x7f);
    CLEAR_FLAG(N);

    *reg += 1;
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);

    CHANGE_FLAG(Y, *reg & 0b100000);
    CHANGE_FLAG(X, *reg & 0b1000);
}

static void DEC_8(z80_t* z80, uint8_t* reg){
    bool aux_carry = calculateCarry(4, *reg, -1, 0);
    CHANGE_FLAG(H, !aux_carry);
    CHANGE_FLAG(P, *reg == 0x80);
    SET_FLAG(N);

    *reg -= 1;
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);

    CHANGE_FLAG(Y, *reg & 0b100000);
    CHANGE_FLAG(X, *reg & 0b1000);
}

static void DAA(z80_t* z80){
    // explanation at:
    //      http://z80-heaven.wikidot.com/instructions-set:daa
    uint8_t correction = 0;
    if((z80->A & 0X0F) > 0x09 || (z80->F & SET_H))
        correction += 0x06;

    if((z80->A > 0x99) || (z80->F & SET_C)){
        correction += 0x60;
        SET_FLAG(C);
    }

    bool new_half;
    if(z80->F & SET_N){
        bool old_half = (z80->F & SET_H);
        new_half = old_half && (z80->A & 0x0F) < 0x06;
        z80->A -= correction;
    } else {
        new_half = (z80->A & 0x0F) > 0x09;
        z80->A += correction;
    }
    CHANGE_FLAG(H, new_half);

    setSign8Bit(z80, z80->A);
    setZero(z80, z80->A);
    setParity(z80, z80->A);

    CHANGE_FLAG(X, z80->A & 0b1000);
    CHANGE_FLAG(Y, z80->A & 0b100000);
}

static void CPL(z80_t* z80){
    z80->A = ~(z80->A);
    SET_FLAG(N);
    SET_FLAG(H);
    CHANGE_FLAG(X, z80->A & 0b1000);
    CHANGE_FLAG(Y, z80->A & 0b100000);
}

static void SCF(z80_t* z80){
    SET_FLAG(C);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);
    if(z80->Q){
        CHANGE_FLAG(X, (z80->A & (1 << 3)));
        CHANGE_FLAG(Y, (z80->A & (1 << 5)));
    } else {
        if(z80->A & (1 << 3))
            SET_FLAG(X);
        if(z80->A & (1 << 5))
            SET_FLAG(Y);
    }
}

static void CCF(z80_t* z80){
    bool carry = z80->F & SET_C;
    CHANGE_FLAG(C, !carry);
    CHANGE_FLAG(H, carry);
    CLEAR_FLAG(N);
    if(z80->Q){
        CHANGE_FLAG(X, (z80->A & (1 << 3)));
        CHANGE_FLAG(Y, (z80->A & (1 << 5)));
    } else {
        if(z80->A & (1 << 3))
            SET_FLAG(X);
        if(z80->A & (1 << 5))
            SET_FLAG(Y);
    }
}

static void HLT(z80_t* z80){
    z80->HALTED = true;
}

static void RETNZ(z80_t* z80){
    if(!(z80->F & SET_Z))
        RET(z80);
}

static void RETZ(z80_t* z80){
    if(z80->F & SET_Z)
        RET(z80);
}

static void RETNC(z80_t* z80){
    if(!(z80->F & SET_C))
        RET(z80);
}

static void RETC(z80_t* z80){
    if(z80->F & SET_C)
        RET(z80);
}

static void RETPO(z80_t* z80){
    if(!(z80->F & SET_P))
        RET(z80);
}

static void RETPE(z80_t* z80){
    if(z80->F & SET_P)
        RET(z80);
}

static void RETP(z80_t* z80){
    if(!(z80->F & SET_S))
        RET(z80);
}

static void RETM(z80_t* z80){
    if(z80->F & SET_S)
        RET(z80);
}

static void POP(z80_t* z80, uint16_t* reg){
    *reg = readHalfWord(z80, z80->SP);
    z80->SP = z80->SP + 2; 
}

static void RET(z80_t* z80){
    POP(z80, &z80->PC);
    z80->WZ = z80->PC;
}

static void EXX(z80_t* z80){
    EX(z80, &z80->BC, &z80->BC_);
    EX(z80, &z80->DE, &z80->DE_);
    EX(z80, &z80->HL, &z80->HL_);
}

static void JP(z80_t* z80, uint16_t val){
    z80->PC = val;
    z80->WZ = val;
}

static void JPNZ(z80_t* z80, uint16_t val){
    if(!(z80->F & SET_Z))
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPZ(z80_t* z80, uint16_t val){
    if(z80->F & SET_Z)
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPNC(z80_t* z80, uint16_t val){
    if(!(z80->F & SET_C))
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPC(z80_t* z80, uint16_t val){
    if(z80->F & SET_C)
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPPO(z80_t* z80, uint16_t val){
    if(!(z80->F & SET_P))
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPPE(z80_t* z80, uint16_t val){
    if(z80->F & SET_P)
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPP(z80_t* z80, uint16_t val){
    if(!(z80->F & SET_S))
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPM(z80_t* z80, uint16_t val){
    if(z80->F & SET_S)
        JP(z80, val);
    else
        z80->WZ = val;
}

static void DI(z80_t* z80){
    z80->IFF1 = false;
    z80->IFF2 = false;
}

static void EI(z80_t* z80){
    z80->INTERRUPT_DELAY = true;
    z80->IFF1 = true;
    z80->IFF2 = true;
}

static void CALL(z80_t* z80, uint16_t val){
    PUSH(z80, z80->PC);
    z80->PC = val;
    z80->WZ = val;
}

static void CALLNZ(z80_t* z80, uint16_t val){
    if(!(z80->F & SET_Z))
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLZ(z80_t* z80, uint16_t val){
    if(z80->F & SET_Z)
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLNC(z80_t* z80, uint16_t val){
    if(!(z80->F & SET_C))
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLC(z80_t* z80, uint16_t val){
    if(z80->F & SET_C)
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLPO(z80_t* z80, uint16_t val){
    if(!(z80->F & SET_P))
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLPE(z80_t* z80, uint16_t val){
    if(z80->F & SET_P)
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLP(z80_t* z80, uint16_t val){
    if(!(z80->F & SET_S))
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLM(z80_t* z80, uint16_t val){
    if(z80->F & SET_S)
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void PUSH(z80_t* z80, uint16_t val){
    z80->SP = z80->SP - 2;
    writeHalfWord(z80, z80->SP, val);
}

static void RST(z80_t* z80, uint8_t addr){
    CALL(z80, addr*8);
}

static void RLCA(z80_t* z80){
    bool carry = z80->A & 0b10000000;
    CHANGE_FLAG(C, carry);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);

    z80->A = (z80->A << 1) | (carry);
    
    CHANGE_FLAG(X, z80->A & 0b1000);
    CHANGE_FLAG(Y, z80->A & 0b100000);
}

static void RRCA(z80_t* z80){
    bool carry = z80->A & 0b1;
    CHANGE_FLAG(C, carry);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);
    z80->A = (z80->A >> 1) | (carry << 7);

    CHANGE_FLAG(Y, z80->A & 0b100000);
    CHANGE_FLAG(X, z80->A & 0b1000);
}

static void RLA(z80_t* z80){
    bool carry = z80->F & SET_C;
    CHANGE_FLAG(C, z80->A & 0b10000000);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);
    z80->A = (z80->A << 1) | carry;

    CHANGE_FLAG(X, z80->A & 0b1000);
    CHANGE_FLAG(Y, z80->A & 0b100000);
}

static void RRA(z80_t* z80){
    bool carry = z80->F & SET_C;
    CHANGE_FLAG(C, z80->A & 0b1);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);
    z80->A = (z80->A >> 1) | (carry << 7);

    CHANGE_FLAG(X, z80->A & 0b1000);
    CHANGE_FLAG(Y, z80->A & 0b100000);
}

static void RLC(z80_t* z80, uint8_t* reg){
    bool msb = *reg >> 7;
    CHANGE_FLAG(C, msb);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);
    *reg = (*reg << 1) | msb;
    
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    setParity(z80, *reg);
    
    CHANGE_FLAG(X, *reg & 0b1000);
    CHANGE_FLAG(Y, *reg & 0b100000);
}

static void RRC(z80_t* z80, uint8_t* reg){
    bool lsb = *reg & 0b1;
    CHANGE_FLAG(C, lsb);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);
    *reg = (lsb << 7) | (*reg >> 1);

    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    setParity(z80, *reg);
    
    CHANGE_FLAG(X, *reg & 0b1000);
    CHANGE_FLAG(Y, *reg & 0b100000);
}

static void RL(z80_t* z80, uint8_t* reg){
    bool carry = z80->F & SET_C;
    bool msb = *reg >> 7;
    *reg = (*reg << 1) | carry;
    CHANGE_FLAG(C, msb);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);

    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    setParity(z80, *reg);
    
    CHANGE_FLAG(X, *reg & 0b1000);
    CHANGE_FLAG(Y, *reg & 0b100000);
}

static void RR(z80_t* z80, uint8_t* reg){
    bool carry = z80->F & SET_C;
    bool lsb = *reg & 0b1;
    *reg = (*reg >> 1) | (carry << 7);
    CHANGE_FLAG(C, lsb);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);

    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    setParity(z80, *reg);

    CHANGE_FLAG(X, *reg & 0b1000);
    CHANGE_FLAG(Y, *reg & 0b100000);
}

static void SLA(z80_t* z80, uint8_t* reg){
    bool msb = *reg >> 7;
    CHANGE_FLAG(C, msb);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);

    *reg = *reg << 1;

    setParity(z80, *reg);
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    
    CHANGE_FLAG(X, *reg & 0b1000);
    CHANGE_FLAG(Y, *reg & 0b100000);
}

static void SRA(z80_t* z80, uint8_t* reg){
    bool sign = *reg >> 7;
    bool lsb = *reg & 0b1;
    CHANGE_FLAG(C, lsb);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);

    *reg = (*reg >> 1) | (sign << 7);

    setParity(z80, *reg);
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);

    CHANGE_FLAG(X, *reg & 0b1000);
    CHANGE_FLAG(Y, *reg & 0b100000);
}

static void SLL(z80_t* z80, uint8_t* reg){
    bool msb = *reg >> 7;
    CHANGE_FLAG(C, msb);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);

    *reg = *reg << 1;
    *reg = *reg | 0b1;

    setParity(z80, *reg);
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);

    CHANGE_FLAG(X, *reg & 0b1000);
    CHANGE_FLAG(Y, *reg & 0b100000);
}


static void SRL(z80_t* z80, uint8_t* reg){
    bool lsb = *reg & 0b1;
    CHANGE_FLAG(C, lsb);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);

    *reg = *reg >> 1;
   
    setParity(z80, *reg);
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    
    CHANGE_FLAG(X, *reg & 0b1000);
    CHANGE_FLAG(Y, *reg & 0b100000);
}

static void BIT(z80_t* z80, uint8_t bit, uint8_t* reg){
    uint8_t masked_bit = *reg & (1 << bit);
    CHANGE_FLAG(Z, masked_bit == 0);
    CHANGE_FLAG(P, masked_bit == 0);
    CHANGE_FLAG(S, masked_bit & 0x80);
    SET_FLAG(H);
    CLEAR_FLAG(N);
    CHANGE_FLAG(Y, *reg & 0b100000);
    CHANGE_FLAG(X, *reg & 0b1000);
}

static void RES(z80_t* z80, uint8_t bit, uint8_t* reg){
    *reg = *reg & (~(uint8_t)(1 << bit));
}

static void SET(z80_t* z80, uint8_t bit, uint8_t* reg){
    *reg = * reg | (1 << bit);
}

static void ADD(z80_t* z80, uint8_t* reg, uint8_t val){
    uint8_t res = *reg + val;
    bool carry = calculateCarry(8, *reg, val, 0);
    CHANGE_FLAG(C, carry);

    bool aux_carry = calculateCarry(4, *reg, val, 0);
    CHANGE_FLAG(H, aux_carry);
    
    bool overflow = calculateCarry(7, *reg, val, 0) != calculateCarry(8, *reg, val, 0);
    CHANGE_FLAG(P, overflow);

    *reg = res;
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    CLEAR_FLAG(N);

    CHANGE_FLAG(Y, res & 0b100000);
    CHANGE_FLAG(X, res & 0b1000);
}

static void ADC(z80_t* z80, uint8_t* reg, uint8_t val){
    bool carry = z80->F & SET_C;
    uint8_t res = *reg + val + carry;

    bool new_carry = calculateCarry(8, *reg, val, carry);
    CHANGE_FLAG(C, new_carry);

    bool aux_carry = calculateCarry(4, *reg, val, carry);
    CHANGE_FLAG(H, aux_carry);
    
    bool overflow = calculateCarry(7, *reg, val, carry) != calculateCarry(8, *reg, val, carry);
    CHANGE_FLAG(P, overflow);

    *reg = res;
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    CLEAR_FLAG(N);

    CHANGE_FLAG(Y, *reg & 0b100000);
    CHANGE_FLAG(X, *reg & 0b1000);
}

static void SUB(z80_t* z80, uint8_t* reg, uint8_t val){
    val = ~val + 1;
    bool carry = calculateCarry(8, *reg, val - 1, 1);
    CHANGE_FLAG(C, !carry);

    bool aux_carry = calculateCarry(4, *reg, val - 1, 1);
    CHANGE_FLAG(H, !aux_carry);

    uint8_t res = *reg + val;

    bool overflow = calculateCarry(7, *reg, val - 1, 1) != calculateCarry(8, *reg, val - 1, 1);
    CHANGE_FLAG(P, overflow);

    *reg = res;
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    SET_FLAG(N);

    CHANGE_FLAG(Y, res & 0b100000);
    CHANGE_FLAG(X, res & 0b1000);
}

static void SBC(z80_t* z80, uint8_t* reg, uint8_t val){
    val = ~val + 1;
    bool carry = z80->F & SET_C;
    bool new_carry = calculateCarry(8, *reg, val - 1, !carry);
    bool aux_carry = calculateCarry(4, *reg, val - 1, !carry);

    CHANGE_FLAG(C, !new_carry);
    CHANGE_FLAG(H, !aux_carry);

    uint8_t res = *reg + val - carry;
    bool overflow = calculateCarry(7, *reg, val - 1, !carry) != calculateCarry(8, *reg, val - 1, !carry);
    CHANGE_FLAG(P, overflow); 

    *reg = res;
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    SET_FLAG(N);

    CHANGE_FLAG(Y, *reg & 0b100000);
    CHANGE_FLAG(X, *reg & 0b1000);
}

static void AND(z80_t* z80, uint8_t* reg, uint8_t val){
    *reg &= val;
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    SET_FLAG(H);
    CLEAR_FLAG(C);
    CLEAR_FLAG(N);
    setParity(z80, *reg);

    CHANGE_FLAG(Y, *reg & 0b100000);
    CHANGE_FLAG(X, *reg & 0b1000);
}

static void XOR(z80_t* z80, uint8_t* reg, uint8_t val){
    *reg ^= val;
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    setParity(z80, *reg);
    CLEAR_FLAG(H);
    CLEAR_FLAG(C);
    CLEAR_FLAG(N);
    CHANGE_FLAG(Y, *reg & 0b100000);
    CHANGE_FLAG(X, *reg & 0b1000);
}

static void OR(z80_t* z80, uint8_t* reg, uint8_t val){
    *reg |= val;
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    setParity(z80, *reg);
    CLEAR_FLAG(H);
    CLEAR_FLAG(C);
    CLEAR_FLAG(N);
    CHANGE_FLAG(Y, *reg & 0b100000);
    CHANGE_FLAG(X, *reg & 0b1000);
}

static void CP(z80_t* z80, uint8_t* reg, uint8_t val){
    uint8_t copy = *reg;
    SUB(z80, reg, val);
    *reg = copy;

    CHANGE_FLAG(Y, val & 0b100000);
    CHANGE_FLAG(X, val & 0b1000);
}

static void ADC_16(z80_t* z80, uint16_t* reg, uint16_t val){
    bool carry = z80->F & SET_C;

    bool new_carry = calculateCarry(16, *reg, val, carry);
    CHANGE_FLAG(C, new_carry);
    
    bool aux_carry = calculateCarry(12, *reg, val, carry);
    CHANGE_FLAG(H, aux_carry);

    uint16_t res = *reg + val + carry;
    bool overflow = calculateCarry(16, *reg, val, carry) != calculateCarry(15, *reg, val, carry);
    CHANGE_FLAG(P, overflow);
           
    *reg = res;
    setSign16Bit(z80, *reg);
    setZero(z80, *reg);
    CLEAR_FLAG(N);

    CHANGE_FLAG(Y, (res >> 8) & 0b100000);
    CHANGE_FLAG(X, (res >> 8) & 0b1000);
}

static void SBC_16(z80_t* z80, uint16_t* reg, uint16_t val){
    val = ~val + 1;
    bool carry = z80->F & SET_C;
    bool new_carry = calculateCarry(16, *reg, val - 1, !carry);
    bool aux_carry = calculateCarry(12, *reg, val - 1, !carry);

    CHANGE_FLAG(C, !new_carry);
    CHANGE_FLAG(H, !aux_carry);

    uint16_t res = *reg + val - carry;
    bool overflow = calculateCarry(16, *reg, val - 1, !carry) != calculateCarry(15, *reg, val - 1, !carry);
    CHANGE_FLAG(P, overflow);  

    *reg = res;
    setSign16Bit(z80, *reg);
    setZero(z80, *reg);
    SET_FLAG(N);

    CHANGE_FLAG(Y, (res >> 8) & 0b100000);
    CHANGE_FLAG(X, (res >> 8) & 0b1000);
}

static void NEG(z80_t* z80, uint8_t* reg){
    uint8_t tmp = *reg;
    *reg = 0;
    SUB(z80, reg, tmp);
}

static void RETI(z80_t* z80){
    RET(z80);
    z80->IFF1 = z80->IFF2;
}

static void RETN(z80_t* z80){
    RET(z80);
    z80->IFF1 = z80->IFF2;
}

static void IM(z80_t* z80, uint8_t im_mode){
    z80->INTERRUPT_MODE = im_mode;
}

static void RRD(z80_t* z80){
    uint8_t tmpA = z80->A;
    uint8_t val = z80->readMemory(z80, z80->HL);
    z80->A = (tmpA & 0xF0) | (val & 0xF);
    z80->writeMemory(z80,  z80->HL, (val >> 4) | (tmpA << 4) );
    setSign8Bit(z80, z80->A);
    setZero(z80, z80->A);
    setParity(z80, z80->A);
    CLEAR_FLAG(N);
    CLEAR_FLAG(H);
    CHANGE_FLAG(X, z80->A & 0b1000);
    CHANGE_FLAG(Y, z80->A & 0b100000);
}

static void RLD(z80_t* z80){
    uint8_t tmpA = z80->A;
    uint8_t val = z80->readMemory(z80, z80->HL);
    z80->A = (tmpA & 0xF0) | (val >> 4);
    z80->writeMemory(z80, z80->HL, (val << 4) | (tmpA & 0xF));
    setSign8Bit(z80, z80->A);
    setZero(z80, z80->A);
    setParity(z80, z80->A);
    CLEAR_FLAG(N);
    CLEAR_FLAG(H);
    CHANGE_FLAG(X, z80->A & 0b1000);
    CHANGE_FLAG(Y, z80->A & 0b100000);
}

// block instructions

static void LDI(z80_t* z80){
    uint8_t hl_val = z80->readMemory(z80, z80->HL);
    z80->writeMemory(z80, z80->DE, hl_val);
    z80->DE += 1;
    z80->HL += 1;
    z80->BC -= 1;

    CHANGE_FLAG(P, z80->BC != 0);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);

    z80->cycles += 16;

    uint8_t res = z80->A + hl_val;
    CHANGE_FLAG(X, res & (1 << 3));
    CHANGE_FLAG(Y, res & (1 << 1));
}

static void LDD(z80_t* z80){
    LDI(z80);
    z80->HL -= 2;
    z80->DE -= 2;
}

static void LDIR(z80_t* z80){
    uint8_t hl_val = z80->readMemory(z80, z80->HL);
    z80->writeMemory(z80, z80->DE, hl_val);
    z80->DE += 1;
    z80->HL += 1;
    z80->BC -= 1;
    CHANGE_FLAG(P, z80->BC != 0);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);

    if(z80->BC != 0){
        z80->PC -= 1;
        z80->WZ = z80->PC;
        z80->PC -= 1;
        CHANGE_FLAG(X, z80->PC & (1 << 11));
        CHANGE_FLAG(Y, z80->PC & (1 << 13));
        z80->cycles += 21;
    } else {
        uint8_t res = z80->A + hl_val;
        CHANGE_FLAG(X, res & (1 << 3));
        CHANGE_FLAG(Y, res & (1 << 1));
        z80->cycles += 16;
    }
}

static void LDDR(z80_t* z80){
    uint8_t hl_val = z80->readMemory(z80, z80->HL);
    z80->writeMemory(z80, z80->DE, hl_val);
    z80->DE -= 1;
    z80->HL -= 1;
    z80->BC -= 1;

    CHANGE_FLAG(P, z80->BC != 0);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);

    if(z80->BC != 0){
        z80->PC -= 1;
        z80->WZ = z80->PC;
        z80->PC -= 1;
        CHANGE_FLAG(X, z80->PC & (1 << 11));
        CHANGE_FLAG(Y, z80->PC & (1 << 13));
        z80->cycles += 21;
    } else {
        uint8_t res = z80->A + hl_val;
        CHANGE_FLAG(X, res & (1 << 3));
        CHANGE_FLAG(Y, res & (1 << 1));
        z80->cycles += 16;
    }
}

static void CPI(z80_t* z80){
    bool carry = (bool)(z80->F & SET_C);
    uint8_t memory_val = z80->readMemory(z80, z80->HL);
    CP(z80, &z80->A, memory_val);
    z80->HL += 1;
    z80->BC -= 1;

    CHANGE_FLAG(P, z80->BC != 0);
    CHANGE_FLAG(C, carry);
    SET_FLAG(N);

    bool aux_carry = (bool)(z80->F & SET_H);
    uint8_t val = z80->A - memory_val - aux_carry;
    
    CHANGE_FLAG(Y, val & 0b10);
    CHANGE_FLAG(X, val & 0b1000);

    z80->WZ += 1;
    z80->cycles += 16;
}

static void CPD(z80_t* z80){
    bool carry = (bool)(z80->F & SET_C);
    uint8_t memory_val = z80->readMemory(z80, z80->HL);
    CP(z80, &z80->A, memory_val);
    z80->HL -= 1;
    z80->BC -= 1;

    CHANGE_FLAG(P, z80->BC != 0);
    CHANGE_FLAG(C, carry);
    SET_FLAG(N);

    bool aux_carry = (bool)(z80->F & SET_H);
    uint8_t val = z80->A - memory_val - aux_carry;
    
    CHANGE_FLAG(Y, val & 0b10);
    CHANGE_FLAG(X, val & 0b1000);

    z80->WZ -= 1;
    z80->cycles += 16;
}

static void CPIR(z80_t* z80){
    bool carry = (bool)(z80->F & SET_C);
    uint8_t memory_val = z80->readMemory(z80, z80->HL);
    CP(z80, &z80->A, memory_val);
    z80->HL += 1;
    z80->BC -= 1;

    CHANGE_FLAG(P, z80->BC != 0);
    CHANGE_FLAG(C, carry);
    SET_FLAG(N);

    bool aux_carry = (bool)(z80->F & SET_H);
    uint8_t val = z80->A - memory_val - aux_carry;
    
    if(z80->BC != 0 && !(bool)(z80->F & SET_Z)){
        z80->PC -= 1;
        z80->WZ = z80->PC;
        z80->PC -= 1;
        CHANGE_FLAG(X, z80->PC & (1 << 11));
        CHANGE_FLAG(Y, z80->PC & (1 << 13));
        z80->cycles += 21;
    } else {
        z80->WZ += 1;
        CHANGE_FLAG(Y, val & 0b10);
        CHANGE_FLAG(X, val & 0b1000);
        z80->cycles += 16;
    }
}

static void CPDR(z80_t* z80){
    bool carry = (bool)(z80->F & SET_C);
    uint8_t memory_val = z80->readMemory(z80, z80->HL);
    CP(z80, &z80->A, memory_val);
    z80->HL -= 1;
    z80->BC -= 1;

    CHANGE_FLAG(P, z80->BC != 0);
    CHANGE_FLAG(C, carry);
    SET_FLAG(N);

    if(z80->BC != 0 && !(bool)(z80->F & SET_Z)){
        z80->PC -= 1;
        z80->WZ = z80->PC;
        z80->PC -= 1;
        CHANGE_FLAG(X, z80->PC & (1 << 11));
        CHANGE_FLAG(Y, z80->PC & (1 << 13));
        z80->cycles += 21;
    } else {
        bool aux_carry = (bool)(z80->F & SET_H);
        uint8_t val = z80->A - memory_val - aux_carry;
        CHANGE_FLAG(Y, val & 0b10);
        CHANGE_FLAG(X, val & 0b1000);
        z80->cycles += 16;
    }
}

static void INI(z80_t* z80){
    uint8_t val = z80->readIO(z80, z80->BC);
    z80->writeMemory(z80, z80->HL, val);
    z80->WZ = z80->BC + 1;
    z80->HL += 1;  
    z80->B -= 1;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    CHANGE_FLAG(Y, z80->B & 0b100000);
    CHANGE_FLAG(X, z80->B & 0b1000);

    uint16_t flag_calc = (val + ((z80->C + 1) & 255)); 
    bool hcf = (flag_calc > 255);
    CHANGE_FLAG(H, hcf);
    CHANGE_FLAG(C, hcf);
    setParity(z80, (flag_calc & 7) ^ z80->B);
    CHANGE_FLAG(N, val & 0x80);

    z80->cycles += 16;
}

static void IND(z80_t* z80){
    uint8_t val = z80->readIO(z80, z80->BC);
    z80->writeMemory(z80, z80->HL, val);
    z80->WZ = z80->BC - 1;
    z80->B -= 1;
    z80->HL -= 1;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    CHANGE_FLAG(Y, z80->B & 0b100000);
    CHANGE_FLAG(X, z80->B & 0b1000);

    uint16_t flag_calc = (val + ((z80->C - 1) & 255)); 
    bool hcf = (flag_calc > 255);
    CHANGE_FLAG(H, hcf);
    CHANGE_FLAG(C, hcf);
    setParity(z80, (flag_calc & 7) ^ z80->B);
    CHANGE_FLAG(N, val & 0x80);

    z80->cycles += 16;
}

static void INIR(z80_t* z80){
    uint8_t val = z80->readIO(z80, z80->BC);
    z80->writeMemory(z80, z80->HL, val);
    z80->HL += 1;  
    z80->B -= 1;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    uint16_t flag_calc = (val + ((z80->C + 1) & 255)); 
    bool hcf = (flag_calc > 255);
    CHANGE_FLAG(H, hcf);
    CHANGE_FLAG(C, hcf);
    setParity(z80, (flag_calc & 7) ^ z80->B);
    CHANGE_FLAG(N, val & 0x80);

    if(z80->B != 0){
        z80->PC -= 1;
        z80->WZ = z80->PC;
        z80->PC -= 1;
        CHANGE_FLAG(X, z80->PC & (1 << 11));
        CHANGE_FLAG(Y, z80->PC & (1 << 13));
        bool old_parity = z80->F & SET_P;
        bool parity;
        if(z80->F & SET_C) {
            if (val & 0x80) {
                setParity(z80, (z80->B - 1) & 7);
                parity = z80->F & SET_P;
                CHANGE_FLAG(P, old_parity ^ parity ^ 1);
                CHANGE_FLAG(H, !(z80->B & 0x0F));
            } else {
                setParity(z80, (z80->B + 1) & 7);
                parity = z80->F & SET_P;
                CHANGE_FLAG(P, old_parity ^ parity ^ 1);
                CHANGE_FLAG(H, (z80->B & 0x0F) == 0x0F);
            }
        } else {
            setParity(z80, z80->B & 0x7);
            parity = z80->F & SET_P;
            CHANGE_FLAG(P, old_parity ^ parity ^ 1);
        }
        z80->cycles += 21;
    } else {
        z80->WZ = z80->BC + 0x0101;
        CHANGE_FLAG(Y, z80->B & 0b100000);
        CHANGE_FLAG(X, z80->B & 0b1000);
        z80->cycles += 16;
    }
}

static void INDR(z80_t* z80){
    uint8_t val = z80->readIO(z80, z80->BC);
    z80->writeMemory(z80, z80->HL, val);
    
    z80->B -= 1;
    z80->HL -= 1;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    uint16_t flag_calc = (val + ((z80->C - 1) & 255)); 
    bool hcf = (flag_calc > 255);
    CHANGE_FLAG(H, hcf);
    CHANGE_FLAG(C, hcf);
    setParity(z80, (flag_calc & 7) ^ z80->B);
    CHANGE_FLAG(N, val & 0x80);

    if(z80->B != 0){
        z80->PC -= 1;
        z80->WZ = z80->PC;
        z80->PC -= 1;
        CHANGE_FLAG(X, z80->PC & (1 << 11));
        CHANGE_FLAG(Y, z80->PC & (1 << 13));
        bool old_parity = z80->F & SET_P;
        bool parity;
        if(z80->F & SET_C) {
            if (val & 0x80) {
                setParity(z80, (z80->B - 1) & 7);
                parity = z80->F & SET_P;
                CHANGE_FLAG(P, old_parity ^ parity ^ 1);
                CHANGE_FLAG(H, !(z80->B & 0x0F));
            } else {
                setParity(z80, (z80->B + 1) & 7);
                parity = z80->F & SET_P;
                CHANGE_FLAG(P, old_parity ^ parity ^ 1);
                CHANGE_FLAG(H, (z80->B & 0x0F) == 0x0F);
            }
        } else {
            setParity(z80, z80->B & 0x7);
            parity = z80->F & SET_P;
            CHANGE_FLAG(P, old_parity ^ parity ^ 1);
        }
        z80->cycles += 21;
    } else {
        z80->WZ = z80->BC + 0x0100 - 1;
        CHANGE_FLAG(Y, z80->B & 0b100000);
        CHANGE_FLAG(X, z80->B & 0b1000);
        z80->cycles += 16;
    }
}

static void OUTI(z80_t* z80){
    z80->B -= 1;
    z80->WZ = z80->BC + 1;
    uint8_t val = z80->readMemory(z80, z80->HL);
    z80->writeIO(z80, z80->BC, val);
    z80->HL += 1;
    z80->cycles += 16;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    CHANGE_FLAG(Y, z80->B & 0b100000);
    CHANGE_FLAG(X, z80->B & 0b1000);

    uint16_t flag_calc = (val + (z80->L & 255)); 
    bool hcf = (flag_calc > 255);
    CHANGE_FLAG(H, hcf);
    CHANGE_FLAG(C, hcf);
    setParity(z80, (flag_calc & 7) ^ z80->B);
    CHANGE_FLAG(N, val & 0x80);
}

static void OUTD(z80_t* z80){
    z80->B -= 1;
    z80->WZ = z80->BC - 1;
    uint8_t val = z80->readMemory(z80, z80->HL);
    z80->writeIO(z80, z80->BC, val);
    z80->HL -= 1;
    z80->cycles += 16;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    CHANGE_FLAG(Y, z80->B & 0b100000);
    CHANGE_FLAG(X, z80->B & 0b1000);

    uint16_t flag_calc = (val + (z80->L & 255)); 
    bool hcf = (flag_calc > 255);
    CHANGE_FLAG(H, hcf);
    CHANGE_FLAG(C, hcf);
    setParity(z80, (flag_calc & 7) ^ z80->B);
    CHANGE_FLAG(N, val & 0x80);
}

static void OTIR(z80_t* z80){
    z80->B -= 1;
    uint8_t val = z80->readMemory(z80, z80->HL);
    z80->writeIO(z80, z80->BC, val);
    z80->HL += 1;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    uint16_t flag_calc = (val + (z80->L & 255)); 
    bool hcf = (flag_calc > 255);
    CHANGE_FLAG(H, hcf);
    CHANGE_FLAG(C, hcf);
    setParity(z80, (flag_calc & 7) ^ z80->B);
    CHANGE_FLAG(N, val & 0x80);

    if(z80->B != 0){
        z80->PC -= 1;
        z80->WZ = z80->PC;
        z80->PC -= 1;
        CHANGE_FLAG(X, z80->PC & (1 << 11));
        CHANGE_FLAG(Y, z80->PC & (1 << 13));
        bool old_parity = z80->F & SET_P;
        bool parity;
        if(z80->F & SET_C) {
            if (val & 0x80) {
                setParity(z80, (z80->B - 1) & 7);
                parity = z80->F & SET_P;
                CHANGE_FLAG(P, old_parity ^ parity ^ 1);
                CHANGE_FLAG(H, !(z80->B & 0x0F));
            } else {
                setParity(z80, (z80->B + 1) & 7);
                parity = z80->F & SET_P;
                CHANGE_FLAG(P, old_parity ^ parity ^ 1);
                CHANGE_FLAG(H, (z80->B & 0x0F) == 0x0F);
            }
        } else {
            setParity(z80, z80->B & 0x7);
            parity = z80->F & SET_P;
            CHANGE_FLAG(P, old_parity ^ parity ^ 1);
        }
        z80->cycles += 21;
    } else {
        z80->WZ = z80->BC + 1;
        CHANGE_FLAG(Y, z80->B & 0b100000);
        CHANGE_FLAG(X, z80->B & 0b1000);
        z80->cycles += 16;
    }
}

static void OTDR(z80_t* z80){
    z80->B -= 1;
    uint8_t val = z80->readMemory(z80, z80->HL);
    z80->writeIO(z80, z80->BC, val);
    z80->HL -= 1;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    uint16_t flag_calc = (val + (z80->L & 255)); 
    bool hcf = (flag_calc > 255);
    CHANGE_FLAG(H, hcf);
    CHANGE_FLAG(C, hcf);
    setParity(z80, (flag_calc & 7) ^ z80->B);
    CHANGE_FLAG(N, val & 0x80);

    if(z80->B != 0){
        z80->PC -= 1;
        z80->WZ = z80->PC;
        z80->PC -= 1;
        CHANGE_FLAG(X, z80->PC & (1 << 11));
        CHANGE_FLAG(Y, z80->PC & (1 << 13));
        bool old_parity = z80->F & SET_P;
        bool parity;
        if(z80->F & SET_C) {
            if (val & 0x80) {
                setParity(z80, (z80->B - 1) & 7);
                parity = z80->F & SET_P;
                CHANGE_FLAG(P, old_parity ^ parity ^ 1);
                CHANGE_FLAG(H, !(z80->B & 0x0F));
            } else {
                setParity(z80, (z80->B + 1) & 7);
                parity = z80->F & SET_P;
                CHANGE_FLAG(P, old_parity ^ parity ^ 1);
                CHANGE_FLAG(H, (z80->B & 0x0F) == 0x0F);
            }
        } else {
            setParity(z80, z80->B & 0x7);
            parity = z80->F & SET_P;
            CHANGE_FLAG(P, old_parity ^ parity ^ 1);
        }
        z80->cycles += 21;
    } else {
        CHANGE_FLAG(Y, z80->B & 0b100000);
        CHANGE_FLAG(X, z80->B & 0b1000);    
        z80->cycles += 16;
    }
}

static void incrementR(uint8_t* R){
    *R = ((*R + 1) & 0x7F) | (*R & 0x80);
}

static void setParity(z80_t* z80, uint8_t val){
    int counter = 0;
    while(val != 0){
        counter += (val & 0x1);
        val = val >> 1;
    }
    bool parity = !(counter & 1);
    CHANGE_FLAG(P, parity);
}

static void setZero(z80_t* z80, uint16_t val){
    CHANGE_FLAG(Z, val == 0);
}

static void setSign8Bit(z80_t* z80, uint8_t val){
    CHANGE_FLAG(S, val & 0x80);
}

static void setSign16Bit(z80_t* z80, uint16_t val){
    CHANGE_FLAG(S, val & 0x8000);
}

static bool calculateCarry(int bit, uint16_t a, uint16_t b, bool cy) {
  int32_t result = a + b + cy;
  int32_t carry = result ^ a ^ b;
  return carry & (1 << bit);
}

static uint16_t readHalfWord(z80_t* z80, uint16_t addr){
    uint8_t lo = z80->readMemory(z80, addr);
    uint8_t hi = z80->readMemory(z80, addr+1);
    return (hi << 8) | lo;
}

static void writeHalfWord(z80_t* z80, uint16_t addr, uint16_t halfword){
    z80->writeMemory(z80, addr, halfword);
    z80->writeMemory(z80, addr+1, halfword >> 8);
}