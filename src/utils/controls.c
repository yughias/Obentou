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

static SDL_Gamepad* gamepads[MAX_GAMEPADS];

static bool disable_illegal;
static control_t dpad;

static int keyboard_player;
static int gamepad_players[MAX_GAMEPADS];

#define DPAD_UP(player) pressed[dpad+0 - begin + player * ACTIVE_BUTTONS]
#define DPAD_DOWN(player) pressed[dpad+1 - begin + player * ACTIVE_BUTTONS]
#define DPAD_LEFT(player) pressed[dpad+2 - begin + player * ACTIVE_BUTTONS]
#define DPAD_RIGHT(player) pressed[dpad+3 - begin + player * ACTIVE_BUTTONS]

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
XYZ(HOTKEY, DEBUG_VIEW, "d"); \
XYZ(HOTKEY, CMD_DEBUG_VIEW, "left ctrl"); \
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
XYZ(COLECO, UP, "up") \
XYZ(COLECO, DOWN, "down") \
XYZ(COLECO, LEFT, "left") \
XYZ(COLECO, RIGHT, "right") \
XYZ(COLECO, 0, "0") \
XYZ(COLECO, 1, "1") \
XYZ(COLECO, 2, "2") \
XYZ(COLECO, 3, "3") \
XYZ(COLECO, 4, "4") \
XYZ(COLECO, 5, "5") \
XYZ(COLECO, 6, "6") \
XYZ(COLECO, 7, "7") \
XYZ(COLECO, 8, "8") \
XYZ(COLECO, 9, "9") \
XYZ(COLECO, HASHTAG, "q") \
XYZ(COLECO, ASTERISK, "w") \
XYZ(COLECO, BLUE, "a") \
XYZ(COLECO, PURPLE, "s") \
XYZ(COLECO, BTN_1, "z") \
XYZ(COLECO, BTN_2, "x") \
\
XYZ(CHIP8, 1, "1"); \
XYZ(CHIP8, 2, "2"); \
XYZ(CHIP8, 3, "3"); \
XYZ(CHIP8, C, "4"); \
XYZ(CHIP8, 4, "q"); \
XYZ(CHIP8, 5, "w"); \
XYZ(CHIP8, 6, "e"); \
XYZ(CHIP8, D, "r"); \
XYZ(CHIP8, 7, "a"); \
XYZ(CHIP8, 8, "s"); \
XYZ(CHIP8, 9, "d"); \
XYZ(CHIP8, E, "f"); \
XYZ(CHIP8, A, "z"); \
XYZ(CHIP8, 0, "x"); \
XYZ(CHIP8, B, "c"); \
XYZ(CHIP8, F, "v"); \
\
XYZ(PACMAN, UP, "up"); \
XYZ(PACMAN, DOWN, "down"); \
XYZ(PACMAN, LEFT, "left"); \
XYZ(PACMAN, RIGHT, "right"); \
XYZ(PACMAN, BUTTON, "z"); \
XYZ(PACMAN, COIN, "right shift"); \
XYZ(PACMAN, START1, "1"); \
XYZ(PACMAN, START2, "2"); \
\
XYZ(SPACEINVADERS, FIRE, "z"); \
XYZ(SPACEINVADERS, LEFT, "left"); \
XYZ(SPACEINVADERS, RIGHT, "right"); \
XYZ(SPACEINVADERS, COIN, "right shift"); \
XYZ(SPACEINVADERS, START, "return"); \
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
XYZ(SEGA, UP, "up"); \
XYZ(SEGA, DOWN, "down"); \
XYZ(SEGA, LEFT, "left"); \
XYZ(SEGA, RIGHT, "right"); \
XYZ(SEGA, BTN_1, "z"); \
XYZ(SEGA, BTN_2, "x"); \
XYZ(SEGA, PAUSE, "f1"); \
XYZ(SEGA, GG_START, "return"); \
\
XYZ(SEGA, 1, "1"); \
XYZ(SEGA, Q, "q"); \
XYZ(SEGA, A, "a"); \
XYZ(SEGA, Z, "z"); \
XYZ(SEGA, ED, "right ctrl"); \
XYZ(SEGA, COMMA, ","); \
XYZ(SEGA, K, "k"); \
XYZ(SEGA, I, "i"); \
XYZ(SEGA, 8, "8"); \
XYZ(SEGA, 2, "2"); \
XYZ(SEGA, W, "w"); \
XYZ(SEGA, S, "s"); \
XYZ(SEGA, X, "x"); \
XYZ(SEGA, SPC, "space"); \
XYZ(SEGA, DOT, "."); \
XYZ(SEGA, L, "l"); \
XYZ(SEGA, O, "o"); \
XYZ(SEGA, 9, "9"); \
XYZ(SEGA, 3, "3"); \
XYZ(SEGA, E, "e"); \
XYZ(SEGA, D, "d"); \
XYZ(SEGA, C, "c"); \
XYZ(SEGA, HC, "delete"); \
XYZ(SEGA, SLASH, "/"); \
XYZ(SEGA, SEMICOLON, ";"); \
XYZ(SEGA, P, "p"); \
XYZ(SEGA, 0, "0"); \
XYZ(SEGA, 4, "4"); \
XYZ(SEGA, R, "r"); \
XYZ(SEGA, F, "f"); \
XYZ(SEGA, V, "v"); \
XYZ(SEGA, ID, "backspace"); \
XYZ(SEGA, PI, "right alt"); \
XYZ(SEGA, COLON, "\'"); \
XYZ(SEGA, AT, "\\"); \
XYZ(SEGA, MINUS, "-"); \
XYZ(SEGA, 5, "5"); \
XYZ(SEGA, T, "t"); \
XYZ(SEGA, G, "g"); \
XYZ(SEGA, B, "b"); \
XYZ(SEGA, DA, "down"); \
XYZ(SEGA, CLOSE_BRACKET, "]"); \
XYZ(SEGA, OPEN_BRACKET, "["); \
XYZ(SEGA, CARET, "="); \
XYZ(SEGA, 6, "6"); \
XYZ(SEGA, Y, "y"); \
XYZ(SEGA, H, "h"); \
XYZ(SEGA, N, "n"); \
XYZ(SEGA, LA, "left"); \
XYZ(SEGA, CR, "return"); \
XYZ(SEGA, YEN, "`"); \
XYZ(SEGA, FNC, "tab"); \
XYZ(SEGA, 7, "7"); \
XYZ(SEGA, U, "u"); \
XYZ(SEGA, J, "j"); \
XYZ(SEGA, M, "m"); \
XYZ(SEGA, RA, "right"); \
XYZ(SEGA, UA, "up"); \
XYZ(SEGA, BRK, "right shift"); \
XYZ(SEGA, GRP, "left alt"); \
XYZ(SEGA, CTL, "left ctrl"); \
XYZ(SEGA, SHF, "left shift");

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
XYZ(SEGA, UP, "dpup"); \
XYZ(SEGA, DOWN, "dpdown"); \
XYZ(SEGA, LEFT, "dpleft"); \
XYZ(SEGA, RIGHT, "dpright"); \
XYZ(SEGA, BTN_1, "a"); \
XYZ(SEGA, BTN_2, "b"); \
XYZ(SEGA, PAUSE, "none"); \
XYZ(SEGA, GG_START, "start"); \
\
XYZ(COLECO, UP, "dpup"); \
XYZ(COLECO, DOWN, "dpdown"); \
XYZ(COLECO, LEFT, "dpleft"); \
XYZ(COLECO, RIGHT, "dpright"); \
XYZ(COLECO, BTN_1, "a"); \
XYZ(COLECO, BTN_2, "b"); \
XYZ(COLECO, 1, "start"); \
XYZ(COLECO, 2, "back"); \
XYZ(COLECO, ASTERISK, "x"); \
XYZ(COLECO, HASHTAG, "z"); \
\
XYZ(PACMAN, UP, "dpup"); \
XYZ(PACMAN, DOWN, "dpdown"); \
XYZ(PACMAN, LEFT, "dpleft"); \
XYZ(PACMAN, RIGHT, "dpright"); \
XYZ(PACMAN, BUTTON, "b"); \
XYZ(PACMAN, COIN, "back"); \
XYZ(PACMAN, START1, "start"); \
\
XYZ(SPACEINVADERS, FIRE, "b"); \
XYZ(SPACEINVADERS, LEFT, "dpleft"); \
XYZ(SPACEINVADERS, RIGHT, "dpright"); \
XYZ(SPACEINVADERS, COIN, "back"); \
XYZ(SPACEINVADERS, START, "start"); \

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
    pressed = malloc(ACTIVE_BUTTONS * sizeof(bool) * MAX_PLAYERS);
    prev_pressed = malloc(ACTIVE_BUTTONS * sizeof(bool) * MAX_PLAYERS);
    memset(pressed, 0, ACTIVE_BUTTONS * sizeof(bool) * MAX_PLAYERS);
    memset(prev_pressed, 0, ACTIVE_BUTTONS * sizeof(bool) * MAX_PLAYERS);
    memset(gamepads, 0, MAX_GAMEPADS * sizeof(SDL_Gamepad*));

    disable_illegal = ini_getbool("GENERAL", "DISABLE_ILLEGAL_INPUT", true, argument_get_ini_path());

    keyboard_player = ini_getl("GENERAL", "KEYBOARD_PLAYER", 0, argument_get_ini_path());

    for(int i = 0; i < MAX_GAMEPADS; i++){
        char key[32];
        sprintf(key, "GAMEPAD_%d_PLAYER", i);
        gamepad_players[i] = ini_getl("GENERAL", key, 0, argument_get_ini_path());
    }

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

    for(int i = 0; i < MAX_GAMEPADS; i++){
        if(gamepads[i]){
            SDL_CloseGamepad(gamepads[i]);
            gamepads[i] = NULL;
        }
    }
}

