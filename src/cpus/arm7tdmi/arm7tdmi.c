#include "cpus/arm7tdmi/arm7tdmi.h"
#include "cpus/arm7tdmi/arm.h"
#include "cpus/arm7tdmi/thumb.h"
#include "cpus/arm7tdmi/arm7tdmi_util.h"
#include "types.h"

#include "utils/serializer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void arm7tdmi_print(arm7tdmi_t* cpu){
    for(int i = 0; i < 16; i++){
        printf("r[%d]: %08X\n", i, cpu->r[i]);
    }

    u32 pc = cpu->r[15];
    if(cpu->thumb_mode)
        pc -= 2;
    else
        pc -= 4;

    printf("CPSR: %08X\n", cpu->CPSR);
    printf("SPSR: %08X\n", *getSPSR(cpu));
    printf("PC: %08X %d\n", pc, cpu->thumb_mode);
    printf("\n");
}

void arm7tdmi_step(arm7tdmi_t* cpu){
    if(cpu->thumb_mode)
        thumb_step(cpu);
    else
        arm_step(cpu);
}

void arm7tdmi_pipeline_refill(arm7tdmi_t* cpu){
    cpu->thumb_mode ? thumb_pipeline_refill(cpu) : arm_pipeline_refill(cpu);
}

void saveBankedReg(arm7tdmi_t* cpu){
    switch(cpu->mode_bits){
        case 0x10:
        case 0x1F:
        memcpy(cpu->usr_r, cpu->r, sizeof(u32)*16);
        break;

        case 0x11:
        memcpy(cpu->usr_r, cpu->r, sizeof(u32)*8);;
        memcpy(cpu->fiq_r, &cpu->r[8], sizeof(u32)*7);
        memcpy(&cpu->usr_r[15], &cpu->r[15], sizeof(u32)*1);
        break;

        case 0x12:
        memcpy(cpu->usr_r, cpu->r, sizeof(u32)*13);;
        memcpy(cpu->irq_r, &cpu->r[13], sizeof(u32)*2);
        memcpy(&cpu->usr_r[15], &cpu->r[15], sizeof(u32)*1);
        break;

        case 0x13:
        memcpy(cpu->usr_r, cpu->r, sizeof(u32)*13);;
        memcpy(cpu->svc_r, &cpu->r[13], sizeof(u32)*2);
        memcpy(&cpu->usr_r[15], &cpu->r[15], sizeof(u32)*1);
        break;

        case 0x1B:
        memcpy(cpu->usr_r, cpu->r, sizeof(u32)*13);;
        memcpy(cpu->und_r, &cpu->r[13], sizeof(u32)*2);
        memcpy(&cpu->usr_r[15], &cpu->r[15], sizeof(u32)*1);
        break;

        break;

        default:
        printf("CANNOT DECODE MODE! 0x%X\n", cpu->mode_bits);
        exit(0);
        break;
    }
}

void loadBankedReg(arm7tdmi_t* cpu){
    switch(cpu->mode_bits){
        case 0x10:
        case 0x1F:
        memcpy(cpu->r, cpu->usr_r, sizeof(u32)*16);
        break;

        case 0x11:
        memcpy(cpu->r, cpu->usr_r, sizeof(u32)*8);
        memcpy(&cpu->r[8], cpu->fiq_r, sizeof(u32)*7);
        memcpy(&cpu->r[15], &cpu->usr_r[15], sizeof(u32)*1);
        break;

        case 0x12:
        memcpy(cpu->r, cpu->usr_r, sizeof(u32)*13);
        memcpy(&cpu->r[13], cpu->irq_r, sizeof(u32)*2);
        memcpy(&cpu->r[15], &cpu->usr_r[15], sizeof(u32)*1);
        break;

        case 0x13:
        memcpy(cpu->r, cpu->usr_r, sizeof(u32)*13);
        memcpy(&cpu->r[13], cpu->svc_r, sizeof(u32)*2);
        memcpy(&cpu->r[15], &cpu->usr_r[15], sizeof(u32)*1);
        break;

        case 0x1B:
        memcpy(cpu->r, cpu->usr_r, sizeof(u32)*13);
        memcpy(&cpu->r[13], cpu->und_r, sizeof(u32)*2);
        memcpy(&cpu->r[15], &cpu->usr_r[15], sizeof(u32)*1);
        break;

        default:
        printf("CANNOT DECODE MODE! 0x%X\n", cpu->mode_bits);
        exit(0);
        break;
    }
}

u32* getSPSR(arm7tdmi_t* cpu){
    switch(cpu->mode_bits){
        case 0x11:
        return &cpu->SPSR_fiq;

        case 0x12:
        return &cpu->SPSR_irq;

        case 0x13:
        return &cpu->SPSR_svc;

        default:
        return &cpu->CPSR;
    }
}


