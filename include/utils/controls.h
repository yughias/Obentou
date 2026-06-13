#ifndef __CONTROLS_PERIPHERAL__
#define __CONTROLS_PERIPHERAL__

#include "types.h"

#define MAX_PLAYERS 2
#define MAX_GAMEPADS 4

#define CONTROLS_BOTH -1

#define CONTROLS_ENUM(XY) \
    XY(HOTKEY, PAUSE, BEGIN, "p", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY, TURBO, "tab", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY, REWIND, "tab", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY, RESET, "r", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY, SLOWDOWN, "-", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY, SPEEDUP, "=", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY, SAVESTATE, "s", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY, LOADSTATE, "l", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY, DEBUG_VIEW, "d", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY, OPEN, "o", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY, OPEN_BIOS, END, "b", "none", 0.0f, 0.0f, 1.0f) \
    \
    XY(HOTKEY_CMD, PAUSE, BEGIN, "left ctrl", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY_CMD, TURBO, "none", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY_CMD, REWIND, "left shift", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY_CMD, RESET, "left ctrl", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY_CMD, SLOWDOWN, "none", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY_CMD, SPEEDUP, "none", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY_CMD, SAVESTATE, "left ctrl", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY_CMD, LOADSTATE, "left ctrl", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY_CMD, DEBUG_VIEW, "left ctrl", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY_CMD, OPEN, "left ctrl", "none", 0.0f, 0.0f, 1.0f) \
    XY(HOTKEY_CMD, OPEN_BIOS, END, "left ctrl", "none", 0.0f, 0.0f, 1.0f) \
    \
    XY(GBC, A, BEGIN, "x", "b", 0.87f, 0.28f, 1.0f) \
    XY(GBC, B, "z", "a", 0.71f, 0.42f, 1.0f) \
    XY(GBC, START, "return", "start", 0.62f, 0.85f, 1.0f) \
    XY(GBC, SELECT, "right shift", "back", 0.38f, 0.85f, 1.0f) \
    XY(GBC, UP, "up", "dpup", 0.25f, 0.22f, 1.0f) \
    XY(GBC, DOWN, "down", "dpdown", 0.25f, 0.48f, 1.0f) \
    XY(GBC, LEFT, "left", "dpleft", 0.12f, 0.35f, 1.0f) \
    XY(GBC, RIGHT, END, "right", "dpright", 0.38f, 0.35f, 1.0f) \
    \
    XY(NES, A, BEGIN, "x", "b", 0.87f, 0.28f, 1.0f) \
    XY(NES, B, "z", "a", 0.71f, 0.42f, 1.0f) \
    XY(NES, SELECT, "right shift", "back", 0.38f, 0.85f, 1.0f) \
    XY(NES, START, "return", "start", 0.62f, 0.85f, 1.0f) \
    XY(NES, UP, "up", "dpup", 0.25f, 0.22f, 1.0f) \
    XY(NES, DOWN, "down", "dpdown", 0.25f, 0.48f, 1.0f) \
    XY(NES, LEFT, "left", "dpleft", 0.12f, 0.35f, 1.0f) \
    XY(NES, RIGHT, END, "right", "dpright", 0.38f, 0.35f, 1.0f) \
    \
    XY(PV1000, BTN_1, BEGIN, "z", "a", 0.87f, 0.28f, 1.0f) \
    XY(PV1000, BTN_2, "x", "b", 0.71f, 0.42f, 1.0f) \
    XY(PV1000, START, "return", "start", 0.62f, 0.85f, 1.0f) \
    XY(PV1000, SELECT, "right shift", "back", 0.38f, 0.85f, 1.0f) \
    XY(PV1000, UP, "up", "dpup", 0.25f, 0.22f, 1.0f) \
    XY(PV1000, DOWN, "down", "dpdown", 0.25f, 0.48f, 1.0f) \
    XY(PV1000, LEFT, "left", "dpleft", 0.12f, 0.35f, 1.0f) \
    XY(PV1000, RIGHT, END, "right", "dpright", 0.38f, 0.35f, 1.0f) \
    \
    XY(WATARA, A, BEGIN, "x", "b", 0.87f, 0.28f, 1.0f) \
    XY(WATARA, B, "z", "a", 0.71f, 0.42f, 1.0f) \
    XY(WATARA, START, "return", "start", 0.62f, 0.85f, 1.0f) \
    XY(WATARA, SELECT, "right shift", "back", 0.38f, 0.85f, 1.0f) \
    XY(WATARA, UP, "up", "dpup", 0.25f, 0.22f, 1.0f) \
    XY(WATARA, DOWN, "down", "dpdown", 0.25f, 0.48f, 1.0f) \
    XY(WATARA, LEFT, "left", "dpleft", 0.12f, 0.35f, 1.0f) \
    XY(WATARA, RIGHT, END, "right", "dpright", 0.38f, 0.35f, 1.0f) \
    \
    XY(PCE, BTN_1, BEGIN, "x", "b", 0.87f, 0.28f, 1.0f) \
    XY(PCE, BTN_2, "z", "a", 0.71f, 0.42f, 1.0f) \
    XY(PCE, START, "return", "start", 0.62f, 0.85f, 1.0f) \
    XY(PCE, SELECT, "right shift", "back", 0.38f, 0.85f, 1.0f) \
    XY(PCE, UP, "up", "dpup", 0.25f, 0.22f, 1.0f) \
    XY(PCE, DOWN, "down", "dpdown", 0.25f, 0.48f, 1.0f) \
    XY(PCE, LEFT, "left", "dpleft", 0.12f, 0.35f, 1.0f) \
    XY(PCE, RIGHT, END, "right", "dpright", 0.38f, 0.35f, 1.0f) \
    \
    XY(SEGA, 1, BEGIN, "1", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, Q, "q", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, A, "a", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, Z, "z", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, ED, "right ctrl", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, COMMA, ",", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, K, "k", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, I, "i", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, 8, "8", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, 2, "2", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, W, "w", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, S, "s", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, X, "x", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, SPC, "space", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, DOT, ".", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, L, "l", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, O, "o", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, 9, "9", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, 3, "3", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, E, "e", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, D, "d", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, C, "c", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, HC, "delete", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, SLASH, "/", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, SEMICOLON, "", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, P, "p", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, 0, "0", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, 4, "4", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, R, "r", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, F, "f", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, V, "v", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, ID, "backspace", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, PI, "right alt", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, COLON, "\'", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, AT, "\\", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, MINUS, "-", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, 5, "5", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, T, "t", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, G, "g", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, B, "b", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, DA, "down", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, CLOSE_BRACKET, "]", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, OPEN_BRACKET, "[", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, CARET, "=", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, 6, "6", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, Y, "y", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, H, "h", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, N, "n", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, LA, "left", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, CR, "return", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, YEN, "`", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, FNC, "tab", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, 7, "7", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, U, "u", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, J, "j", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, M, "m", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, RA, "right", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, UA, "up", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, BRK, "right shift", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, GRP, "left alt", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, CTL, "left ctrl", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, SHF, "left shift", "none", 0.0f, 0.0f, 1.0f) \
    XY(SEGA, UP, "up", "dpup", 0.25f, 0.22f, 1.0f) \
    XY(SEGA, DOWN, "down", "dpdown", 0.25f, 0.48f, 1.0f) \
    XY(SEGA, LEFT, "left", "dpleft", 0.12f, 0.35f, 1.0f) \
    XY(SEGA, RIGHT, "right", "dpright", 0.38f, 0.35f, 1.0f) \
    XY(SEGA, BTN_1, "z", "a", 0.87f, 0.28f, 1.0f) \
    XY(SEGA, BTN_2, "x", "b", 0.71f, 0.42f, 1.0f) \
    XY(SEGA, PAUSE, "f1", "none", 0.38f, 0.85f, 1.0f) \
    XY(SEGA, GG_START, END, "return", "start", 0.62f, 0.85f, 1.0f) \
    \
    XY(COLECO, UP, BEGIN, "up", "dpup", 0.27f, 0.51f, 1.0f) \
    XY(COLECO, DOWN, "down", "dpdown", 0.27f, 0.77f, 1.0f) \
    XY(COLECO, LEFT, "left", "dpleft", 0.14f, 0.64f, 1.0f) \
    XY(COLECO, RIGHT, "right", "dpright", 0.40f, 0.64f, 1.0f) \
    XY(COLECO, 1, "1", "start", 0.60f, 0.44f, 0.65f) \
    XY(COLECO, 2, "2", "back", 0.72f, 0.44f, 0.65f) \
    XY(COLECO, 3, "3", "none", 0.84f, 0.44f, 0.65f) \
    XY(COLECO, 4, "4", "none", 0.60f, 0.56f, 0.65f) \
    XY(COLECO, 5, "5", "none", 0.72f, 0.56f, 0.65f) \
    XY(COLECO, 6, "6", "none", 0.84f, 0.56f, 0.65f) \
    XY(COLECO, 7, "7", "none", 0.60f, 0.68f, 0.65f) \
    XY(COLECO, 8, "8", "none", 0.72f, 0.68f, 0.65f) \
    XY(COLECO, 9, "9", "none", 0.84f, 0.68f, 0.65f) \
    XY(COLECO, ASTERISK, "w", "x", 0.60f, 0.80f, 0.65f) \
    XY(COLECO, 0, "0", "none", 0.72f, 0.80f, 0.65f) \
    XY(COLECO, HASHTAG, "q", "z", 0.84f, 0.80f, 0.65f) \
    XY(COLECO, BLUE, "a", "none", 0.17f, 0.18f, 1.0f) \
    XY(COLECO, PURPLE, "s", "none", 0.37f, 0.18f, 1.0f) \
    XY(COLECO, BTN_1, "z", "a", 0.83f, 0.18f, 1.0f) \
    XY(COLECO, BTN_2, END, "x", "b", 0.67f, 0.28f, 1.0f) \
    \
    XY(BYTEPUSHER, 1, BEGIN, "1", "none", 0.15f, 0.20f, 1.0f) \
    XY(BYTEPUSHER, 2, "2", "none", 0.35f, 0.20f, 1.0f) \
    XY(BYTEPUSHER, 3, "3", "none", 0.65f, 0.20f, 1.0f) \
    XY(BYTEPUSHER, C, "4", "none", 0.85f, 0.20f, 1.0f) \
    XY(BYTEPUSHER, 4, "q", "none", 0.15f, 0.40f, 1.0f) \
    XY(BYTEPUSHER, 5, "w", "none", 0.35f, 0.40f, 1.0f) \
    XY(BYTEPUSHER, 6, "e", "none", 0.65f, 0.40f, 1.0f) \
    XY(BYTEPUSHER, D, "r", "none", 0.85f, 0.40f, 1.0f) \
    XY(BYTEPUSHER, 7, "a", "none", 0.15f, 0.60f, 1.0f) \
    XY(BYTEPUSHER, 8, "s", "none", 0.35f, 0.60f, 1.0f) \
    XY(BYTEPUSHER, 9, "d", "none", 0.65f, 0.60f, 1.0f) \
    XY(BYTEPUSHER, E, "f", "none", 0.85f, 0.60f, 1.0f) \
    XY(BYTEPUSHER, A, "z", "none", 0.15f, 0.80f, 1.0f) \
    XY(BYTEPUSHER, 0, "x", "none", 0.35f, 0.80f, 1.0f) \
    XY(BYTEPUSHER, B, "c", "none", 0.65f, 0.80f, 1.0f) \
    XY(BYTEPUSHER, F, END, "v", "none", 0.85f, 0.80f, 1.0f) \
    \
    XY(CHIP8, 1, BEGIN, "1", "none", 0.15f, 0.20f, 1.0f) \
    XY(CHIP8, 2, "2", "none", 0.35f, 0.20f, 1.0f) \
    XY(CHIP8, 3, "3", "none", 0.65f, 0.20f, 1.0f) \
    XY(CHIP8, C, "4", "none", 0.85f, 0.20f, 1.0f) \
    XY(CHIP8, 4, "q", "none", 0.15f, 0.40f, 1.0f) \
    XY(CHIP8, 5, "w", "none", 0.35f, 0.40f, 1.0f) \
    XY(CHIP8, 6, "e", "none", 0.65f, 0.40f, 1.0f) \
    XY(CHIP8, D, "r", "none", 0.85f, 0.40f, 1.0f) \
    XY(CHIP8, 7, "a", "none", 0.15f, 0.60f, 1.0f) \
    XY(CHIP8, 8, "s", "none", 0.35f, 0.60f, 1.0f) \
    XY(CHIP8, 9, "d", "none", 0.65f, 0.60f, 1.0f) \
    XY(CHIP8, E, "f", "none", 0.85f, 0.60f, 1.0f) \
    XY(CHIP8, A, "z", "none", 0.15f, 0.80f, 1.0f) \
    XY(CHIP8, 0, "x", "none", 0.35f, 0.80f, 1.0f) \
    XY(CHIP8, B, "c", "none", 0.65f, 0.80f, 1.0f) \
    XY(CHIP8, F, END, "v", "none", 0.85f, 0.80f, 1.0f) \
    \
    XY(PACMAN, UP, BEGIN, "up", "dpup", 0.25f, 0.22f, 1.0f) \
    XY(PACMAN, DOWN, "down", "dpdown", 0.25f, 0.48f, 1.0f) \
    XY(PACMAN, LEFT, "left", "dpleft", 0.12f, 0.35f, 1.0f) \
    XY(PACMAN, RIGHT, "right", "dpright", 0.38f, 0.35f, 1.0f) \
    XY(PACMAN, BUTTON, "z", "b", 0.75f, 0.35f, 1.0f) \
    XY(PACMAN, COIN, "right shift", "back", 0.38f, 0.85f, 1.0f) \
    XY(PACMAN, START1, "1", "start", 0.62f, 0.85f, 1.0f) \
    XY(PACMAN, START2, END, "2", "none", 0.86f, 0.85f, 1.0f) \
    \
    XY(SPACEINVADERS, FIRE, BEGIN, "z", "b", 0.75f, 0.35f, 1.0f) \
    XY(SPACEINVADERS, LEFT, "left", "dpleft", 0.12f, 0.35f, 1.0f) \
    XY(SPACEINVADERS, RIGHT, "right", "dpright", 0.38f, 0.35f, 1.0f) \
    XY(SPACEINVADERS, COIN, "right shift", "back", 0.38f, 0.85f, 1.0f) \
    XY(SPACEINVADERS, START, END, "return", "start", 0.62f, 0.85f, 1.0f) \
    \
    XY(SPECCY, UP, BEGIN, "up", "none", 0.0f, 0.0f, 0.5f) \
    XY(SPECCY, DOWN, "down", "none", 0.0f, 0.0f, 0.5f) \
    XY(SPECCY, LEFT, "left", "none", 0.0f, 0.0f, 0.5f) \
    XY(SPECCY, RIGHT, "right", "none", 0.0f, 0.0f, 0.5f) \
    XY(SPECCY, JOY_BTN, "x", "none", 0.0f, 0.0f, 0.5f) \
    XY(SPECCY, SHIFT, "left shift", "none", 0.09f, 0.70f, 0.5f) \
    XY(SPECCY, Z, "z", "none", 0.18f, 0.70f, 0.5f) \
    XY(SPECCY, X, "x", "none", 0.27f, 0.70f, 0.5f) \
    XY(SPECCY, C, "c", "none", 0.36f, 0.70f, 0.5f) \
    XY(SPECCY, V, "v", "none", 0.45f, 0.70f, 0.5f) \
    XY(SPECCY, A, "a", "none", 0.09f, 0.60f, 0.5f) \
    XY(SPECCY, S, "s", "none", 0.18f, 0.60f, 0.5f) \
    XY(SPECCY, D, "d", "none", 0.27f, 0.60f, 0.5f) \
    XY(SPECCY, F, "f", "none", 0.36f, 0.60f, 0.5f) \
    XY(SPECCY, G, "g", "none", 0.45f, 0.60f, 0.5f) \
    XY(SPECCY, Q, "q", "none", 0.09f, 0.50f, 0.5f) \
    XY(SPECCY, W, "w", "none", 0.18f, 0.50f, 0.5f) \
    XY(SPECCY, E, "e", "none", 0.27f, 0.50f, 0.5f) \
    XY(SPECCY, R, "r", "none", 0.36f, 0.50f, 0.5f) \
    XY(SPECCY, T, "t", "none", 0.45f, 0.50f, 0.5f) \
    XY(SPECCY, 1, "1", "none", 0.09f, 0.40f, 0.5f) \
    XY(SPECCY, 2, "2", "none", 0.18f, 0.40f, 0.5f) \
    XY(SPECCY, 3, "3", "none", 0.27f, 0.40f, 0.5f) \
    XY(SPECCY, 4, "4", "none", 0.36f, 0.40f, 0.5f) \
    XY(SPECCY, 5, "5", "none", 0.45f, 0.40f, 0.5f) \
    XY(SPECCY, 0, "0", "none", 0.91f, 0.40f, 0.5f) \
    XY(SPECCY, 9, "9", "none", 0.82f, 0.40f, 0.5f) \
    XY(SPECCY, 8, "8", "none", 0.73f, 0.40f, 0.5f) \
    XY(SPECCY, 7, "7", "none", 0.64f, 0.40f, 0.5f) \
    XY(SPECCY, 6, "6", "none", 0.55f, 0.40f, 0.5f) \
    XY(SPECCY, P, "p", "none", 0.91f, 0.50f, 0.5f) \
    XY(SPECCY, O, "o", "none", 0.82f, 0.50f, 0.5f) \
    XY(SPECCY, I, "i", "none", 0.73f, 0.50f, 0.5f) \
    XY(SPECCY, U, "u", "none", 0.64f, 0.50f, 0.5f) \
    XY(SPECCY, Y, "y", "none", 0.55f, 0.50f, 0.5f) \
    XY(SPECCY, ENTER, "return", "none", 0.91f, 0.60f, 0.5f) \
    XY(SPECCY, L, "l", "none", 0.82f, 0.60f, 0.5f) \
    XY(SPECCY, K, "k", "none", 0.73f, 0.60f, 0.5f) \
    XY(SPECCY, J, "j", "none", 0.64f, 0.60f, 0.5f) \
    XY(SPECCY, H, "h", "none", 0.55f, 0.60f, 0.5f) \
    XY(SPECCY, SPACE, "space", "none", 0.91f, 0.70f, 0.5f) \
    XY(SPECCY, SYM_SHIFT, "left alt", "none", 0.82f, 0.70f, 0.5f) \
    XY(SPECCY, M, "m", "none", 0.73f, 0.70f, 0.5f) \
    XY(SPECCY, N, "n", "none", 0.64f, 0.70f, 0.5f) \
    XY(SPECCY, B, "b", "none", 0.55f, 0.70f, 0.5f) \
    XY(SPECCY, DELETE, END, "backspace", "none", 0.0f, 0.0f, 0.5f) \
    \
    XY(JACE, UP, BEGIN, "up", "none", 0.0f, 0.0f, 0.5f) \
    XY(JACE, DOWN, "down", "none", 0.0f, 0.0f, 0.5f) \
    XY(JACE, LEFT, "left", "none", 0.0f, 0.0f, 0.5f) \
    XY(JACE, RIGHT, "right", "none", 0.0f, 0.0f, 0.5f) \
    XY(JACE, SHIFT, "left shift", "none", 0.09f, 0.70f, 0.5f) \
    XY(JACE, Z, "z", "none", 0.18f, 0.70f, 0.5f) \
    XY(JACE, X, "x", "none", 0.27f, 0.70f, 0.5f) \
    XY(JACE, C, "c", "none", 0.36f, 0.70f, 0.5f) \
    XY(JACE, V, "v", "none", 0.45f, 0.70f, 0.5f) \
    XY(JACE, A, "a", "none", 0.09f, 0.60f, 0.5f) \
    XY(JACE, S, "s", "none", 0.18f, 0.60f, 0.5f) \
    XY(JACE, D, "d", "none", 0.27f, 0.60f, 0.5f) \
    XY(JACE, F, "f", "none", 0.36f, 0.60f, 0.5f) \
    XY(JACE, G, "g", "none", 0.45f, 0.60f, 0.5f) \
    XY(JACE, Q, "q", "none", 0.09f, 0.50f, 0.5f) \
    XY(JACE, W, "w", "none", 0.18f, 0.50f, 0.5f) \
    XY(JACE, E, "e", "none", 0.27f, 0.50f, 0.5f) \
    XY(JACE, R, "r", "none", 0.36f, 0.50f, 0.5f) \
    XY(JACE, T, "t", "none", 0.45f, 0.50f, 0.5f) \
    XY(JACE, 1, "1", "none", 0.09f, 0.40f, 0.5f) \
    XY(JACE, 2, "2", "none", 0.18f, 0.40f, 0.5f) \
    XY(JACE, 3, "3", "none", 0.27f, 0.40f, 0.5f) \
    XY(JACE, 4, "4", "none", 0.36f, 0.40f, 0.5f) \
    XY(JACE, 5, "5", "none", 0.45f, 0.40f, 0.5f) \
    XY(JACE, 0, "0", "none", 0.91f, 0.40f, 0.5f) \
    XY(JACE, 9, "9", "none", 0.82f, 0.40f, 0.5f) \
    XY(JACE, 8, "8", "none", 0.73f, 0.40f, 0.5f) \
    XY(JACE, 7, "7", "none", 0.64f, 0.40f, 0.5f) \
    XY(JACE, 6, "6", "none", 0.55f, 0.40f, 0.5f) \
    XY(JACE, P, "p", "none", 0.91f, 0.50f, 0.5f) \
    XY(JACE, O, "o", "none", 0.82f, 0.50f, 0.5f) \
    XY(JACE, I, "i", "none", 0.73f, 0.50f, 0.5f) \
    XY(JACE, U, "u", "none", 0.64f, 0.50f, 0.5f) \
    XY(JACE, Y, "y", "none", 0.55f, 0.50f, 0.5f) \
    XY(JACE, ENTER, "return", "none", 0.91f, 0.60f, 0.5f) \
    XY(JACE, L, "l", "none", 0.82f, 0.60f, 0.5f) \
    XY(JACE, K, "k", "none", 0.73f, 0.60f, 0.5f) \
    XY(JACE, J, "j", "none", 0.64f, 0.60f, 0.5f) \
    XY(JACE, H, "h", "none", 0.55f, 0.60f, 0.5f) \
    XY(JACE, SPACE, "space", "none", 0.91f, 0.70f, 0.5f) \
    XY(JACE, SYM_SHIFT, "left alt", "none", 0.82f, 0.70f, 0.5f) \
    XY(JACE, M, "m", "none", 0.73f, 0.70f, 0.5f) \
    XY(JACE, N, "n", "none", 0.64f, 0.70f, 0.5f) \
    XY(JACE, B, "b", "none", 0.55f, 0.70f, 0.5f) \
    XY(JACE, DELETE, "backspace", "none", 0.0f, 0.0f, 0.5f) \
    XY(JACE, TAPE_PLAY_STOP, END, "f5", "none", 0.25f, 0.20f, 1.0f)

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
    XY(SPECCY, KEYBOARD_WITH_KEMPSTON, END) \
    \
    XY(JACE, DEFAULT, BEGIN, END)

    
