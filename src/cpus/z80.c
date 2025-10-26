#include "cpus/z80.h"
#include "utils/vec.h"

DEFINE_VEC(byte_vec, u8);

#include <stdio.h>
#include <string.h>

typedef void (*rotateFunc)(z80_t*, u8*);
typedef void (*aluFunc)(z80_t*, u8*, u8);
typedef void (*blockFunc)(z80_t*);

static void useDDRegisterTable(z80_t*, u8**, u16**, u16**);
static void useFDRegisterTable(z80_t*, u8**, u16**, u16**);
static void restoreRegisterTable(z80_t*, u8**, u16**, u16**);
static void adjustDDorFFOpcode(z80_t*, u8**, u16**, u16**);

static void NOP(z80_t*);
static void EX(z80_t*, u16*, u16*);
static void DJNZ(z80_t*, int8_t);
static void JR(z80_t*, int8_t);
static void JRNZ(z80_t*, int8_t);
static void JRZ(z80_t*, int8_t);
static void JRNC(z80_t*, int8_t);
static void JRC(z80_t*, int8_t);
static void LD_8(z80_t*, u8*, u8);
static void LD_16(z80_t*, u16*, u16);
static void ADD_16(z80_t*, u16*, u16*);
static void INC_16(z80_t*, u16*);
static void DEC_16(z80_t*, u16*);
static void INC_8(z80_t*, u8*);
static void DEC_8(z80_t*, u8*);
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
static void POP(z80_t*, u16*);
static void RET(z80_t*);
static void EXX(z80_t*);
static void JP(z80_t*, u16);
static void JPNZ(z80_t*, u16);
static void JPZ(z80_t*, u16);
static void JPNC(z80_t*, u16);
static void JPC(z80_t*, u16);
static void JPPO(z80_t*, u16);
static void JPPE(z80_t*, u16);
static void JPP(z80_t*, u16);
static void JPM(z80_t*, u16);
static void EI(z80_t*);
static void DI(z80_t*);
static void CALL(z80_t*, u16);
static void CALLNZ(z80_t*, u16);
static void CALLZ(z80_t*, u16);
static void CALLNC(z80_t*, u16);
static void CALLC(z80_t*, u16);
static void CALLPO(z80_t*, u16);
static void CALLPE(z80_t*, u16);
static void CALLP(z80_t*, u16);
static void CALLM(z80_t*, u16);
static void PUSH(z80_t*, u16);
static void RST(z80_t*, u8);
static void RLC(z80_t*, u8*);
static void RRC(z80_t*, u8*);
static void RLCA(z80_t*);
static void RRCA(z80_t*);
static void RLA(z80_t*);
static void RRA(z80_t*);
static void RLC(z80_t*, u8*);
static void RRC(z80_t*, u8*);
static void RL(z80_t*, u8*);
static void RR(z80_t*, u8*);
static void SLA(z80_t*, u8*);
static void SRA(z80_t*, u8*);
static void SLL(z80_t*, u8*);
static void SRL(z80_t*, u8*);
static void BIT(z80_t*, u8, u8*);
static void SET(z80_t*, u8, u8*);
static void RES(z80_t*, u8, u8*);
static void ADD(z80_t*, u8*, u8);
static void ADC(z80_t*, u8*, u8);
static void SUB(z80_t*, u8*, u8);
static void SBC(z80_t*, u8*, u8);
static void AND(z80_t*, u8*, u8);
static void XOR(z80_t*, u8*, u8);
static void OR(z80_t*, u8*, u8);
static void CP(z80_t*, u8*, u8);
static void ADC_16(z80_t*, u16*, u16);
static void SBC_16(z80_t*, u16*, u16);
static void NEG(z80_t*, u8*);
static void RETI(z80_t*);
static void RETN(z80_t*);
static void IM(z80_t*, u8);
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

static void incrementR(u8*);
static void setParity(z80_t*, u8);
static void setZero(z80_t*, u16);
static void setSign8Bit(z80_t*, u8);
static void setSign16Bit(z80_t*, u16);
static bool calculateCarry(int, u16, u16, bool);

static u16 readHalfWord(z80_t*, u16);
static void writeHalfWord(z80_t*, u16, u16);

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

