#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "cpus/z80.h"
#include "types.h"

u8 tms80_sg_sc_readMemory(z80_t* z80, u16 addr);
void tms80_sg_sc_writeMemory(z80_t* z80, u16 addr, u8 byte);
u8 tms80_sg_sc_mirrored_readMemory(z80_t* z80, u16 addr);
void tms80_sg_sc_ram_adapter_writeMemory(z80_t* z80, u16 addr, u8 byte);

u8 tms80_sms_bios_readMemory(z80_t* z80, u16 addr);
void tms80_sms_bios_writeMemory(z80_t* z80, u16 addr, u8 byte);
u8 tms80_sms_readMemory(z80_t* z80, u16 addr);
void tms80_sms_writeMemory(z80_t* z80, u16 addr, u8 byte);

u8 tms80_readIO(z80_t* z80, u16 addr);
void tms80_writeIO(z80_t* z80, u16 addr, u8 byte);

#endif