#include "cores/speccy/speccy.h"

#include "utils/sound.h"

static void get_sample(void* ctx, void* sample_void){
    speccy_t* speccy = (speccy_t*)ctx;
    ay_t* ay = &speccy->ay;

    const uint16_t volume_multiplier = SDL_MAX_SINT16 / 4;

    u16* sample = (u16*)sample_void;
    *sample = ay_get_sample(ay, volume_multiplier);
    bool buzzer = (speccy->ula & 0b10000); 
    *sample += sound_set_channel_sample(buzzer, 3) ? volume_multiplier : 0;
}


void speccy_send_audio(void* ctx){
    sound_push_sample(1, sizeof(u16), ctx, get_sample);
}