#define GET_MACRO_ENUM(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define DECLARE_CONTROL_ENUM7(system, name, scancode, gamepad, x, y, scale) CONTROL_ ## system ## _ ## name,
#define DECLARE_CONTROL_ENUM8(system, name, val, scancode, gamepad, x, y, scale) DECLARE_CONTROL_ENUM7(system, name, scancode, gamepad, x, y, scale) CONTROL_ ## system ## _ ## val = CONTROL_ ## system ## _ ## name,
#define DECLARE_CONTROL_ENUM(...) GET_MACRO_ENUM(__VA_ARGS__, DECLARE_CONTROL_ENUM8, DECLARE_CONTROL_ENUM7)(__VA_ARGS__)

#define DECLARE_CONTROL_TYPE_ENUM2(system, name) CONTROL_TYPE_ ## system ## _ ## name,
#define DECLARE_CONTROL_TYPE_ENUM3(system, name, val) DECLARE_CONTROL_TYPE_ENUM2(system, name) CONTROL_TYPE_ ## system ## _ ## val = CONTROL_TYPE_ ## system ## _ ## name,
#define DECLARE_CONTROL_TYPE_ENUM4(system, name, val1, val2) DECLARE_CONTROL_TYPE_ENUM2(system, name) CONTROL_TYPE_ ## system ## _ ## val1 = CONTROL_TYPE_ ## system ## _ ## name, CONTROL_TYPE_ ## system ## _ ## val2 = CONTROL_TYPE_ ## system ## _ ## name,
#define DECLARE_CONTROL_TYPE_ENUM(...) GET_MACRO_ENUM(__VA_ARGS__, _, _, _, _, DECLARE_CONTROL_TYPE_ENUM4, DECLARE_CONTROL_TYPE_ENUM3, DECLARE_CONTROL_TYPE_ENUM2)(__VA_ARGS__)

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
extern const float controls_overlays[CONTROL_COUNT][3];
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
void controls_get_accelerometer(float* sensors);
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
