#include "utils/controls.h"
#include "utils/argument.h"

#include "SDL_MAINLOOP.h"

#include "minIni.h"
#include "tinyfiledialogs.h"

#include <stdlib.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

#define ACTIVE_BUTTONS (end - begin + 1)
#define HOTKEYS_COUNT (CONTROL_HOTKEY_END - CONTROL_HOTKEY_BEGIN + 1)

static SDL_Scancode control_scancode_maps[CONTROL_COUNT];
static SDL_GamepadButton control_gamepad_maps[CONTROL_COUNT];
static control_t begin;
static control_t end;

static bool* pressed;
static bool* prev_pressed;

static bool hotkeys_pressed_arr[HOTKEYS_COUNT];
static bool hotkeys_prev_pressed_arr[HOTKEYS_COUNT];

static SDL_Gamepad* gamepad = NULL;

static bool disable_illegal;
static control_t dpad;

#define DPAD_UP pressed[dpad+0 - begin]
#define DPAD_DOWN pressed[dpad+1 - begin]
#define DPAD_LEFT pressed[dpad+2 - begin]
#define DPAD_RIGHT pressed[dpad+3 - begin]

#define DECLARE_CONTROL_NAME2(system, name) [ CONTROL_ ## system ## _ ## name ] = #name,
#define DECLARE_CONTROL_NAME3(system, name, val) [ CONTROL_ ## system ## _ ## name ] = #name,
#define DECLARE_CONTROL_NAME(...) GET_MACRO_ENUM(__VA_ARGS__, DECLARE_CONTROL_NAME3, DECLARE_CONTROL_NAME2)(__VA_ARGS__)

const char controls_names[CONTROL_COUNT][32] = {
    [CONTROL_NONE] = "None",
    CONTROLS_ENUM(DECLARE_CONTROL_NAME)
};

