#ifndef __PCE_INTERFACE_H__
#define __PCE_INTERFACE_H__

void PCE_run_frame(void* ctx);
void* PCE_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool PCE_detect(const archive_t* rom_archive, const archive_t* bios_archive);
byte_vec_t PCE_savestate(void* ctx);
bool PCE_loadstate(void* ctx, byte_vec_t* state);
#define PCE_close NULL

#define PCE_WIDTH 256
#define PCE_HEIGHT 242
#define PCE_FPS 59.8337024193
#define PCE_SOUND_PUSH_RATE (7.16e6/44100.0f)
#define PCE_sound_callback NULL
#define PCE_has_bios false

#define PCE_AUDIO_SPEC \
{ \
    .channels = 2, \
    .format = SDL_AUDIO_F32, \
    .freq = 44100, \
} \

#define PCE_apu_channels "WAVE0", "WAVE1", "WAVE2", "WAVE3", "WAVE4 OR NOISE", "WAVE5 OR NOISE"

#endif