static bool is_gamepad_opened(SDL_JoystickID id) {
    for (int i = 0; i < MAX_GAMEPADS; i++) {
        if (gamepads[i] && SDL_GetGamepadID(gamepads[i]) == id) {
            return true;
        }
    }
    return false;
}

bool controls_gamepad_search() {
    int num_joysticks = 0;
    SDL_JoystickID* joystick_ids = SDL_GetGamepads(&num_joysticks);
    bool changed = false;

    for(int i = 0; i < MAX_GAMEPADS; i++){
        if(gamepads[i] && !SDL_GamepadConnected(gamepads[i])){
            printf("closed %p %s\n", gamepads[i], SDL_GetGamepadName(gamepads[i]));
            SDL_CloseGamepad(gamepads[i]);
            gamepads[i] = NULL;
            changed = true;
        }
    }

    if (joystick_ids) {
        for (int j = 0; j < num_joysticks; j++) {
            if (is_gamepad_opened(joystick_ids[j]))
                continue;

            for (int i = 0; i < MAX_GAMEPADS; i++) {
                if (!gamepads[i]) {
                    gamepads[i] = SDL_OpenGamepad(joystick_ids[j]);
                    SDL_SetGamepadSensorEnabled(gamepads[i], SDL_SENSOR_ACCEL, true);
                    printf("opened slot %d: %p %s\n", i, gamepads[i], SDL_GetGamepadName(gamepads[i]));
                    changed = true;
                    break;
                }
            }
        }
        SDL_free(joystick_ids);
    }

    return changed;
}

