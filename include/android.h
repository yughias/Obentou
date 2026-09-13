#ifndef __ANDROID_H__
#define __ANDROID_H__

#include "types.h"

bool android_is_rewind();
bool android_rumble(u16 amp, u32 duration);

#endif