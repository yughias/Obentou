#ifndef _SDL_MAINLOOP_H_
#define _SDL_MAINLOOP_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#ifdef MAINLOOP_WINDOWS
#include <windows.h>
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define frameRate(x) frameRate = x

#define LEFT SDLK_LEFT
#define RIGHT SDLK_RIGHT
#define UP SDLK_UP
#define DOWN SDLK_DOWN
#define BUTTON_LEFT SDL_BUTTON_LEFT
#define BUTTON_RIGHT SDL_BUTTON_RIGHT

typedef int keyboard;
typedef Uint32 button;

extern unsigned int displayWidth;
extern unsigned int displayHeight;
extern int width;
extern int height;
extern int* pixels;

extern float frameRate;
extern unsigned int frameCount;
extern float deltaTime;

extern int pmouseX;
extern int pmouseY;
extern int mouseX;
extern int mouseY;
extern button exitButton;

void setup();
void loop();
void size(int, int);
void setTitle(const char*);
void setWindowSize(int, int);
void setAspectRatio(float);
float millis();
void fullScreen();
void background(int);
int color(int, int, int);
void getRGB(int, Uint8*, Uint8*, Uint8*);
void rect(int, int, int, int, int);
void loadWindowIcon(const char*);

void noRender();
void autoRender();
void renderPixels();

// getter for program arguments
int getArgc();
char* getArgv(int);

typedef enum {NEAREST, LINEAR, ANISOTROPIC} ScaleMode;
typedef GLuint* Shader;
Shader loadShader(const char*);
void noGlobalShader();
void setGlobalShader(Shader);
void setScaleMode(ScaleMode);
void setVoidColor(int, int, int);
SDL_Window* createWindowWithIcon(const char* title, int x, int y, int w, int h, Uint32 flags);

typedef size_t menuId;
typedef size_t buttonId;
#ifdef MAINLOOP_WINDOWS
menuId addMenuTo(menuId, const wchar_t*, bool);
buttonId addButtonTo(menuId, const wchar_t*, void (*callback)());
void checkRadioButton(buttonId); 
void tickButton(buttonId, bool);
void getAbsoluteDir(char*); 
#else
#define addMenuTo(a, b, c) 0
#define addButtonTo(a, b, c) 0 
#define checkRadioButton(a); 
#define tickButton(a, b); 
#define getAbsoluteDir(a);
#endif

#endif