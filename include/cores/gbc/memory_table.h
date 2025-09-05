#ifndef __MEMORY_TABLE_H__
#define __MEMORY_TABLE_H__

#include "types.h"

typedef struct gb_t gb_t;
typedef u8 (*readGbFunc)(gb_t*, u16);
typedef void (*writeGbFunc)(gb_t*, u16, u8);

void gb_fillReadTable(readGbFunc*, u8, u8, readGbFunc);
void gb_fillWriteTable(writeGbFunc*, u8, u8, writeGbFunc);

u8 gb_readBootrom(gb_t*, u16);
u8 gb_readVram(gb_t*, u16);
u8 gb_readWram(gb_t*, u16);
u8 gb_readMirrorRam(gb_t*, u16);
u8 gb_readOam(gb_t*, u16);
u8 gb_readIO(gb_t*, u16);
u8 gb_readCRAM(u8*, u8*);

void gb_writeVram(gb_t*, u16, u8);
void gb_writeWram(gb_t*, u16, u8);
void gb_writeMirrorRam(gb_t*, u16, u8);
void gb_writeOam(gb_t*, u16, u8);
void gb_writeIO(gb_t*, u16, u8);
void gb_writeCRAM(u8*, u8, u8*);

#endif