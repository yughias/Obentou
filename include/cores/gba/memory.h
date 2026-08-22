#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "types.h"

#include "utils/serializer.h"

typedef struct gba_t gba_t;
typedef struct arm7tdmi_t arm7tdmi_t;
typedef struct gba_gamepak_t gba_gamepak_t;

#define BIOS_SIZE 0x4000
#define EWRAM_SIZE (1 << 18)
#define IWRAM_SIZE (1 << 15)

#define BIOS_STRUCT(X) \
    X(u8*, data, 0, 0) \
    X(u32, last_fetched, 1, 0)

DECLARE_SERIALIZABLE_STRUCT(bios, BIOS_STRUCT);

u8 gba_read_byte(arm7tdmi_t* cpu, u32 addr, bool seq);
u16 gba_read_halfword(arm7tdmi_t* cpu, u32 addr, bool seq);
u32 gba_read_word(arm7tdmi_t* cpu, u32 addr, bool seq);

void gba_write_byte(arm7tdmi_t* cpu, u32 addr, u8 val, bool seq);
void gba_write_halfword(arm7tdmi_t* cpu, u32 addr, u16 val, bool seq);
void gba_write_word(arm7tdmi_t* cpu, u32 addr, u32 val, bool seq);

#endif