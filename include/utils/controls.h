#ifndef __CONTROLS_PERIPHERAL__
#define __CONTROLS_PERIPHERAL__

#include "types.h"

#define CONTROLS_ENUM(XY) \
    XY(HOTKEY, BEGIN) \
    XY(HOTKEY, PAUSE, BEGIN) \
    XY(HOTKEY, TURBO) \
    XY(HOTKEY, RESET) \
    XY(HOTKEY, SLOWDOWN) \
    XY(HOTKEY, SPEEDUP) \
    XY(HOTKEY, SAVESTATE) \
    XY(HOTKEY, LOADSTATE) \
    XY(HOTKEY, OPEN) \
    XY(HOTKEY, OPEN_BIOS) \
    XY(HOTKEY, END, OPEN_BIOS) \
    XY(HOTKEY_CMD, BEGIN) \
    XY(HOTKEY_CMD, PAUSE, BEGIN) \
    XY(HOTKEY_CMD, TURBO) \
    XY(HOTKEY_CMD, RESET) \
    XY(HOTKEY_CMD, SLOWDOWN) \
    XY(HOTKEY_CMD, SPEEDUP) \
    XY(HOTKEY_CMD, SAVESTATE) \
    XY(HOTKEY_CMD, LOADSTATE) \
    XY(HOTKEY_CMD, OPEN) \
    XY(HOTKEY_CMD, OPEN_BIOS) \
    XY(HOTKEY_CMD, END, OPEN_BIOS) \
    XY(GBC, BEGIN) \
    XY(GBC, A, BEGIN) \
    XY(GBC, B) \
    XY(GBC, START) \
    XY(GBC, SELECT) \
    XY(GBC, RIGHT) \
    XY(GBC, LEFT) \
    XY(GBC, UP) \
    XY(GBC, DOWN) \
    XY(GBC, END, DOWN) \
    XY(NES, BEGIN) \
    XY(NES, A, BEGIN) \
    XY(NES, B) \
    XY(NES, SELECT) \
    XY(NES, START) \
    XY(NES, UP) \
    XY(NES, DOWN) \
    XY(NES, LEFT) \
    XY(NES, RIGHT) \
    XY(NES, END, RIGHT) \
    XY(PV1000, BEGIN) \
    XY(PV1000, BTN_1, BEGIN) \
    XY(PV1000, BTN_2) \
    XY(PV1000, START) \
    XY(PV1000, SELECT) \
    XY(PV1000, UP) \
    XY(PV1000, DOWN) \
    XY(PV1000, LEFT) \
    XY(PV1000, RIGHT) \
    XY(PV1000, END, RIGHT) \
    XY(WATARA, BEGIN) \
    XY(WATARA, A, BEGIN) \
    XY(WATARA, B) \
    XY(WATARA, START) \
    XY(WATARA, SELECT) \
    XY(WATARA, RIGHT) \
    XY(WATARA, LEFT) \
    XY(WATARA, UP) \
    XY(WATARA, DOWN) \
    XY(WATARA, END, DOWN) \
    XY(PCE, BEGIN) \
    XY(PCE, BTN_1, BEGIN) \
    XY(PCE, BTN_2) \
    XY(PCE, START) \
    XY(PCE, SELECT) \
    XY(PCE, RIGHT) \
    XY(PCE, LEFT) \
    XY(PCE, UP) \
    XY(PCE, DOWN) \
    XY(PCE, END, DOWN) \
    XY(TMS80, BEGIN) \
    XY(TMS80, 1, BEGIN) \
    XY(TMS80, Q) \
    XY(TMS80, A) \
    XY(TMS80, Z) \
    XY(TMS80, ED) \
    XY(TMS80, COMMA) \
    XY(TMS80, K) \
    XY(TMS80, I) \
    XY(TMS80, 8) \
    XY(TMS80, 2) \
    XY(TMS80, W) \
    XY(TMS80, S) \
    XY(TMS80, X) \
    XY(TMS80, SPC) \
    XY(TMS80, DOT) \
    XY(TMS80, L) \
    XY(TMS80, O) \
    XY(TMS80, 9) \
    XY(TMS80, 3) \
    XY(TMS80, E) \
    XY(TMS80, D) \
    XY(TMS80, C) \
    XY(TMS80, HC) \
    XY(TMS80, SLASH) \
    XY(TMS80, SEMICOLON) \
    XY(TMS80, P) \
    XY(TMS80, 0) \
    XY(TMS80, 4) \
    XY(TMS80, R) \
    XY(TMS80, F) \
    XY(TMS80, V) \
    XY(TMS80, ID) \
    XY(TMS80, PI) \
    XY(TMS80, COLON) \
    XY(TMS80, AT) \
    XY(TMS80, MINUS) \
    XY(TMS80, 5) \
    XY(TMS80, T) \
    XY(TMS80, G) \
    XY(TMS80, B) \
    XY(TMS80, DA) \
    XY(TMS80, CLOSE_BRACKET) \
    XY(TMS80, OPEN_BRACKET) \
    XY(TMS80, CARET) \
    XY(TMS80, 6) \
    XY(TMS80, Y) \
    XY(TMS80, H) \
    XY(TMS80, N) \
    XY(TMS80, LA) \
    XY(TMS80, CR) \
    XY(TMS80, YEN) \
    XY(TMS80, FNC) \
    XY(TMS80, 7) \
    XY(TMS80, U) \
    XY(TMS80, J) \
    XY(TMS80, M) \
    XY(TMS80, RA) \
    XY(TMS80, UA) \
    XY(TMS80, BRK) \
    XY(TMS80, GRP) \
    XY(TMS80, CTL) \
    XY(TMS80, SHF) \
    XY(TMS80, UP) \
    XY(TMS80, DOWN) \
    XY(TMS80, LEFT) \
    XY(TMS80, RIGHT) \
    XY(TMS80, BTN_1) \
    XY(TMS80, BTN_2) \
    XY(TMS80, PAUSE) \
    XY(TMS80, GG_START) \
    XY(TMS80, END, GG_START) \
    XY(BYTEPUSHER, BEGIN) \
    XY(BYTEPUSHER, 0, BEGIN) \
    XY(BYTEPUSHER, 1) \
    XY(BYTEPUSHER, 2) \
    XY(BYTEPUSHER, 3) \
    XY(BYTEPUSHER, 4) \
    XY(BYTEPUSHER, 5) \
    XY(BYTEPUSHER, 6) \
    XY(BYTEPUSHER, 7) \
    XY(BYTEPUSHER, 8) \
    XY(BYTEPUSHER, 9) \
    XY(BYTEPUSHER, A) \
    XY(BYTEPUSHER, B) \
    XY(BYTEPUSHER, C) \
    XY(BYTEPUSHER, D) \
    XY(BYTEPUSHER, E) \
    XY(BYTEPUSHER, F) \
    XY(BYTEPUSHER, END, F) \

