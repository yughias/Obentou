#ifndef _SDL_MAINLOOP_H_
#define _SDL_MAINLOOP_H_

#include <SDL3/SDL.h>

#ifdef MAINLOOP_WINDOWS
#include <windows.h>
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define frameRate(x) frameRate = x

typedef int keyboard;
typedef Uint32 button;

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

extern void (*onExit)();

void setup();
void loop();
void size(int, int);
void setTitle(const char*);
void setWindowSize(int, int);
void setAspectRatio(float);
SDL_Window* getMainWindow();
SDL_Surface* getMainWindowSurface();
float millis();
void fullScreen();
void background(int);
bool isGrabbed();
int color(int, int, int);
void getRGB(int, Uint8*, Uint8*, Uint8*);
void rect(int, int, int, int, int);
void loadWindowIcon(const char*);
void setScalingMode(SDL_RendererLogicalPresentation mode);

void renderPixels();

// getter for program arguments
int getArgc();
char* getArgv(int);
char** getArgvs();

SDL_Window* createWindowWithIcon(const char* title, int w, int h, Uint32 flags);

typedef size_t menuId;
typedef size_t buttonId;
#ifdef _WIN32
menuId addMenuTo(menuId, const wchar_t*, bool);
buttonId addButtonTo(menuId, const wchar_t*, void (*callback)(void*), void*);
void setButtonTitle(buttonId, const wchar_t*);
void destroyAllMenus();
void checkRadioButton(buttonId); 
void tickButton(buttonId, bool);

#else
#define addMenuTo(a, b, c) 0
#define addButtonTo(a, b, c, d) 0 
#define setButtonTitle(a, b)
#define destroyAllMenus()
#define checkRadioButton(a); 
#define tickButton(a, b); 
#endif

#endif