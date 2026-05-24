#ifndef __SEGA_INTERFACE_H__
#define __SEGA_INTERFACE_H__

#define SEGA_run_frame TMS80_run_frame
#define SEGA_init TMS80_init
bool SEGA_detect(const archive_t* rom_archive, const archive_t* bios_archive);
#define SEGA_savestate TMS80_savestate
#define SEGA_loadstate TMS80_loadstate
#define SEGA_close TMS80_close

#define SEGA_WIDTH TMS80_WIDTH
#define SEGA_HEIGHT TMS80_HEIGHT
#define SEGA_FPS TMS80_FPS
#define SEGA_SOUND_PUSH_RATE TMS80_SOUND_PUSH_RATE
#define SEGA_sound_callback TMS80_sound_callback
#define SEGA_has_bios TMS80_has_bios

#define SEGA_AUDIO_SPEC TMS80_AUDIO_SPEC
#define SEGA_sound_channels TMS80_sound_channels
#define SEGA_widgets TMS80_widgets

#endif