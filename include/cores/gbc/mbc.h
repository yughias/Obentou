#ifndef __MBC_H__
#define __MBC_H__

#include "types.h"

typedef struct gb_t gb_t;

typedef u8 (*readGbFunc)(gb_t*, u16);
typedef void (*writeGbFunc)(gb_t*, u16, u8);

typedef struct mbc_t {
    readGbFunc mapper_0000_3FFF;
    readGbFunc mapper_4000_7FFF;
    readGbFunc mapper_A000_BFFF_read;
    writeGbFunc mapper_A000_BFFF_write;

    writeGbFunc rom_write;

    u8 REG_0000_1FFF;
    u8 REG_2000_3FFF;
    u8 REG_4000_5FFF;
    u8 REG_6000_7FFF;

    // advanced mbc registers
    u8 REG_2000_2FFF;
    u8 REG_3000_3FFF;

    bool hasBattery;
    bool mbcAlreadyWritten;
    bool hasRtc;
    bool hasCamera;
    bool hasRumble;

    void* data;
} mbc_t;

void gb_detectConsoleAndMbc(gb_t*);
bool gb_detectM161(const u8*);
bool gb_detectMMM01(const u8*, size_t);
bool gb_detectMBC1M(const u8*, size_t);
bool gb_containNintendoLogo(const u8*);

#endif