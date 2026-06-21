#ifndef __CONTROLS_PERIPHERAL__
#define __CONTROLS_PERIPHERAL__

#include "types.h"

#define MAX_PLAYERS 2
#define MAX_GAMEPADS 4

#define CONTROLS_BOTH -1

#define CONTROLS_ENUM(XY) \
    XY(HOTKEY, PAUSE, BEGIN, "p", "none") \
    XY(HOTKEY, TURBO, "tab", "none") \
    XY(HOTKEY, REWIND, "tab", "none") \
    XY(HOTKEY, RESET, "r", "none") \
    XY(HOTKEY, SLOWDOWN, "-", "none") \
    XY(HOTKEY, SPEEDUP, "=", "none") \
    XY(HOTKEY, SAVESTATE, "s", "none") \
    XY(HOTKEY, LOADSTATE, "l", "none") \
    XY(HOTKEY, DEBUG_VIEW, "d", "none") \
    XY(HOTKEY, OPEN, "o", "none") \
    XY(HOTKEY, OPEN_BIOS, END, "b", "none") \
    \
    XY(HOTKEY_CMD, PAUSE, BEGIN, "left ctrl", "none") \
    XY(HOTKEY_CMD, TURBO, "none", "none") \
    XY(HOTKEY_CMD, REWIND, "left shift", "none") \
    XY(HOTKEY_CMD, RESET, "left ctrl", "none") \
    XY(HOTKEY_CMD, SLOWDOWN, "none", "none") \
    XY(HOTKEY_CMD, SPEEDUP, "none", "none") \
    XY(HOTKEY_CMD, SAVESTATE, "left ctrl", "none") \
    XY(HOTKEY_CMD, LOADSTATE, "left ctrl", "none") \
    XY(HOTKEY_CMD, DEBUG_VIEW, "left ctrl", "none") \
    XY(HOTKEY_CMD, OPEN, "left ctrl", "none") \
    XY(HOTKEY_CMD, OPEN_BIOS, END, "left ctrl", "none") \
    \
    XY(GBC, A, BEGIN, "x", "b") \
    XY(GBC, B, "z", "a") \
    XY(GBC, START, "return", "start") \
    XY(GBC, SELECT, "right shift", "back") \
    XY(GBC, UP, "up", "dpup") \
    XY(GBC, DOWN, "down", "dpdown") \
    XY(GBC, LEFT, "left", "dpleft") \
    XY(GBC, RIGHT, END, "right", "dpright") \
    \
    XY(NES, A, BEGIN, "x", "b") \
    XY(NES, B, "z", "a") \
    XY(NES, SELECT, "right shift", "back") \
    XY(NES, START, "return", "start") \
    XY(NES, UP, "up", "dpup") \
    XY(NES, DOWN, "down", "dpdown") \
    XY(NES, LEFT, "left", "dpleft") \
    XY(NES, RIGHT, END, "right", "dpright") \
    \
    XY(PV1000, BTN_1, BEGIN, "z", "a") \
    XY(PV1000, BTN_2, "x", "b") \
    XY(PV1000, START, "return", "start") \
    XY(PV1000, SELECT, "right shift", "back") \
    XY(PV1000, UP, "up", "dpup") \
    XY(PV1000, DOWN, "down", "dpdown") \
    XY(PV1000, LEFT, "left", "dpleft") \
    XY(PV1000, RIGHT, END, "right", "dpright") \
    \
    XY(WATARA, A, BEGIN, "x", "b") \
    XY(WATARA, B, "z", "a") \
    XY(WATARA, START, "return", "start") \
    XY(WATARA, SELECT, "right shift", "back") \
    XY(WATARA, UP, "up", "dpup") \
    XY(WATARA, DOWN, "down", "dpdown") \
    XY(WATARA, LEFT, "left", "dpleft") \
    XY(WATARA, RIGHT, END, "right", "dpright") \
    \
    XY(PCE, BTN_1, BEGIN, "x", "b") \
    XY(PCE, BTN_2, "z", "a") \
    XY(PCE, START, "return", "start") \
    XY(PCE, SELECT, "right shift", "back") \
    XY(PCE, UP, "up", "dpup") \
    XY(PCE, DOWN, "down", "dpdown") \
    XY(PCE, LEFT, "left", "dpleft") \
    XY(PCE, RIGHT, END, "right", "dpright") \
    \
    XY(SEGA, 1, BEGIN, "1", "none") \
    XY(SEGA, Q, "q", "none") \
    XY(SEGA, A, "a", "none") \
    XY(SEGA, Z, "z", "none") \
    XY(SEGA, ED, "right ctrl", "none") \
    XY(SEGA, COMMA, ",", "none") \
    XY(SEGA, K, "k", "none") \
    XY(SEGA, I, "i", "none") \
    XY(SEGA, 8, "8", "none") \
    XY(SEGA, 2, "2", "none") \
    XY(SEGA, W, "w", "none") \
    XY(SEGA, S, "s", "none") \
    XY(SEGA, X, "x", "none") \
    XY(SEGA, SPC, "space", "none") \
    XY(SEGA, DOT, ".", "none") \
    XY(SEGA, L, "l", "none") \
    XY(SEGA, O, "o", "none") \
    XY(SEGA, 9, "9", "none") \
    XY(SEGA, 3, "3", "none") \
    XY(SEGA, E, "e", "none") \
    XY(SEGA, D, "d", "none") \
    XY(SEGA, C, "c", "none") \
    XY(SEGA, HC, "delete", "none") \
    XY(SEGA, SLASH, "/", "none") \
    XY(SEGA, SEMICOLON, "", "none") \
    XY(SEGA, P, "p", "none") \
    XY(SEGA, 0, "0", "none") \
    XY(SEGA, 4, "4", "none") \
    XY(SEGA, R, "r", "none") \
    XY(SEGA, F, "f", "none") \
    XY(SEGA, V, "v", "none") \
    XY(SEGA, ID, "backspace", "none") \
    XY(SEGA, PI, "right alt", "none") \
    XY(SEGA, COLON, "\'", "none") \
    XY(SEGA, AT, "\\", "none") \
    XY(SEGA, MINUS, "-", "none") \
    XY(SEGA, 5, "5", "none") \
    XY(SEGA, T, "t", "none") \
    XY(SEGA, G, "g", "none") \
    XY(SEGA, B, "b", "none") \
    XY(SEGA, DA, "down", "none") \
    XY(SEGA, CLOSE_BRACKET, "]", "none") \
    XY(SEGA, OPEN_BRACKET, "[", "none") \
    XY(SEGA, CARET, "=", "none") \
    XY(SEGA, 6, "6", "none") \
    XY(SEGA, Y, "y", "none") \
    XY(SEGA, H, "h", "none") \
    XY(SEGA, N, "n", "none") \
    XY(SEGA, LA, "left", "none") \
    XY(SEGA, CR, "return", "none") \
    XY(SEGA, YEN, "`", "none") \
    XY(SEGA, FNC, "tab", "none") \
    XY(SEGA, 7, "7", "none") \
    XY(SEGA, U, "u", "none") \
    XY(SEGA, J, "j", "none") \
    XY(SEGA, M, "m", "none") \
    XY(SEGA, RA, "right", "none") \
    XY(SEGA, UA, "up", "none") \
    XY(SEGA, BRK, "right shift", "none") \
    XY(SEGA, GRP, "left alt", "none") \
    XY(SEGA, CTL, "left ctrl", "none") \
    XY(SEGA, SHF, "left shift", "none") \
    XY(SEGA, UP, "up", "dpup") \
    XY(SEGA, DOWN, "down", "dpdown") \
    XY(SEGA, LEFT, "left", "dpleft") \
    XY(SEGA, RIGHT, "right", "dpright") \
    XY(SEGA, BTN_1, "z", "a") \
    XY(SEGA, BTN_2, "x", "b") \
    XY(SEGA, PAUSE, "f1", "none") \
    XY(SEGA, GG_START, END, "return", "start") \
    \
    XY(COLECO, UP, BEGIN, "up", "dpup") \
    XY(COLECO, DOWN, "down", "dpdown") \
    XY(COLECO, LEFT, "left", "dpleft") \
    XY(COLECO, RIGHT, "right", "dpright") \
    XY(COLECO, 0, "0", "none") \
    XY(COLECO, 1, "1", "start") \
    XY(COLECO, 2, "2", "back") \
    XY(COLECO, 3, "3", "none") \
    XY(COLECO, 4, "4", "none") \
    XY(COLECO, 5, "5", "none") \
    XY(COLECO, 6, "6", "none") \
    XY(COLECO, 7, "7", "none") \
    XY(COLECO, 8, "8", "none") \
    XY(COLECO, 9, "9", "none") \
    XY(COLECO, HASHTAG, "q", "z") \
    XY(COLECO, ASTERISK, "w", "x") \
    XY(COLECO, BLUE, "a", "none") \
    XY(COLECO, PURPLE, "s", "none") \
    XY(COLECO, BTN_1, "z", "a") \
    XY(COLECO, BTN_2, END, "x", "b") \
    \
    XY(BYTEPUSHER, 0, BEGIN, "x", "none") \
    XY(BYTEPUSHER, 1, "1", "none") \
    XY(BYTEPUSHER, 2, "2", "none") \
    XY(BYTEPUSHER, 3, "3", "none") \
    XY(BYTEPUSHER, 4, "q", "none") \
    XY(BYTEPUSHER, 5, "w", "none") \
    XY(BYTEPUSHER, 6, "e", "none") \
    XY(BYTEPUSHER, 7, "a", "none") \
    XY(BYTEPUSHER, 8, "s", "none") \
    XY(BYTEPUSHER, 9, "d", "none") \
    XY(BYTEPUSHER, A, "z", "none") \
    XY(BYTEPUSHER, B, "c", "none") \
    XY(BYTEPUSHER, C, "4", "none") \
    XY(BYTEPUSHER, D, "r", "none") \
    XY(BYTEPUSHER, E, "f", "none") \
    XY(BYTEPUSHER, F, END, "v", "none") \
    \
    XY(CHIP8, 0, BEGIN, "x", "none") \
    XY(CHIP8, 1, "1", "none") \
    XY(CHIP8, 2, "2", "none") \
    XY(CHIP8, 3, "3", "none") \
    XY(CHIP8, 4, "q", "none") \
    XY(CHIP8, 5, "w", "none") \
    XY(CHIP8, 6, "e", "none") \
    XY(CHIP8, 7, "a", "none") \
    XY(CHIP8, 8, "s", "none") \
    XY(CHIP8, 9, "d", "none") \
    XY(CHIP8, A, "z", "none") \
    XY(CHIP8, B, "c", "none") \
    XY(CHIP8, C, "4", "none") \
    XY(CHIP8, D, "r", "none") \
    XY(CHIP8, E, "f", "none") \
    XY(CHIP8, F, END, "v", "none") \
    \
    XY(PACMAN, UP, BEGIN, "up", "dpup") \
    XY(PACMAN, DOWN, "down", "dpdown") \
    XY(PACMAN, LEFT, "left", "dpleft") \
    XY(PACMAN, RIGHT, "right", "dpright") \
    XY(PACMAN, BUTTON, "z", "b") \
    XY(PACMAN, COIN, "right shift", "back") \
    XY(PACMAN, START1, "1", "start") \
    XY(PACMAN, START2, END, "2", "none") \
    \
    XY(SPACEINVADERS, FIRE, BEGIN, "z", "b") \
    XY(SPACEINVADERS, LEFT, "left", "dpleft") \
    XY(SPACEINVADERS, RIGHT, "right", "dpright") \
    XY(SPACEINVADERS, COIN, "right shift", "back") \
    XY(SPACEINVADERS, START, END, "return", "start") \
    \
    XY(SPECCY, UP, BEGIN, "up", "none") \
    XY(SPECCY, DOWN, "down", "none") \
    XY(SPECCY, LEFT, "left", "none") \
    XY(SPECCY, RIGHT, "right", "none") \
    XY(SPECCY, JOY_BTN, "x", "none") \
    XY(SPECCY, SHIFT, "left shift", "none") \
    XY(SPECCY, Z, "z", "none") \
    XY(SPECCY, X, "x", "none") \
    XY(SPECCY, C, "c", "none") \
    XY(SPECCY, V, "v", "none") \
    XY(SPECCY, A, "a", "none") \
    XY(SPECCY, S, "s", "none") \
    XY(SPECCY, D, "d", "none") \
    XY(SPECCY, F, "f", "none") \
    XY(SPECCY, G, "g", "none") \
    XY(SPECCY, Q, "q", "none") \
    XY(SPECCY, W, "w", "none") \
    XY(SPECCY, E, "e", "none") \
    XY(SPECCY, R, "r", "none") \
    XY(SPECCY, T, "t", "none") \
    XY(SPECCY, 1, "1", "none") \
    XY(SPECCY, 2, "2", "none") \
    XY(SPECCY, 3, "3", "none") \
    XY(SPECCY, 4, "4", "none") \
    XY(SPECCY, 5, "5", "none") \
    XY(SPECCY, 0, "0", "none") \
    XY(SPECCY, 9, "9", "none") \
    XY(SPECCY, 8, "8", "none") \
    XY(SPECCY, 7, "7", "none") \
    XY(SPECCY, 6, "6", "none") \
    XY(SPECCY, P, "p", "none") \
    XY(SPECCY, O, "o", "none") \
    XY(SPECCY, I, "i", "none") \
    XY(SPECCY, U, "u", "none") \
    XY(SPECCY, Y, "y", "none") \
    XY(SPECCY, ENTER, "return", "none") \
    XY(SPECCY, L, "l", "none") \
    XY(SPECCY, K, "k", "none") \
    XY(SPECCY, J, "j", "none") \
    XY(SPECCY, H, "h", "none") \
    XY(SPECCY, SPACE, "space", "none") \
    XY(SPECCY, SYM_SHIFT, "left alt", "none") \
    XY(SPECCY, M, "m", "none") \
    XY(SPECCY, N, "n", "none") \
    XY(SPECCY, B, "b", "none") \
    XY(SPECCY, DELETE, END, "backspace", "none")