void serialize_z80_t(z80_t* z80, byte_vec_t* vec){
    byte_vec_push(vec, z80->HALTED);
    byte_vec_push(vec, z80->IFF1);
    byte_vec_push(vec, z80->IFF2);
    byte_vec_push(vec, z80->INTERRUPT_DELAY);
    byte_vec_push(vec, z80->INTERRUPT_PENDING);
    byte_vec_push(vec, z80->INTERRUPT_MODE);
    byte_vec_push(vec, z80->INTERRUPT_VECT);

    byte_vec_push_array(vec, (u8*)&z80->AF, sizeof(z80->AF));
    byte_vec_push_array(vec, (u8*)&z80->BC, sizeof(z80->BC));
    byte_vec_push_array(vec, (u8*)&z80->DE, sizeof(z80->DE));
    byte_vec_push_array(vec, (u8*)&z80->HL, sizeof(z80->HL));
    byte_vec_push_array(vec, (u8*)&z80->IX, sizeof(z80->IX));
    byte_vec_push_array(vec, (u8*)&z80->IY, sizeof(z80->IY));
    byte_vec_push_array(vec, (u8*)&z80->WZ, sizeof(z80->WZ));
    byte_vec_push_array(vec, (u8*)&z80->AF_, sizeof(z80->AF_));
    byte_vec_push_array(vec, (u8*)&z80->BC_, sizeof(z80->BC_));
    byte_vec_push_array(vec, (u8*)&z80->DE_, sizeof(z80->DE_));
    byte_vec_push_array(vec, (u8*)&z80->HL_, sizeof(z80->HL_));

    byte_vec_push_array(vec, (u8*)&z80->SP, sizeof(z80->SP));
    byte_vec_push_array(vec, (u8*)&z80->PC, sizeof(z80->PC));

    byte_vec_push(vec, z80->I);
    byte_vec_push(vec, z80->R);
    byte_vec_push(vec, z80->Q);

    byte_vec_push_array(vec, (u8*)&z80->cycles, sizeof(z80->cycles));
}

u8* deserialize_z80_t(z80_t* z80, u8* data){
    z80->HALTED = *(data++);
    z80->IFF1 = *(data++);
    z80->IFF2 = *(data++);
    z80->INTERRUPT_DELAY = *(data++);
    z80->INTERRUPT_PENDING = *(data++);
    z80->INTERRUPT_MODE = *(data++);
    z80->INTERRUPT_VECT = *(data++);

    memcpy(&z80->AF, data, sizeof(z80->AF)); data += sizeof(z80->AF);
    memcpy(&z80->BC, data, sizeof(z80->BC)); data += sizeof(z80->BC);
    memcpy(&z80->DE, data, sizeof(z80->DE)); data += sizeof(z80->DE);
    memcpy(&z80->HL, data, sizeof(z80->HL)); data += sizeof(z80->HL);
    memcpy(&z80->IX, data, sizeof(z80->IX)); data += sizeof(z80->IX);
    memcpy(&z80->IY, data, sizeof(z80->IY)); data += sizeof(z80->IY);
    memcpy(&z80->WZ, data, sizeof(z80->WZ)); data += sizeof(z80->WZ);
    memcpy(&z80->AF_, data, sizeof(z80->AF_)); data += sizeof(z80->AF_);
    memcpy(&z80->BC_, data, sizeof(z80->BC_)); data += sizeof(z80->BC_);
    memcpy(&z80->DE_, data, sizeof(z80->DE_)); data += sizeof(z80->DE_);
    memcpy(&z80->HL_, data, sizeof(z80->HL_)); data += sizeof(z80->HL_);

    memcpy(&z80->SP, data, sizeof(z80->SP)); data += sizeof(z80->SP);
    memcpy(&z80->PC, data, sizeof(z80->PC)); data += sizeof(z80->PC);

    z80->I = *(data++);
    z80->R = *(data++);
    z80->Q = *(data++);

    memcpy(&z80->cycles, data, sizeof(z80->cycles)); data += sizeof(z80->cycles);
    return data;
}

