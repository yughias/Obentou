#ifndef __VIDEO_H__
#define __VIDEO_H__

#define REFRESH_RATE 50.08
#define WINDOW_WIDTH 352
#define WINDOW_HEIGHT 296
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
#define SCREEN_X0 48
#define SCREEN_Y0 48
#define NON_VISIBLE_LINE 16
#define LINE_T_STATES 224
#define BORDER_T_STATES 24
#define SCREEN_T_STATES 128
#define RETRACE_T_STATES 48

#include <stdbool.h>
#include <stdint.h>

extern bool flashRevert;

uint16_t getScreenPixelIndex(uint8_t, uint8_t);
int getPixelColor(bool, uint8_t, uint8_t);
void getBorderColor(uint8_t*, uint8_t*, uint8_t*);
void updateColorFlash();
void emulateUlaRender(size_t clock);

#endif
