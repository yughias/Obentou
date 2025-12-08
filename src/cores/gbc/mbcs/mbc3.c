#include "cores/gbc/mbc.h"
#include "cores/gbc/mbcs/mbc3.h"
#include "cores/gbc/memory.h"
#include "cores/gbc/gb.h"

#include <time.h>
#include <stdio.h>

typedef struct rtc_t {
    u8 REG_08;
    u8 REG_09;
    u8 REG_0A;
    u8 REG_0B;
    u8 REG_0C;
    u8 REG_08_LATCHED;
    u8 REG_09_LATCHED;
    u8 REG_0A_LATCHED;
    u8 REG_0B_LATCHED;
    u8 REG_0C_LATCHED;
    uint64_t timestamp;
} rtc_t;

static void emulateRtc(rtc_t* rtc);
static void stepRtc(rtc_t* rtc, uint64_t secs);

rtc_t* gb_allocRtc(size_t* size){ void* out = malloc(sizeof(rtc_t)); memset(out, 0, sizeof(rtc_t)); *size = sizeof(rtc_t); return out; }

#define READ_RTC(addr) case 0x ## addr: return rtc->REG_ ## addr ## _LATCHED
#define WRITE_RTC(addr) case 0x ## addr: rtc->REG_ ## addr = byte; return

u8 gb_mbc3_4000_7FFF(gb_t* gb, u16 addr){
    mbc_t* mbc = &gb->mbc;
    size_t real_addr = addr;
    real_addr &= (1 << 14) - 1;
    u8 bank = mbc->REG_2000_3FFF == 0x00 ? 0x01 : (mbc->REG_2000_3FFF & 0b1111111); 
    real_addr |= bank << 14;
    real_addr &= gb->ROM_SIZE - 1;
    return gb->ROM[real_addr];
}

u8 gb_mbc3_ram_read(gb_t* gb, u16 addr){
    mbc_t* mbc = &gb->mbc;
    rtc_t* rtc = (rtc_t*)mbc->data;
    if((mbc->REG_0000_1FFF & 0x0F) != 0x0A)
        return 0xFF;

    size_t real_addr = addr;
    switch(mbc->REG_4000_5FFF){
        READ_RTC(08);
        READ_RTC(09);
        READ_RTC(0A);
        READ_RTC(0B);
        READ_RTC(0C);

        default:
        real_addr &= (1 << 13) - 1;
        real_addr |= (mbc->REG_4000_5FFF & 0b11) << 13;
        real_addr &= gb->ERAM_SIZE - 1;
        return gb->ERAM[real_addr];
    }
}

void gb_mbc3_ram_write(gb_t* gb, u16 addr, u8 byte){
    mbc_t* mbc = &gb->mbc;
    rtc_t* rtc = (rtc_t*)mbc->data;
    if((mbc->REG_0000_1FFF & 0x0F) != 0x0A)
        return;

    size_t real_addr = addr;
    switch(mbc->REG_4000_5FFF){
        WRITE_RTC(08);
        WRITE_RTC(09);
        WRITE_RTC(0A);
        WRITE_RTC(0B);
        
        case 0x0C:    
        if(rtc->REG_0C & (1 << 6))
            rtc->timestamp = time(NULL);
        rtc->REG_0C = byte;
        break;

        default:
        real_addr &= (1 << 13) - 1;
        real_addr |= (mbc->REG_4000_5FFF & 0b11) << 13;
        real_addr &= gb->ERAM_SIZE - 1;
        gb->ERAM[real_addr] = byte;
    }
}