void arm7tdmi_trigger_exception(arm7tdmi_t* cpu, u32 addr, u8 mode){
    u8 old_mode = cpu->mode_bits;
    saveBankedReg(cpu);
    cpu->mode_bits = mode;
    *getSPSR(cpu) = (cpu->CPSR & ~(0x1F)) |  old_mode;
    loadBankedReg(cpu);
    u32 ret_addr = cpu->r[15];
    ret_addr -= cpu->thumb_mode ? 2 : 4;
    if(mode == 0x13)
        cpu->r[14] = ret_addr;
    if(mode == 0x12)
        cpu->r[14] = ret_addr + 4;
    cpu->r[15] = addr;
    saveBankedReg(cpu);
    cpu->thumb_mode = false;
    cpu->irq_disable = true;
    arm7tdmi_pipeline_refill(cpu);
}

void serialize_arm7tdmi_t(arm7tdmi_t* arm, byte_vec_t* vec) {
    byte_vec_push_array(vec, (u8*)&arm->r, sizeof(arm->r));
    byte_vec_push_array(vec, (u8*)&arm->usr_r, sizeof(arm->usr_r));
    byte_vec_push_array(vec, (u8*)&arm->irq_r, sizeof(arm->irq_r));
    byte_vec_push_array(vec, (u8*)&arm->svc_r, sizeof(arm->svc_r));
    byte_vec_push_array(vec, (u8*)&arm->fiq_r, sizeof(arm->fiq_r));
    byte_vec_push_array(vec, (u8*)&arm->und_r, sizeof(arm->und_r));

    byte_vec_push_array(vec, (u8*)&arm->CPSR, sizeof(arm->CPSR));
    byte_vec_push_array(vec, (u8*)&arm->SPSR_fiq, sizeof(arm->SPSR_fiq));
    byte_vec_push_array(vec, (u8*)&arm->SPSR_svc, sizeof(arm->SPSR_svc));
    byte_vec_push_array(vec, (u8*)&arm->SPSR_irq, sizeof(arm->SPSR_irq));

    byte_vec_push_array(vec, (u8*)&arm->pipeline_opcode, sizeof(arm->pipeline_opcode));
    byte_vec_push_array(vec, (u8*)&arm->cycles, sizeof(arm->cycles));
    
    byte_vec_push_array(vec, (u8*)&arm->fetch_seq, sizeof(arm->fetch_seq));
}

u8* deserialize_arm7tdmi_t(arm7tdmi_t* arm, u8* data, u8* end) {
    for (int i = 0; i < 16; i++) {
        if (data + sizeof(u32) > end) return NULL;
        memcpy(&arm->r[i], data, sizeof(u32)); data += sizeof(u32);
    }

    for (int i = 0; i < 16; i++) {
        if (data + sizeof(u32) > end) return NULL;
        memcpy(&arm->usr_r[i], data, sizeof(u32)); data += sizeof(u32);
    }

    for (int i = 0; i < 2; i++) {
        if (data + sizeof(u32) > end) return NULL;
        memcpy(&arm->irq_r[i], data, sizeof(u32)); data += sizeof(u32);
    }

    for (int i = 0; i < 2; i++) {
        if (data + sizeof(u32) > end) return NULL;
        memcpy(&arm->svc_r[i], data, sizeof(u32)); data += sizeof(u32);
    }

    for (int i = 0; i < 7; i++) {
        if (data + sizeof(u32) > end) return NULL;
        memcpy(&arm->fiq_r[i], data, sizeof(u32)); data += sizeof(u32);
    }

    for (int i = 0; i < 2; i++) {
        if (data + sizeof(u32) > end) return NULL;
        memcpy(&arm->und_r[i], data, sizeof(u32)); data += sizeof(u32);
    }

    if (data + sizeof(u32) > end) return NULL;
    memcpy(&arm->CPSR, data, sizeof(u32)); data += sizeof(u32);

    if (data + sizeof(u32) > end) return NULL;
    memcpy(&arm->SPSR_fiq, data, sizeof(u32)); data += sizeof(u32);

    if (data + sizeof(u32) > end) return NULL;
    memcpy(&arm->SPSR_svc, data, sizeof(u32)); data += sizeof(u32);

    if (data + sizeof(u32) > end) return NULL;
    memcpy(&arm->SPSR_irq, data, sizeof(u32)); data += sizeof(u32);

    for (int i = 0; i < 2; i++) {
        if (data + sizeof(u32) > end) return NULL;
        memcpy(&arm->pipeline_opcode[i], data, sizeof(u32)); data += sizeof(u32);
    }

    if (data + sizeof(u32) > end) return NULL;
    memcpy(&arm->cycles, data, sizeof(u32)); data += sizeof(u32);

    if (data + sizeof(bool) > end) return NULL;
    memcpy(&arm->fetch_seq, data, sizeof(bool)); data += sizeof(bool);

    return data;
}