#define SCANCODES(XYZ) \
XYZ(HOTKEY, PAUSE, "p"); \
XYZ(HOTKEY, CMD_PAUSE, "left ctrl"); \
XYZ(HOTKEY, RESET, "r"); \
XYZ(HOTKEY, CMD_RESET, "left ctrl"); \
XYZ(HOTKEY, TURBO, "tab"); \
XYZ(HOTKEY, CMD_TURBO, "none"); \
XYZ(HOTKEY, REWIND, "tab"); \
XYZ(HOTKEY, CMD_REWIND, "left shift"); \
XYZ(HOTKEY, OPEN, "o"); \
XYZ(HOTKEY, CMD_OPEN, "left ctrl"); \
XYZ(HOTKEY, SPEEDUP, "="); \
XYZ(HOTKEY, CMD_SPEEDUP, "none"); \
XYZ(HOTKEY, SAVESTATE, "s"); \
XYZ(HOTKEY, CMD_SAVESTATE, "left ctrl"); \
XYZ(HOTKEY, LOADSTATE, "l"); \
XYZ(HOTKEY, CMD_LOADSTATE, "left ctrl"); \
XYZ(HOTKEY, SLOWDOWN, "-"); \
XYZ(HOTKEY, CMD_SLOWDOWN, "none"); \
XYZ(HOTKEY, OPEN_BIOS, "b"); \
XYZ(HOTKEY, CMD_OPEN_BIOS, "left ctrl"); \
\
XYZ(NES, UP, "up"); \
XYZ(NES, DOWN, "down"); \
XYZ(NES, LEFT, "left"); \
XYZ(NES, RIGHT, "right"); \
XYZ(NES, A, "x"); \
XYZ(NES, B, "z"); \
XYZ(NES, SELECT, "right shift"); \
XYZ(NES, START, "return"); \
\
XYZ(WATARA, UP, "up"); \
XYZ(WATARA, DOWN, "down"); \
XYZ(WATARA, LEFT, "left"); \
XYZ(WATARA, RIGHT, "right"); \
XYZ(WATARA, A, "x"); \
XYZ(WATARA, B, "z"); \
XYZ(WATARA, SELECT, "right shift"); \
XYZ(WATARA, START, "return"); \
\
XYZ(GBC, UP, "up"); \
XYZ(GBC, DOWN, "down"); \
XYZ(GBC, LEFT, "left"); \
XYZ(GBC, RIGHT, "right"); \
XYZ(GBC, A, "x"); \
XYZ(GBC, B, "z"); \
XYZ(GBC, SELECT, "right shift"); \
XYZ(GBC, START, "return"); \
\
XYZ(PV1000, UP, "up"); \
XYZ(PV1000, DOWN, "down"); \
XYZ(PV1000, LEFT, "left"); \
XYZ(PV1000, RIGHT, "right"); \
XYZ(PV1000, BTN_1, "z"); \
XYZ(PV1000, BTN_2, "x"); \
XYZ(PV1000, SELECT, "right shift"); \
XYZ(PV1000, START, "return"); \
\
XYZ(BYTEPUSHER, 1, "1"); \
XYZ(BYTEPUSHER, 2, "2"); \
XYZ(BYTEPUSHER, 3, "3"); \
XYZ(BYTEPUSHER, C, "4"); \
XYZ(BYTEPUSHER, 4, "q"); \
XYZ(BYTEPUSHER, 5, "w"); \
XYZ(BYTEPUSHER, 6, "e"); \
XYZ(BYTEPUSHER, D, "r"); \
XYZ(BYTEPUSHER, 7, "a"); \
XYZ(BYTEPUSHER, 8, "s"); \
XYZ(BYTEPUSHER, 9, "d"); \
XYZ(BYTEPUSHER, E, "f"); \
XYZ(BYTEPUSHER, A, "z"); \
XYZ(BYTEPUSHER, 0, "x"); \
XYZ(BYTEPUSHER, B, "c"); \
XYZ(BYTEPUSHER, F, "v"); \
\
XYZ(PCE, UP, "up"); \
XYZ(PCE, DOWN, "down"); \
XYZ(PCE, LEFT, "left"); \
XYZ(PCE, RIGHT, "right"); \
XYZ(PCE, BTN_2, "z"); \
XYZ(PCE, BTN_1, "x"); \
XYZ(PCE, SELECT, "right shift"); \
XYZ(PCE, START, "return"); \
\
XYZ(TMS80, UP, "up"); \
XYZ(TMS80, DOWN, "down"); \
XYZ(TMS80, LEFT, "left"); \
XYZ(TMS80, RIGHT, "right"); \
XYZ(TMS80, BTN_1, "z"); \
XYZ(TMS80, BTN_2, "x"); \
XYZ(TMS80, PAUSE, "f1"); \
XYZ(TMS80, GG_START, "return"); \
\
XYZ(TMS80, 1, "1"); \
XYZ(TMS80, Q, "q"); \
XYZ(TMS80, A, "a"); \
XYZ(TMS80, Z, "z"); \
XYZ(TMS80, ED, "right ctrl"); \
XYZ(TMS80, COMMA, ","); \
XYZ(TMS80, K, "k"); \
XYZ(TMS80, I, "i"); \
XYZ(TMS80, 8, "8"); \
XYZ(TMS80, 2, "2"); \
XYZ(TMS80, W, "w"); \
XYZ(TMS80, S, "s"); \
XYZ(TMS80, X, "x"); \
XYZ(TMS80, SPC, "space"); \
XYZ(TMS80, DOT, "."); \
XYZ(TMS80, L, "l"); \
XYZ(TMS80, O, "o"); \
XYZ(TMS80, 9, "9"); \
XYZ(TMS80, 3, "3"); \
XYZ(TMS80, E, "e"); \
XYZ(TMS80, D, "d"); \
XYZ(TMS80, C, "c"); \
XYZ(TMS80, HC, "delete"); \
XYZ(TMS80, SLASH, "/"); \
XYZ(TMS80, SEMICOLON, ";"); \
XYZ(TMS80, P, "p"); \
XYZ(TMS80, 0, "0"); \
XYZ(TMS80, 4, "4"); \
XYZ(TMS80, R, "r"); \
XYZ(TMS80, F, "f"); \
XYZ(TMS80, V, "v"); \
XYZ(TMS80, ID, "backspace"); \
XYZ(TMS80, PI, "right alt"); \
XYZ(TMS80, COLON, "\'"); \
XYZ(TMS80, AT, "\\"); \
XYZ(TMS80, MINUS, "-"); \
XYZ(TMS80, 5, "5"); \
XYZ(TMS80, T, "t"); \
XYZ(TMS80, G, "g"); \
XYZ(TMS80, B, "b"); \
XYZ(TMS80, DA, "down"); \
XYZ(TMS80, CLOSE_BRACKET, "]"); \
XYZ(TMS80, OPEN_BRACKET, "["); \
XYZ(TMS80, CARET, "="); \
XYZ(TMS80, 6, "6"); \
XYZ(TMS80, Y, "y"); \
XYZ(TMS80, H, "h"); \
XYZ(TMS80, N, "n"); \
XYZ(TMS80, LA, "left"); \
XYZ(TMS80, CR, "return"); \
XYZ(TMS80, YEN, "`"); \
XYZ(TMS80, FNC, "tab"); \
XYZ(TMS80, 7, "7"); \
XYZ(TMS80, U, "u"); \
XYZ(TMS80, J, "j"); \
XYZ(TMS80, M, "m"); \
XYZ(TMS80, RA, "right"); \
XYZ(TMS80, UA, "up"); \
XYZ(TMS80, BRK, "right shift"); \
XYZ(TMS80, GRP, "left alt"); \
XYZ(TMS80, CTL, "left ctrl"); \
XYZ(TMS80, SHF, "left shift");

