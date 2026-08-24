#ifndef __COLECO_INTERFACE_H__
#define __COLECO_INTERFACE_H__

#define COLECO_run_frame TMS80_run_frame
#define COLECO_init TMS80_init
bool COLECO_detect(const archive_t* rom_archive, const archive_t* bios_archive);
#define COLECO_savestate TMS80_savestate
#define COLECO_loadstate TMS80_loadstate
#define COLECO_free TMS80_free
#define COLECO_save TMS80_save

#define COLECO_WIDTH TMS80_WIDTH
#define COLECO_HEIGHT TMS80_HEIGHT
#define COLECO_FPS TMS80_FPS
#define COLECO_SOUND_PUSH_RATE TMS80_SOUND_PUSH_RATE
#define COLECO_sound_callback TMS80_sound_callback
#define COLECO_has_bios TMS80_has_bios

#define COLECO_AUDIO_SPEC TMS80_AUDIO_SPEC
#define COLECO_sound_channels TMS80_sound_channels
#define COLECO_widgets TMS80_widgets

#endif