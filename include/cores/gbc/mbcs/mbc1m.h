#ifndef __MBC1M_H__
#define __MBC1M_H__

#include <stdint.h>

typedef struct gb_t gb_t;

u8 gb_mbc1m_0000_3FFF(gb_t* gb, u16 addr);
u8 gb_mbc1m_4000_7FFF(gb_t* gb, u16 addr);

#endif