static void restoreRegisterTable(z80_t* z80, u8** r, u16** rp, u16** rp2){
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

static void useDDRegisterTable(z80_t* z80, u8** r, u16** rp, u16** rp2){
    r[4] = &z80->IXH;
    r[5] = &z80->IXL;
    rp[2] = &z80->IX;
    rp2[2] = &z80->IX;
}

static void useFDRegisterTable(z80_t* z80, u8** r, u16** rp, u16** rp2){
    r[4] = &z80->IYH;
    r[5] = &z80->IYL;
    rp[2] = &z80->IY;
    rp2[2] = &z80->IY;
}

static void adjustDDorFFOpcode(z80_t* z80, u8** r, u16** rp, u16** rp2){
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
    fprintf(stderr, "OPCODE: 0x%02X %02X %02X\n", z80->readMemory(z80->ctx, z80->PC), z80->readMemory(z80->ctx, z80->PC+1), z80->readMemory(z80->ctx, z80->PC+2));
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
            u16 interruptAddress = readHalfWord(z80, (z80->I << 8) | z80->INTERRUPT_VECT);
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
    const static u8    im[8]   = {   0,   0,    1,    2,    0,    0,    1,    2  };

    u8* r[8];
    u16* rp[4];
    u16* rp2[4];

    u8 opcode = z80->readMemory(z80->ctx, z80->PC);
    u8 x;
    u8 y;
    u8 z;
    u8 q;
    u8 p;

    u8  val8;
    u16 val16;
    u16 nn;
    u16 old_PC;
    u8 old_F = z80->F;
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

    opcode = z80->readMemory(z80->ctx, z80->PC);

    switch(opcode){
        case 0xCB:
        if(prefixDD || prefixFD)
            z80->PC += 1;
        else
            incrementR(&z80->R);

        z80->PC += 1;
        opcode = z80->readMemory(z80->ctx, z80->PC);
        x = opcode >> 6;
        y = (opcode >> 3) & 0b111;
        z = opcode & 0b111;
        q = y & 0b1;
        p = (y >> 1) & 0b11;

        z80->PC += 1;
        if(prefixDD || prefixFD){
            nn = *rp[2] + (int8_t)z80->readMemory(z80->ctx, z80->PC - 2);
            z80->WZ = nn;
            z80->aux_reg = z80->readMemory(z80->ctx, nn);
        } else {
            nn = z80->HL;
            if(z == 6)
                z80->aux_reg = z80->readMemory(z80->ctx, nn);
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
                z80->writeMemory(z80->ctx, nn, *r[z]);
                z80->cycles += 19;
            }
        } else if(z == 6 && !prefixDD && !prefixDD){
            if(x == 1)
                z80->cycles += 12;
            else {
                z80->writeMemory(z80->ctx, nn, z80->aux_reg);
                z80->cycles += 15;
            }
        } else
            z80->cycles += 8;
        break;

        case 0xED:
        z80->PC = z80->PC + 1;
        incrementR(&z80->R);
        opcode = z80->readMemory(z80->ctx, z80->PC);
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
                    *r[y] = z80->readIO(z80->ctx, z80->BC);
                    setSign8Bit(z80, *r[y]);
                    setParity(z80, *r[y]);
                    setZero(z80, *r[y]);
                    CHANGE_FLAG(X, *r[y] & (1 << 3));
                    CHANGE_FLAG(Y, *r[y] & (1 << 5));
                    CLEAR_FLAG(H);
                    CLEAR_FLAG(N);
                } else {
                    u8 copy = z80->A;
                    z80->A = z80->readIO(z80->ctx, z80->BC);
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
                    z80->writeIO(z80->ctx, z80->BC, *r[y]);
                } else {
                    z80->writeIO(z80->ctx, z80->BC, 0);
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
                    DJNZ(z80, z80->readMemory(z80->ctx, z80->PC-1));
                    break;

                    case 3:
                    z80->PC += 2;
                    JR(z80, z80->readMemory(z80->ctx, z80->PC-1));
                    z80->cycles += 12;
                    break;

                    case 4:
                    z80->PC += 2;
                    JRNZ(z80, z80->readMemory(z80->ctx, z80->PC-1));
                    break;

                    case 5:
                    z80->PC += 2;
                    JRZ(z80, z80->readMemory(z80->ctx, z80->PC-1));
                    break;

                    case 6:
                    z80->PC += 2;
                    JRNC(z80, z80->readMemory(z80->ctx, z80->PC-1));
                    break;

                    case 7:
                    z80->PC += 2;
                    JRC(z80, z80->readMemory(z80->ctx, z80->PC-1));
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
                        z80->writeMemory(z80->ctx, z80->BC, z80->A);
                        z80->W = z80->A;
                        z80->Z = z80->C + 1;
                        z80->cycles += 7;
                        break;

                        case 1:
                        z80->PC += 1;
                        z80->writeMemory(z80->ctx, z80->DE, z80->A);
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
                        z80->writeMemory(z80->ctx, nn, z80->A);
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
                        z80->A = z80->readMemory(z80->ctx, z80->BC);
                        z80->WZ = z80->BC + 1;
                        z80->cycles += 7;
                        break;

                        case 1:
                        z80->PC += 1;
                        z80->A = z80->readMemory(z80->ctx, z80->DE);
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
                        val8 = z80->readMemory(z80->ctx, nn);
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
                    nn = *rp[2] + (int8_t)z80->readMemory(z80->ctx, z80->PC);
                    z80->WZ = nn;
                    z80->PC += 1;
                    z80->cycles += 19;
                } else if(y == 6 && !prefixDD && !prefixFD) {
                    nn = z80->HL;
                    z80->cycles += 11;
                } else 
                    z80->cycles += 4;
                if(y == 6)
                    z80->aux_reg = z80->readMemory(z80->ctx, nn);
                INC_8(z80, r[y]);
                if(y == 6)
                    z80->writeMemory(z80->ctx, nn, z80->aux_reg);
                break;

                case 5:
                z80->PC += 1;
                if(y == 6 && (prefixDD || prefixFD)){
                    nn = *rp[2] + (int8_t)z80->readMemory(z80->ctx, z80->PC);
                    z80->WZ = nn;
                    z80->PC += 1;
                    z80->cycles += 19;
                } else if(y == 6 && !prefixDD && !prefixFD){
                    nn = z80->HL;
                    z80->cycles += 11;
                } else 
                    z80->cycles += 4;
                if(y == 6)
                    z80->aux_reg = z80->readMemory(z80->ctx, nn);
                DEC_8(z80, r[y]);
                if(y == 6)
                    z80->writeMemory(z80->ctx, nn, z80->aux_reg);
                break;

                case 6:
                if(y == 6 && (prefixDD || prefixFD)){
                    nn = *rp[2] + (int8_t)z80->readMemory(z80->ctx, z80->PC+1);
                    z80->WZ = nn;
                    adjustDDorFFOpcode(z80, r, rp, rp2);
                    z80->cycles += 15;
                } else if(y == 6 && !prefixDD && !prefixFD) {
                    nn = z80->HL;
                    z80->cycles += 10;
                } else
                    z80->cycles += 7;
                val8 = z80->readMemory(z80->ctx, z80->PC+1);
                z80->PC += 2;
                *r[y] = val8;
                if(y == 6)
                    z80->writeMemory(z80->ctx, nn, z80->aux_reg);
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
                    nn = *rp[2] + (int8_t)z80->readMemory(z80->ctx, z80->PC);
                    z80->WZ = nn;
                    adjustDDorFFOpcode(z80, r, rp, rp2);
                    z80->cycles += 15;
                } else if((z == 6 || y == 6) && !prefixDD && !prefixFD){
                    nn = z80->HL;
                    z80->cycles += 7;
                } else
                    z80->cycles += 4;
                if(z == 6)
                    z80->aux_reg = z80->readMemory(z80->ctx, nn);
                *r[y] = *r[z];
                if(y == 6)
                    z80->writeMemory(z80->ctx, nn, z80->aux_reg);
            }
            break;

            case 2:
            if((prefixDD || prefixFD) && z == 6){
                z80->PC += 1;
                nn = *rp[2] + (int8_t)z80->readMemory(z80->ctx, z80->PC);
                z80->WZ = nn;
                z80->cycles += 15;
            } else if(!prefixDD && !prefixFD && z == 6){
                nn = z80->HL;
                z80->cycles += 7;
            } else
                z80->cycles += 4;
            if(z == 6)
                z80->aux_reg = z80->readMemory(z80->ctx, nn);
            function = alu[y];
            z80->PC += 1;
            (*function)(z80, &z80->A, *r[z]);
            if(z == 6)
                z80->writeMemory(z80->ctx, nn, z80->aux_reg);
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
                    val8 = z80->readMemory(z80->ctx, z80->PC+1);
                    z80->PC += 2;
                    z80->writeIO(z80->ctx, val8, z80->A);
                    z80->W = z80->A;
                    z80->Z = val8 + 1;
                    z80->cycles += 11;
                    break;
                    
                    case 3:
                    val16 = (z80->A << 8) | z80->readMemory(z80->ctx, z80->PC+1);
                    z80->WZ = val16 + 1;
                    z80->PC += 2;
                    z80->A = z80->readIO(z80->ctx, val16);
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
                val8 = z80->readMemory(z80->ctx, z80->PC+1);
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

