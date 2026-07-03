#include "utils/controls.h"
#include "utils/argument.h"
#include "utils/overlay.h"
#include "core.h"

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
static control_type_t begin_type;
static control_type_t end_type;
static control_type_t actual_type;
static SDL_Sensor* accelerometer;

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

#define DECLARE_CONTROL_NAME(system, name, ...) [ CONTROL_ ## system ## _ ## name ] = #name,

const char controls_names[CONTROL_COUNT][32] = {
    [CONTROL_NONE] = "None",
    CONTROLS_ENUM(DECLARE_CONTROL_NAME)
};

#define DECLARE_CONTROL_TYPE_NAME(system, name, ...) [ CONTROL_TYPE_ ## system ## _ ## name ] = #name,

const char controls_type_names[CONTROL_TYPE_COUNT][32] = {
    CONTROLS_TYPE_ENUM(DECLARE_CONTROL_TYPE_NAME)
};

#define DECLARE_CONTROL_OVERLAY7(console, button, keyboard, gamepad, pos_x, pos_y, scale) [ CONTROL_ ## console ## _ ## button ] = {pos_x, pos_y, scale},
#define DECLARE_CONTROL_OVERLAY8(console, button, val, keyboard, gamepad, pos_x, pos_y, scale) DECLARE_CONTROL_OVERLAY7(console, button, keyboard, gamepad, pos_x, pos_y, scale)
#define DECLARE_CONTROL_OVERLAY(...) GET_MACRO_ENUM(__VA_ARGS__, DECLARE_CONTROL_OVERLAY8, DECLARE_CONTROL_OVERLAY7, _, _, _)(__VA_ARGS__)

const float controls_overlays[CONTROL_COUNT][3] = {
    CONTROLS_ENUM(DECLARE_CONTROL_OVERLAY)
};