#define CONTROLS_TYPE_ENUM(XY) \
    XY(GBC, DEFAULT, BEGIN, END) \
    \
    XY(NES, DEFAULT, BEGIN, END) \
    \
    XY(PV1000, DEFAULT, BEGIN, END) \
    \
    XY(WATARA, DEFAULT, BEGIN, END) \
    \
    XY(PCE, TWO_BUTTONS_CONTROLLER, BEGIN, END) \
    \
    XY(SEGA, DEFAULT, BEGIN, END) \
    \
    XY(COLECO, DEFAULT, BEGIN, END) \
    \
    XY(BYTEPUSHER, DEFAULT, BEGIN, END) \
    \
    XY(CHIP8, DEFAULT, BEGIN, END) \
    \
    XY(PACMAN, DEFAULT, BEGIN, END) \
    \
    XY(SPACEINVADERS, DEFAULT, BEGIN, END) \
    \
    XY(SPECCY, KEYBOARD, BEGIN) \
    XY(SPECCY, KEYBOARD_WITH_CURSOR) \
    XY(SPECCY, KEYBOARD_WITH_KEMPSTON, END)

    
#define GET_MACRO_ENUM(_1, _2, _3, _4, _5, NAME, ...) NAME
#define DECLARE_CONTROL_ENUM4(system, name, scancode, gamepad) CONTROL_ ## system ## _ ## name,
#define DECLARE_CONTROL_ENUM5(system, name, val, scancode, gamepad) DECLARE_CONTROL_ENUM4(system, name, scancode, gamepad) CONTROL_ ## system ## _ ## val = CONTROL_ ## system ## _ ## name,
#define DECLARE_CONTROL_ENUM(...) GET_MACRO_ENUM(__VA_ARGS__, DECLARE_CONTROL_ENUM5, DECLARE_CONTROL_ENUM4)(__VA_ARGS__)