void gb_mbc3_registers(gb_t* gb, u16 addr, u8 byte){
    mbc_t* mbc = &gb->mbc;
    if(addr < 0x2000)
        mbc->REG_0000_1FFF = byte;
    else if(addr < 0x4000)
        mbc->REG_2000_3FFF = byte;
    else if(addr < 0x6000)
        mbc->REG_4000_5FFF = byte;
    else if(mbc->hasRtc) {
        if(mbc->REG_6000_7FFF == 0x00 && byte == 0x01){
            rtc_t* rtc = (rtc_t*)mbc->data;
            emulateRtc(rtc);
            rtc->REG_08_LATCHED = rtc->REG_08 & 0b111111;
            rtc->REG_09_LATCHED = rtc->REG_09 & 0b111111;
            rtc->REG_0A_LATCHED = rtc->REG_0A & 0b11111;
            rtc->REG_0B_LATCHED = rtc->REG_0B;
            rtc->REG_0C_LATCHED = rtc->REG_0C & 0b11000001;
        }
        mbc->REG_6000_7FFF = byte;
    }
}

static void emulateRtc(rtc_t* rtc){
    if(rtc->REG_0C & (1 << 6))
        return;

    uint64_t elapsed_sec = time(NULL) - rtc->timestamp;
    stepRtc(rtc, elapsed_sec);

    if(elapsed_sec != 0)
        rtc->timestamp = time(NULL);
}

static void stepRtc(rtc_t* rtc, uint64_t secs){
    uint64_t day_counter = rtc->REG_0B + ((rtc->REG_0C & 1) << 8);
    uint64_t rtc_time = 0;
    rtc_time += (rtc->REG_08 % 60);
    rtc_time += ((rtc->REG_09 % 60) * 60);
    rtc_time += ((rtc->REG_0A % 24) * 60 * 60);
    rtc_time += day_counter * 24 * 60 * 60;
    uint64_t new_rtc_time = rtc_time + secs;

    rtc->REG_08 = new_rtc_time % 60;
    rtc->REG_09 = (new_rtc_time / 60) % 60;
    rtc->REG_0A = (new_rtc_time / 3600) % 24;
    uint64_t new_day_counter = new_rtc_time / (24*60*60); 
    bool day_carry = new_day_counter >= 512 ? 1 : 0;
    rtc->REG_0B = new_day_counter % 512;
    rtc->REG_0C &= 0b11000000;
    rtc->REG_0C |= (bool)(new_day_counter & 0x100);
    rtc->REG_0C |= day_carry << 7;
}

// use BGB 64 bit compatible save format 

void gb_saveRtc(rtc_t* rtc, const char* filename){
    u32 data[12];

    stepRtc(rtc, time(NULL) - rtc->timestamp);
    rtc->timestamp = time(NULL);

    data[0] = rtc->REG_08; 
    data[1] = rtc->REG_09; 
    data[2] = rtc->REG_0A; 
    data[3] = rtc->REG_0B; 
    data[4] = rtc->REG_0C; 
    data[5] = rtc->REG_08_LATCHED; 
    data[6] = rtc->REG_09_LATCHED; 
    data[7] = rtc->REG_0A_LATCHED; 
    data[8] = rtc->REG_0B_LATCHED; 
    data[9] = rtc->REG_0C_LATCHED; 
    data[10] = rtc->timestamp;
    data[11] = rtc->timestamp >> 32;

    file_append(filename, (u8*)data, sizeof(data));
}

void gb_loadRtc(rtc_t* rtc, u8* sav_data, size_t sav_size){
    if(sav_size % 8192 != 48)
        return;

    u32* data_ptr = (u32*)(sav_data + sav_size - 48);

    rtc->REG_08 = data_ptr[0]; 
    rtc->REG_09 = data_ptr[1]; 
    rtc->REG_0A = data_ptr[2]; 
    rtc->REG_0B = data_ptr[3]; 
    rtc->REG_0C = data_ptr[4]; 
    rtc->REG_08_LATCHED = data_ptr[5]; 
    rtc->REG_09_LATCHED = data_ptr[6]; 
    rtc->REG_0A_LATCHED = data_ptr[7]; 
    rtc->REG_0B_LATCHED = data_ptr[8]; 
    rtc->REG_0C_LATCHED = data_ptr[9];
    rtc->timestamp = data_ptr[10] | (((u64)data_ptr[11]) << 32);

    stepRtc(rtc, time(NULL) - rtc->timestamp);
    rtc->timestamp = time(NULL);
}