void controls_update(){
    const bool* keystate = SDL_GetKeyboardState(NULL);

    memcpy(hotkeys_prev_pressed_arr, hotkeys_pressed_arr, HOTKEYS_COUNT * sizeof(bool));

    for(int i = 0; i < HOTKEYS_COUNT; i++){
        int cmd_idx = CONTROL_HOTKEY_CMD_BEGIN + i;
        int hotkey_idx = CONTROL_HOTKEY_BEGIN + i;
        bool active = keystate[control_scancode_maps[cmd_idx]] || (control_scancode_maps[cmd_idx] == SDL_SCANCODE_UNKNOWN);
        bool key = keystate[control_scancode_maps[hotkey_idx]];
        bool gamepad_btn = false;
        for(int g = 0; g < MAX_GAMEPADS; g++){
            if(gamepads[g] && SDL_GetGamepadButton(gamepads[g], control_gamepad_maps[hotkey_idx])){
                gamepad_btn = true;
                break;
            }
        }
        bool hit = (active && key) || gamepad_btn;
        hotkeys_pressed_arr[i] = hit;
    }

    // return if controls are not initialized
    if(!pressed || !prev_pressed)
        return;
    
    memcpy(prev_pressed, pressed, ACTIVE_BUTTONS * sizeof(bool) * MAX_PLAYERS);

    for(int i = begin; i <= end; i++){
        int idx = i - begin;
        for(int j = 0; j < MAX_PLAYERS; j++)
            pressed[idx + ACTIVE_BUTTONS * j] = false;
        
        pressed[ACTIVE_BUTTONS * keyboard_player + idx] |= keystate[control_scancode_maps[i]];

        for(int k = 0; k < MAX_GAMEPADS; k++){
            if(gamepads[k])
                pressed[ACTIVE_BUTTONS * gamepad_players[k] + idx] |= SDL_GetGamepadButton(gamepads[k], control_gamepad_maps[i]);
        }
    }

    if(disable_illegal && dpad){
        for(int i = 0; i < MAX_PLAYERS; i++){
            bool dp[4] = { DPAD_UP(i), DPAD_DOWN(i), DPAD_LEFT(i), DPAD_RIGHT(i) };
            DPAD_UP(i) &= !(dp[0] && dp[1]);
            DPAD_DOWN(i) &= !(dp[0] && dp[1]);
            DPAD_LEFT(i) &= !(dp[2] && dp[3]);
            DPAD_RIGHT(i) &= !(dp[2] && dp[3]);
        }
    }
}

