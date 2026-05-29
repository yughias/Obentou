#ifndef __CONTROLS_PERIPHERAL__
#define __CONTROLS_PERIPHERAL__

#include "types.h"

#define MAX_PLAYERS 2
#define MAX_GAMEPADS 4

#define CONTROLS_BOTH -1

#define CONTROLS_ENUM(XY) \
    XY(HOTKEY, PAUSE, BEGIN) \
    XY(HOTKEY, TURBO) \
    XY(HOTKEY, REWIND) \
    XY(HOTKEY, RESET) \
    XY(HOTKEY, SLOWDOWN) \
    XY(HOTKEY, SPEEDUP) \
    XY(HOTKEY, SAVESTATE) \
    XY(HOTKEY, LOADSTATE) \
    XY(HOTKEY, DEBUG_VIEW) \
    XY(HOTKEY, OPEN) \
    XY(HOTKEY, OPEN_BIOS, END) \
    \
    XY(HOTKEY_CMD, PAUSE, BEGIN) \
    XY(HOTKEY_CMD, TURBO) \
    XY(HOTKEY_CMD, REWIND) \
    XY(HOTKEY_CMD, RESET) \
    XY(HOTKEY_CMD, SLOWDOWN) \
    XY(HOTKEY_CMD, SPEEDUP) \
    XY(HOTKEY_CMD, SAVESTATE) \
    XY(HOTKEY_CMD, LOADSTATE) \
    XY(HOTKEY_CMD, DEBUG_VIEW) \
    XY(HOTKEY_CMD, OPEN) \
    XY(HOTKEY_CMD, OPEN_BIOS, END) \
    \
    XY(GBC, A, BEGIN) \
    XY(GBC, B) \
    XY(GBC, START) \
    XY(GBC, SELECT) \
    XY(GBC, UP) \
    XY(GBC, DOWN) \
    XY(GBC, LEFT) \
    XY(GBC, RIGHT, END) \
    \
    XY(NES, A, BEGIN) \
    XY(NES, B) \
    XY(NES, SELECT) \
    XY(NES, START) \
    XY(NES, UP) \
    XY(NES, DOWN) \
    XY(NES, LEFT) \
    XY(NES, RIGHT, END) \
    \
    XY(PV1000, BTN_1, BEGIN) \
    XY(PV1000, BTN_2) \
    XY(PV1000, START) \
    XY(PV1000, SELECT) \
    XY(PV1000, UP) \
    XY(PV1000, DOWN) \
    XY(PV1000, LEFT) \
    XY(PV1000, RIGHT, END) \
    \
    XY(WATARA, A, BEGIN) \
    XY(WATARA, B) \
    XY(WATARA, START) \
    XY(WATARA, SELECT) \
    XY(WATARA, UP) \
    XY(WATARA, DOWN) \
    XY(WATARA, LEFT) \
    XY(WATARA, RIGHT, END) \
    \
    XY(PCE, BTN_1, BEGIN) \
    XY(PCE, BTN_2) \
    XY(PCE, START) \
    XY(PCE, SELECT) \
    XY(PCE, UP) \
    XY(PCE, DOWN) \
    XY(PCE, LEFT) \
    XY(PCE, RIGHT, END) \
    \
    XY(SEGA, 1, BEGIN) \
    XY(SEGA, Q) \
    XY(SEGA, A) \
    XY(SEGA, Z) \
    XY(SEGA, ED) \
    XY(SEGA, COMMA) \
    XY(SEGA, K) \
    XY(SEGA, I) \
    XY(SEGA, 8) \
    XY(SEGA, 2) \
    XY(SEGA, W) \
    XY(SEGA, S) \
    XY(SEGA, X) \
    XY(SEGA, SPC) \
    XY(SEGA, DOT) \
    XY(SEGA, L) \
    XY(SEGA, O) \
    XY(SEGA, 9) \
    XY(SEGA, 3) \
    XY(SEGA, E) \
    XY(SEGA, D) \
    XY(SEGA, C) \
    XY(SEGA, HC) \
    XY(SEGA, SLASH) \
    XY(SEGA, SEMICOLON) \
    XY(SEGA, P) \
    XY(SEGA, 0) \
    XY(SEGA, 4) \
    XY(SEGA, R) \
    XY(SEGA, F) \
    XY(SEGA, V) \
    XY(SEGA, ID) \
    XY(SEGA, PI) \
    XY(SEGA, COLON) \
    XY(SEGA, AT) \
    XY(SEGA, MINUS) \
    XY(SEGA, 5) \
    XY(SEGA, T) \
    XY(SEGA, G) \
    XY(SEGA, B) \
    XY(SEGA, DA) \
    XY(SEGA, CLOSE_BRACKET) \
    XY(SEGA, OPEN_BRACKET) \
    XY(SEGA, CARET) \
    XY(SEGA, 6) \
    XY(SEGA, Y) \
    XY(SEGA, H) \
    XY(SEGA, N) \
    XY(SEGA, LA) \
    XY(SEGA, CR) \
    XY(SEGA, YEN) \
    XY(SEGA, FNC) \
    XY(SEGA, 7) \
    XY(SEGA, U) \
    XY(SEGA, J) \
    XY(SEGA, M) \
    XY(SEGA, RA) \
    XY(SEGA, UA) \
    XY(SEGA, BRK) \
    XY(SEGA, GRP) \
    XY(SEGA, CTL) \
    XY(SEGA, SHF) \
    XY(SEGA, UP) \
    XY(SEGA, DOWN) \
    XY(SEGA, LEFT) \
    XY(SEGA, RIGHT) \
    XY(SEGA, BTN_1) \
    XY(SEGA, BTN_2) \
    XY(SEGA, PAUSE) \
    XY(SEGA, GG_START, END) \
    \
    XY(COLECO, UP, BEGIN) \
    XY(COLECO, DOWN) \
    XY(COLECO, LEFT) \
    XY(COLECO, RIGHT) \
    XY(COLECO, 0) \
    XY(COLECO, 1) \
    XY(COLECO, 2) \
    XY(COLECO, 3) \
    XY(COLECO, 4) \
    XY(COLECO, 5) \
    XY(COLECO, 6) \
    XY(COLECO, 7) \
    XY(COLECO, 8) \
    XY(COLECO, 9) \
    XY(COLECO, HASHTAG) \
    XY(COLECO, ASTERISK) \
    XY(COLECO, BLUE) \
    XY(COLECO, PURPLE) \
    XY(COLECO, BTN_1) \
    XY(COLECO, BTN_2, END) \
    \
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
    XY(BYTEPUSHER, F, END) \
    \
    XY(CHIP8, 0, BEGIN) \
    XY(CHIP8, 1) \
    XY(CHIP8, 2) \
    XY(CHIP8, 3) \
    XY(CHIP8, 4) \
    XY(CHIP8, 5) \
    XY(CHIP8, 6) \
    XY(CHIP8, 7) \
    XY(CHIP8, 8) \
    XY(CHIP8, 9) \
    XY(CHIP8, A) \
    XY(CHIP8, B) \
    XY(CHIP8, C) \
    XY(CHIP8, D) \
    XY(CHIP8, E) \
    XY(CHIP8, F, END) \
    \
    XY(PACMAN, UP, BEGIN) \
    XY(PACMAN, DOWN) \
    XY(PACMAN, LEFT) \
    XY(PACMAN, RIGHT) \
    XY(PACMAN, BUTTON) \
    XY(PACMAN, COIN) \
    XY(PACMAN, START1) \
    XY(PACMAN, START2, END) \
    \
    XY(SPACEINVADERS, FIRE, BEGIN) \
    XY(SPACEINVADERS, LEFT) \
    XY(SPACEINVADERS, RIGHT) \
    XY(SPACEINVADERS, COIN) \
    XY(SPACEINVADERS, START, END) \
    \
    XY(SPECCY, STUB, BEGIN) \
    XY(SPECCY, STUB1) \
    XY(SPECCY, STUB2, END)

