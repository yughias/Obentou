#ifndef __LOADER_H__
#define __LOADER_H__

#include "types.h"

typedef struct speccy_t speccy_t;

void speccy_load_z80_state(speccy_t* speccy, u8* buffer, size_t size);
void speccy_load_scr(speccy_t* speccy, u8* buffer, size_t size);

#endif