bool controls_pressed(control_t control, int port){
    if(control == CONTROL_ALWAYS)
        return true;
    if(control == CONTROL_NONE)
        return false;

    if(port == CONTROLS_BOTH)
        return controls_pressed(control, 0) || controls_pressed(control, 1);

    return pressed[control - begin + ACTIVE_BUTTONS * port];
}

bool controls_released(control_t control, int port){
    if(control == CONTROL_ALWAYS)
        return true;
    if(control == CONTROL_NONE)
        return false;

    if(port == CONTROLS_BOTH)
        return controls_released(control, 0) || controls_released(control, 1);

    int idx = control - begin + ACTIVE_BUTTONS * port;
    return !pressed[idx] && prev_pressed[idx];
}

bool hotkeys_pressed(control_t control){
    return hotkeys_pressed_arr[control - CONTROL_HOTKEY_BEGIN];
}

bool hotkeys_released(control_t control){
    return !hotkeys_pressed_arr[control - CONTROL_HOTKEY_BEGIN] && hotkeys_prev_pressed_arr[control - CONTROL_HOTKEY_BEGIN];
}

bool controls_gamepad_connected(){
    for(int i = 0; i < MAX_GAMEPADS; i++)
        if(gamepads[i])
            return true;
    return false;
}

bool controls_rumble(u16 low, u16 hi, u32 duration){
    bool ret = false;
    for(int i = 0; i < MAX_GAMEPADS; i++){
        if(gamepads[i] && SDL_RumbleGamepad(gamepads[i], low, hi, duration))
            ret = true;
    }
    return ret;
}

void controls_get_gamepad_accelerometer(float* sensors){
    sensors[0] = 0.0f;
    sensors[1] = 0.0f;
    sensors[2] = 0.0f;
    for(int i = 0; i < MAX_GAMEPADS; i++){
        if(gamepads[i]){
            SDL_GetGamepadSensorData(gamepads[i], SDL_SENSOR_ACCEL, sensors, 3);
            return;
        }
    }
}

void controls_get_gamepad_info(int index, char* name, int len, int* id) {
    if (!gamepads[index]) {
        snprintf(name, len, "Empty Slot");
        *id = -1;
        return;
    }
    strncpy(name, SDL_GetGamepadName(gamepads[index]), len);
    *id = SDL_GetGamepadID(gamepads[index]);
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

void controls_set_keyboard_player(int player){
    keyboard_player = player;
}

void controls_set_gamepad_player(int gamepad_idx, int player){
    gamepad_players[gamepad_idx] = player;
}

int controls_get_gamepad_player(int gamepad_idx){
    return gamepad_players[gamepad_idx];
}
