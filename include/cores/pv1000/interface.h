#ifndef __PV1000_INTERFACE_H__
#define __PV1000_INTERFACE_H__

#include "interface.h"

void pv1000_run_frame(void* ctx);
void* pv1000_init(const char* filename, SDL_AudioDeviceID device_id);
void pv1000_psg_callback(void *userdata, Uint8 * stream, int len);


#define pv1000_width 224
#define pv1000_height 192
#define pv1000_fps 59.9227434033

#define pv1000_audio_spec \
{ \
    .callback = pv1000_psg_callback, \
    .channels = 1, \
    .format = AUDIO_S8, \
    .freq = 44100, \
    .samples = 512, \
} \

#endif