#define GET_MACRO_ENUM(_1, _2, _3, NAME, ...) NAME

#define DECLARE_CONTROL_ENUM2(system, name) CONTROL_ ## system ## _ ## name,
#define DECLARE_CONTROL_ENUM3(system, name, val) DECLARE_CONTROL_ENUM2(system, name) CONTROL_ ## system ## _ ## val = CONTROL_ ## system ## _ ## name,
#define DECLARE_CONTROL_ENUM(...) GET_MACRO_ENUM(__VA_ARGS__, DECLARE_CONTROL_ENUM3, DECLARE_CONTROL_ENUM2)(__VA_ARGS__)

typedef enum control_t {
    CONTROL_ALWAYS = -1,
    CONTROL_NONE = 0,
    CONTROLS_ENUM(DECLARE_CONTROL_ENUM)
    CONTROL_COUNT
} control_t;

extern const char controls_names[CONTROL_COUNT][32];

void controls_init(control_t begin, control_t end);
void controls_update();
void controls_free();
void controls_load_maps();
void controls_save_maps();
bool controls_pressed(control_t input, int port);
bool controls_released(control_t input, int port);
bool hotkeys_pressed(control_t input);
bool hotkeys_released(control_t input);
bool controls_double_click();
bool controls_gamepad_connected();
bool controls_rumble(u16 low, u16 hi, u32 duration);
void controls_get_gamepad_accelerometer(float* sensors);
const char* controls_get_scancode_name(control_t input);
const char* controls_get_gamepad_name(control_t input);
void controls_set_scancode(control_t control, const char* new_scancode_name);
void controls_set_gamepad(control_t control, const char* new_gamepad_name);
void controls_disable_illegal_input(bool disable);
void controls_set_keyboard_player(int player);
void controls_set_gamepad_player(int gamepad_idx, int player);
void controls_get_gamepad_info(int index, char* name, int len, int* id);
int controls_get_gamepad_player(int gamepad_idx);
bool controls_gamepad_search();
#endif
