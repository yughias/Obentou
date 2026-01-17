#ifndef __UTILS_MENU_H__
#define __UTILS_MENU_H__

#include "types.h"

typedef struct core_ctx_t core_ctx_t;

void menu_create(core_ctx_t* ctx);
void menu_speed_check(int speed_level);
void menu_tick_pause(bool paused);
void menu_fullscreen();
void menu_open_rom(core_ctx_t* ctx);
void menu_open_bios(core_ctx_t* ctx);
void menu_save_screenshot(core_ctx_t* ctx);

#endif