static void EX(z80_t* z80, u16* regA, u16* regB){
    u16 tmp = *regA;
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

static void ADD_16(z80_t* z80, u16* regDst, u16* regSrc){
    CLEAR_FLAG(N);
    bool carry = calculateCarry(16, *regDst, *regSrc, 0);
    CHANGE_FLAG(C, carry);

    bool aux_carry = calculateCarry(12, *regDst, *regSrc, 0);
    CHANGE_FLAG(H, aux_carry);

    *regDst += *regSrc;

    CHANGE_FLAG(Y, (*regDst >> 8) & 0b100000);
    CHANGE_FLAG(X, (*regDst >> 8) & 0b1000);
}

static void INC_16(z80_t* z80, u16* reg){
    *reg += 1;
}

static void DEC_16(z80_t* z80, u16* reg){
    *reg -= 1;
}

static void INC_8(z80_t* z80, u8* reg){
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

static void DEC_8(z80_t* z80, u8* reg){
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
    u8 correction = 0;
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

static void POP(z80_t* z80, u16* reg){
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

static void JP(z80_t* z80, u16 val){
    z80->PC = val;
    z80->WZ = val;
}

static void JPNZ(z80_t* z80, u16 val){
    if(!(z80->F & SET_Z))
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPZ(z80_t* z80, u16 val){
    if(z80->F & SET_Z)
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPNC(z80_t* z80, u16 val){
    if(!(z80->F & SET_C))
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPC(z80_t* z80, u16 val){
    if(z80->F & SET_C)
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPPO(z80_t* z80, u16 val){
    if(!(z80->F & SET_P))
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPPE(z80_t* z80, u16 val){
    if(z80->F & SET_P)
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPP(z80_t* z80, u16 val){
    if(!(z80->F & SET_S))
        JP(z80, val);
    else
        z80->WZ = val;
}

static void JPM(z80_t* z80, u16 val){
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

static void CALL(z80_t* z80, u16 val){
    PUSH(z80, z80->PC);
    z80->PC = val;
    z80->WZ = val;
}

static void CALLNZ(z80_t* z80, u16 val){
    if(!(z80->F & SET_Z))
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLZ(z80_t* z80, u16 val){
    if(z80->F & SET_Z)
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLNC(z80_t* z80, u16 val){
    if(!(z80->F & SET_C))
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLC(z80_t* z80, u16 val){
    if(z80->F & SET_C)
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLPO(z80_t* z80, u16 val){
    if(!(z80->F & SET_P))
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLPE(z80_t* z80, u16 val){
    if(z80->F & SET_P)
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLP(z80_t* z80, u16 val){
    if(!(z80->F & SET_S))
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void CALLM(z80_t* z80, u16 val){
    if(z80->F & SET_S)
        CALL(z80, val);
    else
        z80->WZ = val;
}

static void PUSH(z80_t* z80, u16 val){
    z80->SP = z80->SP - 2;
    writeHalfWord(z80, z80->SP, val);
}

static void RST(z80_t* z80, u8 addr){
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

static void RLC(z80_t* z80, u8* reg){
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

static void RRC(z80_t* z80, u8* reg){
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

static void RL(z80_t* z80, u8* reg){
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

static void RR(z80_t* z80, u8* reg){
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

static void SLA(z80_t* z80, u8* reg){
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

static void SRA(z80_t* z80, u8* reg){
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

static void SLL(z80_t* z80, u8* reg){
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


static void SRL(z80_t* z80, u8* reg){
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

static void BIT(z80_t* z80, u8 bit, u8* reg){
    u8 masked_bit = *reg & (1 << bit);
    CHANGE_FLAG(Z, masked_bit == 0);
    CHANGE_FLAG(P, masked_bit == 0);
    CHANGE_FLAG(S, masked_bit & 0x80);
    SET_FLAG(H);
    CLEAR_FLAG(N);
    CHANGE_FLAG(Y, *reg & 0b100000);
    CHANGE_FLAG(X, *reg & 0b1000);
}

static void RES(z80_t* z80, u8 bit, u8* reg){
    *reg = *reg & (~(u8)(1 << bit));
}

static void SET(z80_t* z80, u8 bit, u8* reg){
    *reg = * reg | (1 << bit);
}

static void ADD(z80_t* z80, u8* reg, u8 val){
    u8 res = *reg + val;
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

static void ADC(z80_t* z80, u8* reg, u8 val){
    bool carry = z80->F & SET_C;
    u8 res = *reg + val + carry;

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

static void SUB(z80_t* z80, u8* reg, u8 val){
    val = ~val + 1;
    bool carry = calculateCarry(8, *reg, val - 1, 1);
    CHANGE_FLAG(C, !carry);

    bool aux_carry = calculateCarry(4, *reg, val - 1, 1);
    CHANGE_FLAG(H, !aux_carry);

    u8 res = *reg + val;

    bool overflow = calculateCarry(7, *reg, val - 1, 1) != calculateCarry(8, *reg, val - 1, 1);
    CHANGE_FLAG(P, overflow);

    *reg = res;
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    SET_FLAG(N);

    CHANGE_FLAG(Y, res & 0b100000);
    CHANGE_FLAG(X, res & 0b1000);
}

static void SBC(z80_t* z80, u8* reg, u8 val){
    val = ~val + 1;
    bool carry = z80->F & SET_C;
    bool new_carry = calculateCarry(8, *reg, val - 1, !carry);
    bool aux_carry = calculateCarry(4, *reg, val - 1, !carry);

    CHANGE_FLAG(C, !new_carry);
    CHANGE_FLAG(H, !aux_carry);

    u8 res = *reg + val - carry;
    bool overflow = calculateCarry(7, *reg, val - 1, !carry) != calculateCarry(8, *reg, val - 1, !carry);
    CHANGE_FLAG(P, overflow); 

    *reg = res;
    setSign8Bit(z80, *reg);
    setZero(z80, *reg);
    SET_FLAG(N);

    CHANGE_FLAG(Y, *reg & 0b100000);
    CHANGE_FLAG(X, *reg & 0b1000);
}

static void AND(z80_t* z80, u8* reg, u8 val){
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

static void XOR(z80_t* z80, u8* reg, u8 val){
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

static void OR(z80_t* z80, u8* reg, u8 val){
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

static void CP(z80_t* z80, u8* reg, u8 val){
    u8 copy = *reg;
    SUB(z80, reg, val);
    *reg = copy;

    CHANGE_FLAG(Y, val & 0b100000);
    CHANGE_FLAG(X, val & 0b1000);
}

static void ADC_16(z80_t* z80, u16* reg, u16 val){
    bool carry = z80->F & SET_C;

    bool new_carry = calculateCarry(16, *reg, val, carry);
    CHANGE_FLAG(C, new_carry);
    
    bool aux_carry = calculateCarry(12, *reg, val, carry);
    CHANGE_FLAG(H, aux_carry);

    u16 res = *reg + val + carry;
    bool overflow = calculateCarry(16, *reg, val, carry) != calculateCarry(15, *reg, val, carry);
    CHANGE_FLAG(P, overflow);
           
    *reg = res;
    setSign16Bit(z80, *reg);
    setZero(z80, *reg);
    CLEAR_FLAG(N);

    CHANGE_FLAG(Y, (res >> 8) & 0b100000);
    CHANGE_FLAG(X, (res >> 8) & 0b1000);
}

static void SBC_16(z80_t* z80, u16* reg, u16 val){
    val = ~val + 1;
    bool carry = z80->F & SET_C;
    bool new_carry = calculateCarry(16, *reg, val - 1, !carry);
    bool aux_carry = calculateCarry(12, *reg, val - 1, !carry);

    CHANGE_FLAG(C, !new_carry);
    CHANGE_FLAG(H, !aux_carry);

    u16 res = *reg + val - carry;
    bool overflow = calculateCarry(16, *reg, val - 1, !carry) != calculateCarry(15, *reg, val - 1, !carry);
    CHANGE_FLAG(P, overflow);  

    *reg = res;
    setSign16Bit(z80, *reg);
    setZero(z80, *reg);
    SET_FLAG(N);

    CHANGE_FLAG(Y, (res >> 8) & 0b100000);
    CHANGE_FLAG(X, (res >> 8) & 0b1000);
}

static void NEG(z80_t* z80, u8* reg){
    u8 tmp = *reg;
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

static void IM(z80_t* z80, u8 im_mode){
    z80->INTERRUPT_MODE = im_mode;
}

static void RRD(z80_t* z80){
    u8 tmpA = z80->A;
    u8 val = z80->readMemory(z80->ctx, z80->HL);
    z80->A = (tmpA & 0xF0) | (val & 0xF);
    z80->writeMemory(z80->ctx,  z80->HL, (val >> 4) | (tmpA << 4) );
    setSign8Bit(z80, z80->A);
    setZero(z80, z80->A);
    setParity(z80, z80->A);
    CLEAR_FLAG(N);
    CLEAR_FLAG(H);
    CHANGE_FLAG(X, z80->A & 0b1000);
    CHANGE_FLAG(Y, z80->A & 0b100000);
}

static void RLD(z80_t* z80){
    u8 tmpA = z80->A;
    u8 val = z80->readMemory(z80->ctx, z80->HL);
    z80->A = (tmpA & 0xF0) | (val >> 4);
    z80->writeMemory(z80->ctx, z80->HL, (val << 4) | (tmpA & 0xF));
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
    u8 hl_val = z80->readMemory(z80->ctx, z80->HL);
    z80->writeMemory(z80->ctx, z80->DE, hl_val);
    z80->DE += 1;
    z80->HL += 1;
    z80->BC -= 1;

    CHANGE_FLAG(P, z80->BC != 0);
    CLEAR_FLAG(H);
    CLEAR_FLAG(N);

    z80->cycles += 16;

    u8 res = z80->A + hl_val;
    CHANGE_FLAG(X, res & (1 << 3));
    CHANGE_FLAG(Y, res & (1 << 1));
}

static void LDD(z80_t* z80){
    LDI(z80);
    z80->HL -= 2;
    z80->DE -= 2;
}

static void LDIR(z80_t* z80){
    u8 hl_val = z80->readMemory(z80->ctx, z80->HL);
    z80->writeMemory(z80->ctx, z80->DE, hl_val);
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
        u8 res = z80->A + hl_val;
        CHANGE_FLAG(X, res & (1 << 3));
        CHANGE_FLAG(Y, res & (1 << 1));
        z80->cycles += 16;
    }
}

static void LDDR(z80_t* z80){
    u8 hl_val = z80->readMemory(z80->ctx, z80->HL);
    z80->writeMemory(z80->ctx, z80->DE, hl_val);
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
        u8 res = z80->A + hl_val;
        CHANGE_FLAG(X, res & (1 << 3));
        CHANGE_FLAG(Y, res & (1 << 1));
        z80->cycles += 16;
    }
}

static void CPI(z80_t* z80){
    bool carry = (bool)(z80->F & SET_C);
    u8 memory_val = z80->readMemory(z80->ctx, z80->HL);
    CP(z80, &z80->A, memory_val);
    z80->HL += 1;
    z80->BC -= 1;

    CHANGE_FLAG(P, z80->BC != 0);
    CHANGE_FLAG(C, carry);
    SET_FLAG(N);

    bool aux_carry = (bool)(z80->F & SET_H);
    u8 val = z80->A - memory_val - aux_carry;
    
    CHANGE_FLAG(Y, val & 0b10);
    CHANGE_FLAG(X, val & 0b1000);

    z80->WZ += 1;
    z80->cycles += 16;
}

static void CPD(z80_t* z80){
    bool carry = (bool)(z80->F & SET_C);
    u8 memory_val = z80->readMemory(z80->ctx, z80->HL);
    CP(z80, &z80->A, memory_val);
    z80->HL -= 1;
    z80->BC -= 1;

    CHANGE_FLAG(P, z80->BC != 0);
    CHANGE_FLAG(C, carry);
    SET_FLAG(N);

    bool aux_carry = (bool)(z80->F & SET_H);
    u8 val = z80->A - memory_val - aux_carry;
    
    CHANGE_FLAG(Y, val & 0b10);
    CHANGE_FLAG(X, val & 0b1000);

    z80->WZ -= 1;
    z80->cycles += 16;
}

static void CPIR(z80_t* z80){
    bool carry = (bool)(z80->F & SET_C);
    u8 memory_val = z80->readMemory(z80->ctx, z80->HL);
    CP(z80, &z80->A, memory_val);
    z80->HL += 1;
    z80->BC -= 1;

    CHANGE_FLAG(P, z80->BC != 0);
    CHANGE_FLAG(C, carry);
    SET_FLAG(N);

    bool aux_carry = (bool)(z80->F & SET_H);
    u8 val = z80->A - memory_val - aux_carry;
    
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
    u8 memory_val = z80->readMemory(z80->ctx, z80->HL);
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
        u8 val = z80->A - memory_val - aux_carry;
        CHANGE_FLAG(Y, val & 0b10);
        CHANGE_FLAG(X, val & 0b1000);
        z80->cycles += 16;
    }
}

static void INI(z80_t* z80){
    u8 val = z80->readIO(z80->ctx, z80->BC);
    z80->writeMemory(z80->ctx, z80->HL, val);
    z80->WZ = z80->BC + 1;
    z80->HL += 1;  
    z80->B -= 1;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    CHANGE_FLAG(Y, z80->B & 0b100000);
    CHANGE_FLAG(X, z80->B & 0b1000);

    u16 flag_calc = (val + ((z80->C + 1) & 255)); 
    bool hcf = (flag_calc > 255);
    CHANGE_FLAG(H, hcf);
    CHANGE_FLAG(C, hcf);
    setParity(z80, (flag_calc & 7) ^ z80->B);
    CHANGE_FLAG(N, val & 0x80);

    z80->cycles += 16;
}

static void IND(z80_t* z80){
    u8 val = z80->readIO(z80->ctx, z80->BC);
    z80->writeMemory(z80->ctx, z80->HL, val);
    z80->WZ = z80->BC - 1;
    z80->B -= 1;
    z80->HL -= 1;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    CHANGE_FLAG(Y, z80->B & 0b100000);
    CHANGE_FLAG(X, z80->B & 0b1000);

    u16 flag_calc = (val + ((z80->C - 1) & 255)); 
    bool hcf = (flag_calc > 255);
    CHANGE_FLAG(H, hcf);
    CHANGE_FLAG(C, hcf);
    setParity(z80, (flag_calc & 7) ^ z80->B);
    CHANGE_FLAG(N, val & 0x80);

    z80->cycles += 16;
}

static void INIR(z80_t* z80){
    u8 val = z80->readIO(z80->ctx, z80->BC);
    z80->writeMemory(z80->ctx, z80->HL, val);
    z80->HL += 1;  
    z80->B -= 1;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    u16 flag_calc = (val + ((z80->C + 1) & 255)); 
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
    u8 val = z80->readIO(z80->ctx, z80->BC);
    z80->writeMemory(z80->ctx, z80->HL, val);
    
    z80->B -= 1;
    z80->HL -= 1;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    u16 flag_calc = (val + ((z80->C - 1) & 255)); 
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
    u8 val = z80->readMemory(z80->ctx, z80->HL);
    z80->writeIO(z80->ctx, z80->BC, val);
    z80->HL += 1;
    z80->cycles += 16;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    CHANGE_FLAG(Y, z80->B & 0b100000);
    CHANGE_FLAG(X, z80->B & 0b1000);

    u16 flag_calc = (val + (z80->L & 255)); 
    bool hcf = (flag_calc > 255);
    CHANGE_FLAG(H, hcf);
    CHANGE_FLAG(C, hcf);
    setParity(z80, (flag_calc & 7) ^ z80->B);
    CHANGE_FLAG(N, val & 0x80);
}

static void OUTD(z80_t* z80){
    z80->B -= 1;
    z80->WZ = z80->BC - 1;
    u8 val = z80->readMemory(z80->ctx, z80->HL);
    z80->writeIO(z80->ctx, z80->BC, val);
    z80->HL -= 1;
    z80->cycles += 16;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    CHANGE_FLAG(Y, z80->B & 0b100000);
    CHANGE_FLAG(X, z80->B & 0b1000);

    u16 flag_calc = (val + (z80->L & 255)); 
    bool hcf = (flag_calc > 255);
    CHANGE_FLAG(H, hcf);
    CHANGE_FLAG(C, hcf);
    setParity(z80, (flag_calc & 7) ^ z80->B);
    CHANGE_FLAG(N, val & 0x80);
}

static void OTIR(z80_t* z80){
    z80->B -= 1;
    u8 val = z80->readMemory(z80->ctx, z80->HL);
    z80->writeIO(z80->ctx, z80->BC, val);
    z80->HL += 1;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    u16 flag_calc = (val + (z80->L & 255)); 
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
    u8 val = z80->readMemory(z80->ctx, z80->HL);
    z80->writeIO(z80->ctx, z80->BC, val);
    z80->HL -= 1;

    setSign8Bit(z80, z80->B);
    setZero(z80, z80->B);

    u16 flag_calc = (val + (z80->L & 255)); 
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

static void incrementR(u8* R){
    *R = ((*R + 1) & 0x7F) | (*R & 0x80);
}

static void setParity(z80_t* z80, u8 val){
    int counter = 0;
    while(val != 0){
        counter += (val & 0x1);
        val = val >> 1;
    }
    bool parity = !(counter & 1);
    CHANGE_FLAG(P, parity);
}

static void setZero(z80_t* z80, u16 val){
    CHANGE_FLAG(Z, val == 0);
}

static void setSign8Bit(z80_t* z80, u8 val){
    CHANGE_FLAG(S, val & 0x80);
}

static void setSign16Bit(z80_t* z80, u16 val){
    CHANGE_FLAG(S, val & 0x8000);
}

static bool calculateCarry(int bit, u16 a, u16 b, bool cy) {
  int32_t result = a + b + cy;
  int32_t carry = result ^ a ^ b;
  return carry & (1 << bit);
}

static u16 readHalfWord(z80_t* z80, u16 addr){
    u8 lo = z80->readMemory(z80->ctx, addr);
    u8 hi = z80->readMemory(z80->ctx, addr+1);
    return (hi << 8) | lo;
}

static void writeHalfWord(z80_t* z80, u16 addr, u16 halfword){
    z80->writeMemory(z80->ctx, addr, halfword);
    z80->writeMemory(z80->ctx, addr+1, halfword >> 8);
}