#define DECLARE_CONTROL_TYPE_ENUM2(system, name) CONTROL_TYPE_ ## system ## _ ## name,
#define DECLARE_CONTROL_TYPE_ENUM3(system, name, val) DECLARE_CONTROL_TYPE_ENUM2(system, name) CONTROL_TYPE_ ## system ## _ ## val = CONTROL_TYPE_ ## system ## _ ## name,
#define DECLARE_CONTROL_TYPE_ENUM4(system, name, val1, val2) DECLARE_CONTROL_TYPE_ENUM2(system, name) CONTROL_TYPE_ ## system ## _ ## val1 = CONTROL_TYPE_ ## system ## _ ## name, CONTROL_TYPE_ ## system ## _ ## val2 = CONTROL_TYPE_ ## system ## _ ## name,
#define DECLARE_CONTROL_TYPE_ENUM(...) GET_MACRO_ENUM(__VA_ARGS__, _, DECLARE_CONTROL_TYPE_ENUM4, DECLARE_CONTROL_TYPE_ENUM3, DECLARE_CONTROL_TYPE_ENUM2)(__VA_ARGS__)

typedef enum control_t {
    CONTROL_ALWAYS = -1,
    CONTROL_NONE = 0,
    CONTROLS_ENUM(DECLARE_CONTROL_ENUM)
    CONTROL_COUNT
} control_t;

typedef enum control_type_t {
    CONTROLS_TYPE_ENUM(DECLARE_CONTROL_TYPE_ENUM)
    CONTROL_TYPE_COUNT
} control_type_t;

extern const char controls_names[CONTROL_COUNT][32];
extern const char controls_type_names[CONTROL_TYPE_COUNT][32];

typedef struct core_t core_t;
void controls_init(const core_t* core);
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
void controls_set_type(const char* name, control_type_t type);
control_type_t controls_get_actual_type();
#endif