#define GAMEPADS(XYZ) \
XYZ(HOTKEY, PAUSE, "none"); \
XYZ(HOTKEY, RESET, "none"); \
XYZ(HOTKEY, TURBO, "none"); \
XYZ(HOTKEY, REWIND, "none"); \
XYZ(HOTKEY, OPEN,  "none"); \
XYZ(HOTKEY, SPEEDUP, "none"); \
XYZ(HOTKEY, SLOWDOWN, "none"); \
XYZ(HOTKEY, OPEN_BIOS, "none"); \
XYZ(HOTKEY, LOADSTATE, "none"); \
XYZ(HOTKEY, SAVESTATE, "none"); \
\
XYZ(GBC, A, "b"); \
XYZ(GBC, B, "a"); \
XYZ(GBC, SELECT, "back"); \
XYZ(GBC, START, "start"); \
XYZ(GBC, UP, "dpup"); \
XYZ(GBC, DOWN, "dpdown"); \
XYZ(GBC, LEFT, "dpleft"); \
XYZ(GBC, RIGHT, "dpright"); \
\
XYZ(PV1000, BTN_1, "a"); \
XYZ(PV1000, BTN_2, "b"); \
XYZ(PV1000, SELECT, "back"); \
XYZ(PV1000, START, "start"); \
XYZ(PV1000, UP, "dpup"); \
XYZ(PV1000, DOWN, "dpdown"); \
XYZ(PV1000, LEFT, "dpleft"); \
XYZ(PV1000, RIGHT, "dpright"); \
\
XYZ(WATARA, A, "b"); \
XYZ(WATARA, B, "a"); \
XYZ(WATARA, SELECT, "back"); \
XYZ(WATARA, START, "start"); \
XYZ(WATARA, UP, "dpup"); \
XYZ(WATARA, DOWN, "dpdown"); \
XYZ(WATARA, LEFT, "dpleft"); \
XYZ(WATARA, RIGHT, "dpright"); \
\
XYZ(NES, A, "b"); \
XYZ(NES, B, "a"); \
XYZ(NES, SELECT, "back"); \
XYZ(NES, START, "start"); \
XYZ(NES, UP, "dpup"); \
XYZ(NES, DOWN, "dpdown"); \
XYZ(NES, LEFT, "dpleft"); \
XYZ(NES, RIGHT, "dpright"); \
\
XYZ(PCE, BTN_2, "a"); \
XYZ(PCE, BTN_1, "b"); \
XYZ(PCE, SELECT, "back"); \
XYZ(PCE, START, "start"); \
XYZ(PCE, UP, "dpup"); \
XYZ(PCE, DOWN, "dpdown"); \
XYZ(PCE, LEFT, "dpleft"); \
XYZ(PCE, RIGHT, "dpright"); \
\
XYZ(TMS80, UP, "dpup"); \
XYZ(TMS80, DOWN, "dpdown"); \
XYZ(TMS80, LEFT, "dpleft"); \
XYZ(TMS80, RIGHT, "dpright"); \
XYZ(TMS80, BTN_1, "a"); \
XYZ(TMS80, BTN_2, "b"); \
XYZ(TMS80, PAUSE, "none"); \
XYZ(TMS80, GG_START, "start");

