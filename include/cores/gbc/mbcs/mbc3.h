#ifndef __MBC3_H__
#define __MBC3_H__

#include "types.h"

typedef struct gb_t gb_t;
typedef struct rtc_t rtc_t;

u8 gb_mbc3_4000_7FFF(gb_t* gb, u16 addr);
u8 gb_mbc3_ram_read(gb_t* gb, u16 addr);
void gb_mbc3_ram_write(gb_t* gb, u16 addr, u8 byte);
void gb_mbc3_registers(gb_t* gb, u16 addr, u8 byte);
void gb_saveRtc(rtc_t* rtc, const char* filename);
void gb_loadRtc(rtc_t* rtc, u8* sav_data, size_t sav_size);
rtc_t* gb_allocRtc(size_t* size);

#endif