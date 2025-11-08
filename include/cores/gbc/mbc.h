#ifndef __MBC_H__
#define __MBC_H__

#include "types.h"

#include "utils/serializer.h"

typedef struct gb_t gb_t;

typedef u8 (*readGbFunc)(gb_t*, u16);
typedef void (*writeGbFunc)(gb_t*, u16, u8);

#define MBC_STRUCT(X) \
    X(readGbFunc, mapper_0000_3FFF, 0, 0) \
    X(readGbFunc, mapper_4000_7FFF, 0, 0) \
    X(readGbFunc, mapper_A000_BFFF_read, 0, 0) \
    X(writeGbFunc, mapper_A000_BFFF_write, 0, 0) \
    X(writeGbFunc, rom_write, 0, 0) \
    X(u8, REG_0000_1FFF, 1, 0) \
    X(u8, REG_2000_3FFF, 1, 0) \
    X(u8, REG_4000_5FFF, 1, 0) \
    X(u8, REG_6000_7FFF, 1, 0) \
    X(u8, REG_2000_2FFF, 1, 0) \
    X(u8, REG_3000_3FFF, 1, 0) \
    X(bool, hasBattery, 0, 0) \
    X(bool, mbcAlreadyWritten, 1, 0) \
    X(bool, hasRtc, 0, 0) \
    X(bool, hasCamera, 0, 0) \
    X(bool, hasRumble, 0, 0) \
    X(void*, data, 0, 0) \
    X(size_t, dataSize, 0, 0) \

DECLARE_SERIALIZABLE_STRUCT(mbc, MBC_STRUCT)

void gb_detectConsoleAndMbc(gb_t*);
bool gb_detectM161(const u8*);
bool gb_detectMMM01(const u8*, size_t);
bool gb_detectMBC1M(const u8*, size_t);
bool gb_containNintendoLogo(const u8*);

#endif