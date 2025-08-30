#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "types.h"

u8 tms80_sg_sc_readMemory(void* ctx, u16 addr);
void tms80_sg_sc_writeMemory(void* ctx, u16 addr, u8 byte);
u8 tms80_sg_sc_mirrored_readMemory(void* ctx, u16 addr);
void tms80_sg_sc_ram_adapter_writeMemory(void* ctx, u16 addr, u8 byte);

u8 tms80_sms_bios_readMemory(void* ctx, u16 addr);
void tms80_sms_bios_writeMemory(void* ctx, u16 addr, u8 byte);
u8 tms80_sms_readMemory(void* ctx, u16 addr);
void tms80_sms_writeMemory(void* ctx, u16 addr, u8 byte);

u8 tms80_readIO(void* ctx, u16 addr);
void tms80_writeIO(void* ctx, u16 addr, u8 byte);

#endif