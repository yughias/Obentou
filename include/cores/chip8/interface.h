#ifndef __CHIP8_INTERFACE_H__
#define __CHIP8_INTERFACE_H__

void CHIP8_run_frame(void* ctx);
void* CHIP8_init(const archive_t* rom_archive, const archive_t* bios_archive);
bool CHIP8_detect(const archive_t* rom_archive, const archive_t* bios_archive);
byte_vec_t CHIP8_savestate(void* ctx);
bool CHIP8_loadstate(void* ctx, byte_vec_t* state);
#define CHIP8_close NULL

#define CHIP8_WIDTH 64
#define CHIP8_HEIGHT 32
#define CHIP8_FPS 60
#define CHIP8_SOUND_PUSH_RATE (600.0f/44100.0f)
#define CHIP8_sound_callback NULL
#define CHIP8_has_bios false

#define CHIP8_AUDIO_SPEC \
{ \
    .channels = 1, \
    .format = SDL_AUDIO_S8, \
    .freq = 44100, \
} \

#define CHIP8_sound_channels { "BUZZER", 0, 1 }

#define CHIP8_widgets

#endif