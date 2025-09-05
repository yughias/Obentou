#ifndef __INFO_H__
#define __INFO_H__

#include "types.h"


#define SULEIMAN_TREASURE_CHECKSUM 0xEAC5
#define PUPPET_KNIGHT_CHECKSUM 0x41AB

bool gb_isNewLinceseeCode(u8*);
const char* gb_getRomName(u8*);
const char* gb_getManufacturerName(u8*);
const char* gb_getCartridgeType(u8*, size_t);
size_t gb_getRomSize(u8*);
size_t gb_getRamSize(u8*);
void gb_printInfo(u8*, size_t);
u16 gb_calculateRomChecksum(u8*, size_t);

#endif