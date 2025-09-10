#include "peripherals/controls.h"

#include "SDL2/SDL.h"

#include "minIni.h"

#define ACTIVE_BUTTONS (end - begin + 1)

static SDL_Scancode control_scancode_maps[CONTROL_COUNT];
static SDL_GameControllerButton control_gamepad_maps[CONTROL_COUNT];
static control_t begin;
static control_t end;

static bool* pressed;
static bool* prev_pressed;

static SDL_GameController* controller = NULL;

#define INI_FILE "config.ini"

#define LOAD_SCANCODE(console, button, default) { \
    char name[64] = ""; \
    ini_gets(#console, "INPUT_KEY_" #button, default, name, 64, INI_FILE); \
    SDL_Scancode scancode = SDL_GetScancodeFromName(name); \
    if(scancode == SDL_SCANCODE_UNKNOWN) { \
        printf("Unknown scancode for %s: %s, using %s\n", #console "_" #button, name, default); \
        scancode = SDL_GetScancodeFromName(default); \
    } \
    control_scancode_maps[CONTROL_ ## console ## _ ## button] = scancode;\
}

#define LOAD_GAMEPAD(console, button, default) { \
    char name[64] = ""; \
    ini_gets(#console, "INPUT_GAMEPAD_" #button, default, name, 64, INI_FILE); \
    SDL_GameControllerButton pad_btn = SDL_GameControllerGetButtonFromString(name); \
    if(pad_btn == SDL_CONTROLLER_BUTTON_INVALID) { \
        printf("Unknown gamepad button for %s: %s, using %s\n", #console "_" #button, name, default); \
        pad_btn = SDL_GameControllerGetButtonFromString(default); \
    } \
    control_gamepad_maps[CONTROL_ ## console ## _ ## button] = pad_btn;\
}

static void controls_load_scancode_maps(){
    LOAD_SCANCODE(NES, UP, "up");
    LOAD_SCANCODE(NES, DOWN, "down");
    LOAD_SCANCODE(NES, LEFT, "left");
    LOAD_SCANCODE(NES, RIGHT, "right");
    LOAD_SCANCODE(NES, A, "x");
    LOAD_SCANCODE(NES, B, "z");
    LOAD_SCANCODE(NES, SELECT, "right shift");
    LOAD_SCANCODE(NES, START, "return");

    LOAD_SCANCODE(WATARA, UP, "up");
    LOAD_SCANCODE(WATARA, DOWN, "down");
    LOAD_SCANCODE(WATARA, LEFT, "left");
    LOAD_SCANCODE(WATARA, RIGHT, "right");
    LOAD_SCANCODE(WATARA, A, "x");
    LOAD_SCANCODE(WATARA, B, "z");
    LOAD_SCANCODE(WATARA, SELECT, "right shift");
    LOAD_SCANCODE(WATARA, START, "return");

    LOAD_SCANCODE(GBC, UP, "up");
    LOAD_SCANCODE(GBC, DOWN, "down");
    LOAD_SCANCODE(GBC, LEFT, "left");
    LOAD_SCANCODE(GBC, RIGHT, "right");
    LOAD_SCANCODE(GBC, A, "x");
    LOAD_SCANCODE(GBC, B, "z");
    LOAD_SCANCODE(GBC, SELECT, "right shift");
    LOAD_SCANCODE(GBC, START, "return");
    
    LOAD_SCANCODE(PV1000, UP, "up");
    LOAD_SCANCODE(PV1000, DOWN, "down");
    LOAD_SCANCODE(PV1000, LEFT, "left");
    LOAD_SCANCODE(PV1000, RIGHT, "right");
    LOAD_SCANCODE(PV1000, BTN_1, "z");
    LOAD_SCANCODE(PV1000, BTN_2, "x");
    LOAD_SCANCODE(PV1000, SELECT, "right shift");
    LOAD_SCANCODE(PV1000, START, "return");
    
    LOAD_SCANCODE(BYTEPUSHER, 1, "1");
    LOAD_SCANCODE(BYTEPUSHER, 2, "2");
    LOAD_SCANCODE(BYTEPUSHER, 3, "3");
    LOAD_SCANCODE(BYTEPUSHER, C, "4");
    LOAD_SCANCODE(BYTEPUSHER, 4, "q");
    LOAD_SCANCODE(BYTEPUSHER, 5, "w");
    LOAD_SCANCODE(BYTEPUSHER, 6, "e");
    LOAD_SCANCODE(BYTEPUSHER, D, "r");
    LOAD_SCANCODE(BYTEPUSHER, 7, "a");
    LOAD_SCANCODE(BYTEPUSHER, 8, "s");
    LOAD_SCANCODE(BYTEPUSHER, 9, "d");
    LOAD_SCANCODE(BYTEPUSHER, E, "f");
    LOAD_SCANCODE(BYTEPUSHER, A, "z");
    LOAD_SCANCODE(BYTEPUSHER, 0, "x");
    LOAD_SCANCODE(BYTEPUSHER, B, "c");
    LOAD_SCANCODE(BYTEPUSHER, F, "v");

    LOAD_SCANCODE(PCE, UP, "up");
    LOAD_SCANCODE(PCE, DOWN, "down");
    LOAD_SCANCODE(PCE, LEFT, "left");
    LOAD_SCANCODE(PCE, RIGHT, "right");
    LOAD_SCANCODE(PCE, BTN_2, "z");
    LOAD_SCANCODE(PCE, BTN_1, "x");
    LOAD_SCANCODE(PCE, SELECT, "right shift");
    LOAD_SCANCODE(PCE, START, "return");

    LOAD_SCANCODE(TMS80, UP, "up");
    LOAD_SCANCODE(TMS80, DOWN, "down");
    LOAD_SCANCODE(TMS80, LEFT, "left");
    LOAD_SCANCODE(TMS80, RIGHT, "right");
    LOAD_SCANCODE(TMS80, BTN_1, "z");
    LOAD_SCANCODE(TMS80, BTN_2, "x");
    LOAD_SCANCODE(TMS80, START_PAUSE, "f1");

    // load tms80 keyboard config
    LOAD_SCANCODE(TMS80, 1, "1");
    LOAD_SCANCODE(TMS80, Q, "q");
    LOAD_SCANCODE(TMS80, A, "a");
    LOAD_SCANCODE(TMS80, Z, "z");
    LOAD_SCANCODE(TMS80, ED, "right ctrl");
    LOAD_SCANCODE(TMS80, COMMA, ",");
    LOAD_SCANCODE(TMS80, K, "k");
    LOAD_SCANCODE(TMS80, I, "i");
    LOAD_SCANCODE(TMS80, 8, "8");
    LOAD_SCANCODE(TMS80, 2, "2");
    LOAD_SCANCODE(TMS80, W, "w");
    LOAD_SCANCODE(TMS80, S, "s");
    LOAD_SCANCODE(TMS80, X, "x");
    LOAD_SCANCODE(TMS80, SPC, "space");
    LOAD_SCANCODE(TMS80, DOT, ".");
    LOAD_SCANCODE(TMS80, L, "l");
    LOAD_SCANCODE(TMS80, O, "o");
    LOAD_SCANCODE(TMS80, 9, "9");
    LOAD_SCANCODE(TMS80, 3, "3");
    LOAD_SCANCODE(TMS80, E, "e");
    LOAD_SCANCODE(TMS80, D, "d");
    LOAD_SCANCODE(TMS80, C, "c");
    LOAD_SCANCODE(TMS80, HC, "delete");
    LOAD_SCANCODE(TMS80, SLASH, "/");
    LOAD_SCANCODE(TMS80, SEMICOLON, ";");
    LOAD_SCANCODE(TMS80, P, "p");
    LOAD_SCANCODE(TMS80, 0, "0");
    LOAD_SCANCODE(TMS80, 4, "4");
    LOAD_SCANCODE(TMS80, R, "r");
    LOAD_SCANCODE(TMS80, F, "f");
    LOAD_SCANCODE(TMS80, V, "v");
    LOAD_SCANCODE(TMS80, ID, "backspace");
    LOAD_SCANCODE(TMS80, PI, "right alt");
    LOAD_SCANCODE(TMS80, COLON, "\'");
    LOAD_SCANCODE(TMS80, AT, "\\");
    LOAD_SCANCODE(TMS80, MINUS, "-");
    LOAD_SCANCODE(TMS80, 5, "5");
    LOAD_SCANCODE(TMS80, T, "t");
    LOAD_SCANCODE(TMS80, G, "g");
    LOAD_SCANCODE(TMS80, B, "b");
    LOAD_SCANCODE(TMS80, DA, "down");
    LOAD_SCANCODE(TMS80, CLOSE_BRACKET, "]");
    LOAD_SCANCODE(TMS80, OPEN_BRACKET, "[");
    LOAD_SCANCODE(TMS80, CARET, "=");
    LOAD_SCANCODE(TMS80, 6, "6");
    LOAD_SCANCODE(TMS80, Y, "y");
    LOAD_SCANCODE(TMS80, H, "h");
    LOAD_SCANCODE(TMS80, N, "n");
    LOAD_SCANCODE(TMS80, LA, "left");
    LOAD_SCANCODE(TMS80, CR, "return");
    LOAD_SCANCODE(TMS80, YEN, "`");
    LOAD_SCANCODE(TMS80, FNC, "tab");
    LOAD_SCANCODE(TMS80, 7, "7");
    LOAD_SCANCODE(TMS80, U, "u");
    LOAD_SCANCODE(TMS80, J, "j");
    LOAD_SCANCODE(TMS80, M, "m");
    LOAD_SCANCODE(TMS80, RA, "right");
    LOAD_SCANCODE(TMS80, UA, "up");
    LOAD_SCANCODE(TMS80, BRK, "right shift");
    LOAD_SCANCODE(TMS80, GRP, "left alt");
    LOAD_SCANCODE(TMS80, CTL, "left ctrl");
    LOAD_SCANCODE(TMS80, SHF, "left shift");
}

static void controls_load_gamepad_maps(){
    LOAD_GAMEPAD(GBC, A, "b");
    LOAD_GAMEPAD(GBC, B, "a");
    LOAD_GAMEPAD(GBC, SELECT, "back");
    LOAD_GAMEPAD(GBC, START, "start");
    LOAD_GAMEPAD(GBC, UP, "dpup");
    LOAD_GAMEPAD(GBC, DOWN, "dpdown");
    LOAD_GAMEPAD(GBC, LEFT, "dpleft");
    LOAD_GAMEPAD(GBC, RIGHT, "dpright");

    LOAD_GAMEPAD(PV1000, BTN_1, "a");
    LOAD_GAMEPAD(PV1000, BTN_2, "b");
    LOAD_GAMEPAD(PV1000, SELECT, "back");
    LOAD_GAMEPAD(PV1000, START, "start");
    LOAD_GAMEPAD(PV1000, UP, "dpup");
    LOAD_GAMEPAD(PV1000, DOWN, "dpdown");
    LOAD_GAMEPAD(PV1000, LEFT, "dpleft");
    LOAD_GAMEPAD(PV1000, RIGHT, "dpright");

    LOAD_GAMEPAD(WATARA, A, "b");
    LOAD_GAMEPAD(WATARA, B, "a");
    LOAD_GAMEPAD(WATARA, SELECT, "back");
    LOAD_GAMEPAD(WATARA, START, "start");
    LOAD_GAMEPAD(WATARA, UP, "dpup");
    LOAD_GAMEPAD(WATARA, DOWN, "dpdown");
    LOAD_GAMEPAD(WATARA, LEFT, "dpleft");
    LOAD_GAMEPAD(WATARA, RIGHT, "dpright");

    LOAD_GAMEPAD(NES, A, "b");
    LOAD_GAMEPAD(NES, B, "a");
    LOAD_GAMEPAD(NES, SELECT, "back");
    LOAD_GAMEPAD(NES, START, "start");
    LOAD_GAMEPAD(NES, UP, "dpup");
    LOAD_GAMEPAD(NES, DOWN, "dpdown");
    LOAD_GAMEPAD(NES, LEFT, "dpleft");
    LOAD_GAMEPAD(NES, RIGHT, "dpright");

    LOAD_GAMEPAD(PCE, BTN_2, "a");
    LOAD_GAMEPAD(PCE, BTN_1, "b");
    LOAD_GAMEPAD(PCE, SELECT, "back");
    LOAD_GAMEPAD(PCE, START, "start");
    LOAD_GAMEPAD(PCE, UP, "dpup");
    LOAD_GAMEPAD(PCE, DOWN, "dpdown");
    LOAD_GAMEPAD(PCE, LEFT, "dpleft");
    LOAD_GAMEPAD(PCE, RIGHT, "dpright");

    LOAD_GAMEPAD(TMS80, UP, "dpup");
    LOAD_GAMEPAD(TMS80, DOWN, "dpdown");
    LOAD_GAMEPAD(TMS80, LEFT, "dpleft");
    LOAD_GAMEPAD(TMS80, RIGHT, "dpright");
    LOAD_GAMEPAD(TMS80, BTN_1, "a");
    LOAD_GAMEPAD(TMS80, BTN_2, "b");
    LOAD_GAMEPAD(TMS80, START_PAUSE, "start");
}

void controls_init(control_t begin_, control_t end_) {
    begin = begin_;
    end = end_;
    pressed = malloc(ACTIVE_BUTTONS * sizeof(bool));
    prev_pressed = malloc(ACTIVE_BUTTONS * sizeof(bool));
    memset(pressed, 0, ACTIVE_BUTTONS * sizeof(bool));
    memset(prev_pressed, 0, ACTIVE_BUTTONS * sizeof(bool));

    controls_load_gamepad_maps();
    controls_load_scancode_maps();
}

void controls_update(){
    memcpy(prev_pressed, pressed, ACTIVE_BUTTONS * sizeof(bool));

    if(!controller){
        if(SDL_NumJoysticks()){
            controller = SDL_GameControllerOpen(0);
            printf("opened %p\n", controller);
        }
    } else if(!SDL_GameControllerGetAttached(controller)){
        controller = NULL;
        printf("closed %p\n", controller);
    }

    const Uint8* keystate = SDL_GetKeyboardState(NULL);

    for(int i = begin; i <= end; i++){
        pressed[i - begin] = keystate[control_scancode_maps[i]] || SDL_GameControllerGetButton(controller, control_gamepad_maps[i]);
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