#define GET_MACRO_ENUM(_1, _2, _3, NAME, ...) NAME

#define DECLARE_CONTROL_ENUM2(system, name) CONTROL_ ## system ## _ ## name,
#define DECLARE_CONTROL_ENUM3(system, name, val) CONTROL_ ## system ## _ ## name = CONTROL_ ## system ## _ ## val,
#define DECLARE_CONTROL_ENUM(...) GET_MACRO_ENUM(__VA_ARGS__, DECLARE_CONTROL_ENUM3, DECLARE_CONTROL_ENUM2)(__VA_ARGS__)

typedef enum control_t {
    CONTROL_NONE,
    CONTROLS_ENUM(DECLARE_CONTROL_ENUM)
    CONTROL_COUNT
} control_t;

extern const char controls_names[CONTROL_COUNT][32];

void controls_init(control_t begin, control_t end);
void controls_update();
void controls_free();
void controls_load_maps();
void controls_save_maps();
bool controls_pressed(control_t input);
bool controls_released(control_t input);
bool hotkeys_pressed(control_t input);
bool hotkeys_released(control_t input);
bool controls_double_click();
bool controls_gamepad_connected();
bool controls_rumble(u16 low, u16 hi, u32 duration);
void controls_get_gamepad_accelerometer(float* sensors);
const char* controls_get_scancode_name(control_t input);
const char* controls_get_gamepad_name(control_t input);

#endif