#ifndef __BYTEPUSHER_INTERFACE_H__
#define __BYTEPUSHER_INTERFACE_H__

void bytepusher_run_frame(void* ctx);
void* bytepusher_init(const char* filename, SDL_AudioDeviceID device_id);
bool bytepusher_detect(const char* filename);

#define bytepusher_width 256
#define bytepusher_height 256
#define bytepusher_fps 60

#define bytepusher_audio_spec \
{ \
    .callback = NULL, \
    .channels = 1, \
    .format = AUDIO_S8, \
    .freq = 15360, \
    .samples = 1024, \
} \

#endif