#define LOAD_SCANCODE(console, button, default) { \
    char name[64] = ""; \
    ini_gets(#console, "INPUT_KEY_" #button, default, name, 64, argument_get_ini_path()); \
    SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN; \
    if(strcasecmp(name, "none")) { \
        scancode = SDL_GetScancodeFromName(name); \
        if(scancode == SDL_SCANCODE_UNKNOWN) { \
            printf("Unknown scancode for %s: %s\n", #console "_" #button, name); \
            scancode = SDL_GetScancodeFromName(default); \
        } \
    } \
    control_scancode_maps[CONTROL_ ## console ## _ ## button] = scancode;\
}

#define SAVE_SCANCODE(console, button, default) { \
    const char* name = SDL_GetScancodeName(control_scancode_maps[CONTROL_ ## console ## _ ## button]); \
    ini_puts(#console, "INPUT_KEY_" #button, name &&name[0] ? name : "none", argument_get_ini_path()); \
}

#define LOAD_GAMEPAD(console, button, default) { \
    char name[64] = ""; \
    ini_gets(#console, "INPUT_GAMEPAD_" #button, default, name, 64, argument_get_ini_path()); \
    SDL_GamepadButton pad_btn = SDL_GAMEPAD_BUTTON_INVALID; \
    if(strcasecmp(name, "none")) { \
        pad_btn = SDL_GetGamepadButtonFromString(name); \
        if(pad_btn == SDL_GAMEPAD_BUTTON_INVALID) { \
            printf("Unknown gamepad button for %s: %s\n", #console "_" #button, name); \
        } \
    } \
    control_gamepad_maps[CONTROL_ ## console ## _ ## button] = pad_btn;\
}

#define SAVE_GAMEPAD(console, button, default) { \
    const char* name = SDL_GetGamepadStringForButton(control_gamepad_maps[CONTROL_ ## console ## _ ## button]); \
    ini_puts(#console, "INPUT_GAMEPAD_" #button, name && name[0] ? name : "none", argument_get_ini_path()); \
}

const char* controls_get_scancode_name(control_t input){
    const char* name = SDL_GetScancodeName(control_scancode_maps[input]);
    return name && name[0] ? name : "None";
}

const char* controls_get_gamepad_name(control_t input){
    const char* name = SDL_GetGamepadStringForButton(control_gamepad_maps[input]);
    return name && name[0] ? name : "None";
}

void controls_set_scancode(control_t control, const char* new_scancode_name){
    if(!new_scancode_name)
        return;
    SDL_Scancode new_scancode = SDL_GetScancodeFromName(new_scancode_name);
    control_scancode_maps[control] = new_scancode;
}

void controls_set_gamepad(control_t control, const char* new_gamepad_name){
    if(!new_gamepad_name)
        return;
    SDL_GamepadButton new_gamepad = SDL_GetGamepadButtonFromString(new_gamepad_name);
    control_gamepad_maps[control] = new_gamepad;
}

void controls_load_maps(){
    SCANCODES(LOAD_SCANCODE);
    GAMEPADS(LOAD_GAMEPAD);
}

void controls_save_maps(){
    SCANCODES(SAVE_SCANCODE);
    GAMEPADS(SAVE_GAMEPAD);
}

void controls_init(control_t begin_, control_t end_) {
    controls_free();
    begin = begin_;
    end = end_;
    pressed = malloc(ACTIVE_BUTTONS * sizeof(bool));
    prev_pressed = malloc(ACTIVE_BUTTONS * sizeof(bool));
    memset(pressed, 0, ACTIVE_BUTTONS * sizeof(bool));
    memset(prev_pressed, 0, ACTIVE_BUTTONS * sizeof(bool));

    disable_illegal = ini_getbool("GENERAL", "DISABLE_ILLEGAL_INPUT", true, argument_get_ini_path());

    dpad = CONTROL_NONE;
    for(int i = begin; i <= end; i++){
        const char* name = controls_names[i];
        if(strstr(name, "UP")){
            dpad = i;
            break;
        }
    }
}

void controls_disable_illegal_input(bool disable){
    disable_illegal = disable;
}

void controls_free(){
    free(pressed);
    free(prev_pressed);

    if(gamepad){
        SDL_CloseGamepad(gamepad);
    }
}

void controls_update(){
    if(!gamepad){
        int num_gamepads;
        SDL_JoystickID* gamepads = SDL_GetGamepads(&num_gamepads);
        if(num_gamepads){
            gamepad = SDL_OpenGamepad(gamepads[0]);
            SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_ACCEL, true);
            printf("opened %p %s\n", gamepad, SDL_GetGamepadName(gamepad));
        }
    } else if(!SDL_GamepadConnected(gamepad)){
        gamepad = NULL;
        printf("closed %p\n", gamepad);
    }

    const bool* keystate = SDL_GetKeyboardState(NULL);

    memcpy(hotkeys_prev_pressed_arr, hotkeys_pressed_arr, HOTKEYS_COUNT * sizeof(bool));

    for(int i = 0; i < HOTKEYS_COUNT; i++){
        int cmd_idx = CONTROL_HOTKEY_CMD_BEGIN + i;
        int hotkey_idx = CONTROL_HOTKEY_BEGIN + i;
        bool active = keystate[control_scancode_maps[cmd_idx]] || (control_scancode_maps[cmd_idx] == SDL_SCANCODE_UNKNOWN);
        bool key = keystate[control_scancode_maps[hotkey_idx]];
        bool gamepad_btn = SDL_GetGamepadButton(gamepad, control_gamepad_maps[hotkey_idx]);
        bool hit = (active && key) || gamepad_btn;
        hotkeys_pressed_arr[i] = hit;
    }

    // return if controls are not initialized
    if(!pressed || !prev_pressed)
        return;
    
    memcpy(prev_pressed, pressed, ACTIVE_BUTTONS * sizeof(bool));

    for(int i = begin; i <= end; i++){
        pressed[i - begin] = keystate[control_scancode_maps[i]] || SDL_GetGamepadButton(gamepad, control_gamepad_maps[i]);
    }

    if(disable_illegal && dpad){
        bool dp[4] = { DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT };
        DPAD_UP &= !(dp[0] && dp[1]);
        DPAD_DOWN &= !(dp[0] && dp[1]);
        DPAD_LEFT &= !(dp[2] && dp[3]);
        DPAD_RIGHT &= !(dp[2] && dp[3]);
    }
}

bool controls_pressed(control_t control){
    if(control == CONTROL_NONE)
        return false;

    return pressed[control - begin];
}

bool controls_released(control_t control){
    if(control == CONTROL_NONE)
        return false;

    return !pressed[control - begin] && prev_pressed[control - begin];
}

bool hotkeys_pressed(control_t control){
    return hotkeys_pressed_arr[control - CONTROL_HOTKEY_BEGIN];
}

bool hotkeys_released(control_t control){
    return !hotkeys_pressed_arr[control - CONTROL_HOTKEY_BEGIN] && hotkeys_prev_pressed_arr[control - CONTROL_HOTKEY_BEGIN];
}

bool controls_gamepad_connected(){
    return gamepad;
}

bool controls_rumble(u16 low, u16 hi, u32 duration){
    return SDL_RumbleGamepad(gamepad, low, hi, duration);
}

void controls_get_gamepad_accelerometer(float* sensors){
    sensors[0] = 0.0f;
    sensors[1] = 0.0f;
    sensors[2] = 0.0f;
    SDL_GetGamepadSensorData(gamepad, SDL_SENSOR_ACCEL, sensors, 3);
}

bool controls_double_click(){
    static unsigned int last_pressed = -1;
    static unsigned int last_released = -1;

    const unsigned int detect_value = 15;

    SDL_MouseButtonFlags state = SDL_GetMouseState(NULL, NULL);
    bool pressed = state & SDL_BUTTON_LMASK;
    bool released = !pressed && (last_pressed == frameCount - 1); 

    bool out = false;

    if(pressed){
        last_pressed = frameCount;
    }

    if(released){
        out = (frameCount - last_released) <= detect_value;
        last_released = frameCount;
    }

    return out;
}