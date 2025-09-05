#ifndef __MBC_CAM_H__
#define __MBC_CAM_H__

#include "types.h"

typedef struct gb_t gb_t;

void gb_mbc_cam_init(gb_t*);
void gb_mbc_cam_free(gb_t*);
u8 gb_mbc_cam_ram_read(gb_t*, u16 addr);
void gb_mbc_cam_ram_write(gb_t*, u16 addr, u8 byte);

#endif