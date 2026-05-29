#ifndef __LOADER_H__
#define __LOADER_H__

#define Z80_V1_HEADER_SIZE 30

#include <stdint.h>

void loadROM(const char*);
void loadState(const char*);
void saveState(const char*);
void loadSCR(const char*);

#endif