#define LOAD_SCANCODE7(console, button, keyboard, gamepad, pos_x, pos_y, scale) { \
    char name[64] = ""; \
    ini_gets(#console, "INPUT_KEY_" #button, keyboard, name, 64, argument_get_ini_path()); \
    SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN; \
    if(strcasecmp(name, "none")) { \
        scancode = SDL_GetScancodeFromName(name); \
        if(scancode == SDL_SCANCODE_UNKNOWN) { \
            printf("Unknown scancode for %s: %s\n", #console "_" #button, name); \
            scancode = SDL_GetScancodeFromName(keyboard); \
        } \
    } \
    control_scancode_maps[CONTROL_ ## console ## _ ## button] = scancode; \
}
#define LOAD_SCANCODE8(console, button, val, keyboard, gamepad, pos_x, pos_y, scale) LOAD_SCANCODE7(console, button, keyboard, gamepad, pos_x, pos_y, scale)
#define LOAD_SCANCODE(...) GET_MACRO_ENUM(__VA_ARGS__, LOAD_SCANCODE8, LOAD_SCANCODE7, _, _, _)(__VA_ARGS__)

#define SAVE_SCANCODE7(console, button, keyboard, gamepad, pos_x, pos_y, scale) { \
    const char* name = SDL_GetScancodeName(control_scancode_maps[CONTROL_ ## console ## _ ## button]); \
    ini_puts(#console, "INPUT_KEY_" #button, name && name[0] ? name : "none", argument_get_ini_path()); \
}
#define SAVE_SCANCODE8(console, button, val, keyboard, gamepad, pos_x, pos_y, scale) SAVE_SCANCODE7(console, button, keyboard, gamepad, pos_x, pos_y, scale)
#define SAVE_SCANCODE(...) GET_MACRO_ENUM(__VA_ARGS__, SAVE_SCANCODE8, SAVE_SCANCODE7, _, _, _)(__VA_ARGS__)

#define LOAD_GAMEPAD7(console, button, keyboard, gamepad, pos_x, pos_y, scale) { \
    char name[64] = ""; \
    ini_gets(#console, "INPUT_GAMEPAD_" #button, gamepad, name, 64, argument_get_ini_path()); \
    SDL_GamepadButton pad_btn = SDL_GAMEPAD_BUTTON_INVALID; \
    if(strcasecmp(name, "none")) { \
        pad_btn = SDL_GetGamepadButtonFromString(name); \
        if(pad_btn == SDL_GAMEPAD_BUTTON_INVALID) { \
            printf("Unknown gamepad button for %s: %s\n", #console "_" #button, name); \
        } \
    } \
    control_gamepad_maps[CONTROL_ ## console ## _ ## button] = pad_btn; \
}
#define LOAD_GAMEPAD8(console, button, val, keyboard, gamepad, pos_x, pos_y, scale) LOAD_GAMEPAD7(console, button, keyboard, gamepad, pos_x, pos_y, scale)
#define LOAD_GAMEPAD(...) GET_MACRO_ENUM(__VA_ARGS__, LOAD_GAMEPAD8, LOAD_GAMEPAD7, _, _, _)(__VA_ARGS__)

#define SAVE_GAMEPAD7(console, button, keyboard, gamepad, pos_x, pos_y, scale) { \
    const char* name = SDL_GetGamepadStringForButton(control_gamepad_maps[CONTROL_ ## console ## _ ## button]); \
    ini_puts(#console, "INPUT_GAMEPAD_" #button, name && name[0] ? name : "none", argument_get_ini_path()); \
}
#define SAVE_GAMEPAD8(console, button, val, keyboard, gamepad, pos_x, pos_y, scale) SAVE_GAMEPAD7(console, button, keyboard, gamepad, pos_x, pos_y, scale)
#define SAVE_GAMEPAD(...) GET_MACRO_ENUM(__VA_ARGS__, SAVE_GAMEPAD8, SAVE_GAMEPAD7, _, _, _)(__VA_ARGS__)

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
    CONTROLS_ENUM(LOAD_SCANCODE);
    CONTROLS_ENUM(LOAD_GAMEPAD);
}

void controls_save_maps(){
    CONTROLS_ENUM(SAVE_SCANCODE);
    CONTROLS_ENUM(SAVE_GAMEPAD);
}

void controls_init(const core_t* core) {
    controls_free();
    begin = core->control_begin;
    end = core->control_end;
    begin_type = core->control_type_begin;
    end_type = core->control_type_end;
    pressed = malloc(ACTIVE_BUTTONS * sizeof(bool) * MAX_PLAYERS);
    prev_pressed = malloc(ACTIVE_BUTTONS * sizeof(bool) * MAX_PLAYERS);
    memset(pressed, 0, ACTIVE_BUTTONS * sizeof(bool) * MAX_PLAYERS);
    memset(prev_pressed, 0, ACTIVE_BUTTONS * sizeof(bool) * MAX_PLAYERS);
    memset(gamepads, 0, MAX_GAMEPADS * sizeof(SDL_Gamepad*));

    disable_illegal = ini_getbool("GENERAL", "DISABLE_ILLEGAL_INPUT", true, argument_get_ini_path());
    actual_type = ini_getl(core->name, "CONTROL_TYPE", begin_type, argument_get_ini_path());
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

    int n_sensors = 0;
    SDL_SensorID* sensors = SDL_GetSensors(&n_sensors);
    for (int i = 0; i < n_sensors; i++) {
        if (SDL_GetSensorTypeForID(sensors[i]) == SDL_SENSOR_ACCEL)
            accelerometer = SDL_OpenSensor(sensors[i]);
    }
    SDL_free(sensors);
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

    if (accelerometer)
        SDL_CloseSensor(accelerometer);
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

        pressed[idx] |= overlay_pressed(i);

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

void controls_get_accelerometer(float* sensors){
    sensors[0] = 0.0f;
    sensors[1] = 0.0f;
    sensors[2] = 0.0f;
    for(int i = 0; i < MAX_GAMEPADS; i++){
        if(gamepads[i]){
            SDL_GetGamepadSensorData(gamepads[i], SDL_SENSOR_ACCEL, sensors, 3);
            return;
        }
    }

    if (accelerometer) {
        float accel[3];
        SDL_GetSensorData(accelerometer, accel, 3);
        sensors[0] += accel[0];
        sensors[1] += accel[1];
        sensors[2] += accel[2];
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

control_type_t controls_get_actual_type(){
    return actual_type;
}

void controls_set_type(const char* name, control_type_t type){
    if(type >= begin_type && type <= end_type)
        actual_type = type;
    ini_putl(name, "CONTROL_TYPE", type, argument_get_ini_path());
}