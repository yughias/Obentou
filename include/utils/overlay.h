#ifndef __OVERLAY_H__
#define __OVERLAY_H__

#include "types.h"

#include "utils/controls.h"

void overlay_init(const core_t* core);
void overlay_clear();
bool overlay_pressed(control_t button);

#endif