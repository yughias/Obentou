#include "SDL_MAINLOOP.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define MAX_NAME  64

int width = 800;
int height = 600;
int* pixels;

float frameRate = 60;
unsigned int frameCount = 0;
float deltaTime;

int pmouseX;
int pmouseY;
int mouseX;
int mouseY;
button exitButton = SDLK_ESCAPE;
float aspectRatio = -1.0f;

// not accessible variables
SDL_Window* window;
SDL_Surface* surface;
SDL_Surface* windowIcon;

bool running;

void (*onExit)();

#ifndef __EMSCRIPTEN__
Uint32 winFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
#else
Uint32 winFlags = SDL_WINDOW_HIDDEN;
#endif

char windowName[MAX_NAME+1];
char iconPath[MAX_NAME+1];

int main_argc;
char** main_argv;

SDL_Renderer* renderer = NULL;
SDL_Texture* drawBuffer = NULL;

ScaleMode scale_mode = NEAREST; 

void renderBufferToWindow();
bool filterResize(void*, SDL_Event*);

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#include <dwmapi.h>

char absolutePath[1024];

HWND hwnd = NULL;
HMENU mainMenu = NULL;

typedef struct {
    menuId parent_menu;
    void (*callback)();
    size_t position;
} button_t;
size_t n_button = 0;
button_t* buttons = NULL;

typedef struct {
    HMENU hMenu;
    size_t n_button;
    bool is_radio;
} menu_t;
size_t n_menu = 0;
menu_t* menus = NULL;

HWND getWindowHandler();
void createMainMenu();
void updateButtonVect(void (*callback)(), menuId);
void updateMenuVect(HMENU, bool);

#endif

void mainloop();

// variables used for run loop at correct framerate
#ifndef __EMSCRIPTEN__
Uint64 a_clock;
Uint64 b_clock;
#else
double a_clock;
double b_clock;

void emscripten_mainloop(){
    float millis_per_frame = 1000.0 / frameRate;
    float elapsed;
    b_clock = emscripten_get_now();
    elapsed = b_clock - a_clock;
    deltaTime += elapsed;
    a_clock = b_clock;
    // cap max deltaTime
    if(deltaTime >= millis_per_frame * frameRate)
        deltaTime = millis_per_frame * frameRate;
    while(deltaTime >= millis_per_frame){
        b_clock = emscripten_get_now();
        mainloop();
        elapsed = b_clock - a_clock;
        a_clock = b_clock;
        // if mainloop lasted for more than millis_per_frame
        // immediately end to avoid infinite lag!
        if(elapsed > millis_per_frame)
            deltaTime = 0;
        else 
            deltaTime -= millis_per_frame;
    }   
}
#endif

int main(int argc, char** argv){
    main_argc = argc;
    main_argv = argv;

    #ifdef _WIN32
    GetModuleFileName(NULL, absolutePath, 1024);
    PathRemoveFileSpec(absolutePath);
    #endif

    SDL_Init(
        SDL_INIT_VIDEO |
        SDL_INIT_AUDIO |
        SDL_INIT_GAMEPAD |
        SDL_INIT_JOYSTICK |
        SDL_INIT_SENSOR |
        SDL_INIT_CAMERA
    );

    SDL_SetHintWithPriority(SDL_HINT_JOYSTICK_ENHANCED_REPORTS, "1", SDL_HINT_OVERRIDE);

    strcpy(windowName, "window");
    
    #ifndef __EMSCRIPTEN__
    SDL_SetEventFilter(filterResize, NULL);
    #endif

    setup();

    #ifdef _WIN32
    if(mainMenu && !SDL_GetWindowFullscreenMode(window)){
        SetMenu(hwnd, mainMenu);
    }

    int actual_width, actual_height;
    SDL_GetWindowSize(window, &actual_width, &actual_height);
    SDL_SetWindowSize(window, actual_width, actual_height);
    #endif

    SDL_ShowWindow(window);

    deltaTime = 0;
    #ifdef __EMSCRIPTEN__
    a_clock = emscripten_get_now();
    b_clock = emscripten_get_now();
    emscripten_set_main_loop(emscripten_mainloop, 0, 1);
    #else 
    a_clock = SDL_GetPerformanceCounter();
    b_clock = SDL_GetPerformanceCounter();

    running = true;
    while(running){
        a_clock = SDL_GetPerformanceCounter();
        deltaTime = (float)(a_clock - b_clock)/SDL_GetPerformanceFrequency()*1000;

        if(deltaTime > 1000.0f / frameRate){
            mainloop();

            b_clock = a_clock;
        } else {
            float ms = 1000.0f/frameRate;
            if(ms - deltaTime > 1.0f)
                SDL_Delay(ms - deltaTime - 1);
        }
    }
    #endif

    SDL_DestroyTexture(drawBuffer);
	SDL_DestroyRenderer(renderer);

    #ifdef _WIN32
    free(buttons);
    free(menus);
    #endif

    SDL_DestroyWindow(window);

    if(onExit)
        (*onExit)();

    SDL_Quit();

    return 0;
}

void mainloop(){
    frameCount++;

    #ifdef _WIN32
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {   
        if(msg.message == WM_COMMAND){
            unsigned int button_id = LOWORD(msg.wParam); 
            if(button_id < n_button){
                checkRadioButton(button_id);
                if(buttons[button_id].callback)
                    (*buttons[button_id].callback)();
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    #endif

    pmouseX = mouseX;
    pmouseY = mouseY;
    float m_x, m_y;
    SDL_GetMouseState(&m_x, &m_y);
    SDL_RenderCoordinatesFromWindow(renderer, m_x, m_y, &m_x, &m_y);
    mouseX = m_x;
    mouseY = m_y;
    if(mouseX < 0)
        mouseX = 0;
    if(mouseY < 0)
        mouseY = 0;
    if(mouseX >= width)
        mouseX = width-1;
    if(mouseY >= height)
        mouseY = height-1;
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_EVENT_QUIT:
            running = 0;
            break;

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            Uint32 id = event.window.windowID;
            SDL_Window* target_win = SDL_GetWindowFromID(id);
            if(target_win == window)
                running = 0;
            else
                SDL_DestroyWindow(target_win);
            break;

            case SDL_EVENT_KEY_DOWN:
            if(event.key.key == exitButton)
                running = 0;
            break;
        }
    }

    loop();
}

SDL_Window* createWindowWithIcon(const char* title, int w, int h, Uint32 flags){
    SDL_Window* win = SDL_CreateWindow(title, w, h, flags);
    if(windowIcon)
        SDL_SetWindowIcon(win, windowIcon);
    return win;
}

void size(int w, int h){
    width = w;
    height = h;
    if(!window){
        width = w;
        height = h;
        window = createWindowWithIcon(windowName, width, height, winFlags);

        #ifdef _WIN32
        hwnd = getWindowHandler();
        // make menu bar and window of same color
        BOOL dark = FALSE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
        #endif

        renderer = SDL_CreateRenderer(window, NULL);
        drawBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        SDL_LockTextureToSurface(drawBuffer, NULL, &surface);

        pixels = surface->pixels;
    } else {
        SDL_DestroyTexture(drawBuffer);
        drawBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        setScaleMode(scale_mode);
        SDL_LockTextureToSurface(drawBuffer, NULL, &surface);
        pixels = surface->pixels;
    }

    setScaleMode(scale_mode);
    setAspectRatio(aspectRatio);
}

void setTitle(const char* name){
    strncpy(windowName, name, MAX_NAME);
    if(window){
        SDL_SetWindowTitle(window, windowName);
    }
}

void loadWindowIcon(const char* filename){
    windowIcon = SDL_LoadBMP(filename);
}

float millis(){
    return (float)SDL_GetPerformanceCounter()/SDL_GetPerformanceFrequency()*1000.0;
}

void fullScreen(){
    bool is_fullscreen = SDL_GetWindowFullscreenMode(window);

    #ifdef _WIN32
    if(hwnd && window && mainMenu){
        if(is_fullscreen)
            SetMenu(hwnd, mainMenu);
        else
            SetMenu(hwnd, NULL);
        SDL_SetWindowSize(window, width, height);
    }
    #endif

    SDL_SetWindowFullscreen(window, !is_fullscreen);
}

void background(int col){
    SDL_FillSurfaceRect(surface, NULL, col);
}

int color(int red, int green, int blue){
    return SDL_MapSurfaceRGB(surface, red, green, blue);
}

void getRGB(int pixel, Uint8* r, Uint8* g, Uint8* b){
        SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(surface->format), SDL_GetSurfacePalette(surface), r, g, b, NULL);
}

void rect(int x, int y, int w, int h, int col){
    SDL_Rect rect = {x, y, w, h};
    SDL_FillSurfaceRect(surface, &rect, col);
}

int getArgc(){
    return main_argc;
}

char* getArgv(int idx){
    if(idx >= main_argc)
        return NULL;
    else
        return main_argv[idx];
}

char** getArgvs(){
    return main_argv;
}

void renderPixels(){
    SDL_UnlockTexture(drawBuffer);
    renderBufferToWindow();
    SDL_LockTextureToSurface(drawBuffer, NULL, &surface);
    pixels = surface->pixels;
}

void setScaleMode(ScaleMode mode){
    scale_mode = mode;
    if(!renderer)
        return;    

    switch(mode){
        case NEAREST:
        SDL_SetTextureScaleMode(drawBuffer, SDL_SCALEMODE_NEAREST);
        break;

        case LINEAR:
        SDL_SetTextureScaleMode(drawBuffer, SDL_SCALEMODE_LINEAR);
        break;
    }
}


void setWindowSize(int w, int h){
    int pos_x, pos_y;
    int old_w, old_h;

    SDL_GetWindowPosition(window, &pos_x, &pos_y);
    SDL_GetWindowSize(window, &old_w, &old_h);

    int new_x = pos_x + (old_w - w) / 2;
    int new_y = pos_y + (old_h - h) / 2;

    SDL_SetWindowSize(window, w, h);
    SDL_SetWindowPosition(window, new_x, new_y);
}

void setAspectRatio(float ratio){
    aspectRatio = ratio;
    int render_width = width;
    int render_height = height;
    float render_ratio = (float)render_width / (float)render_height;
    if(aspectRatio > 0.0f){
        if(aspectRatio > 1.0f){
            render_width = width / render_ratio * aspectRatio + 0.5f;
        } else {
            render_height = height * render_ratio / aspectRatio + 0.5f;
        }
    }

    SDL_SetRenderLogicalPresentation(renderer, render_width, render_height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

void renderBufferToWindow(){
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, drawBuffer, NULL, NULL);
    SDL_RenderPresent(renderer);
}

bool filterResize(void* userdata, SDL_Event* event){
    if(
        event->type == SDL_EVENT_WINDOW_MOVED ||
        event->type == SDL_EVENT_WINDOW_RESIZED ||
        event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED
    ){
        if(running)
            renderBufferToWindow();
        return false;
    }

    return true;
}

#ifdef _WIN32
void getAbsoluteDir(char* dst){
    strcpy(dst, absolutePath);
    strcat(dst, "\\");
}

HWND getWindowHandler(){
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
}

void createMainMenu(){
    if(!mainMenu)
        mainMenu = CreateMenu();
}

void updateButtonVect(void (*callback)(), menuId parentMenu){
    if(!buttons)
        buttons = (button_t*)malloc(sizeof(button_t));
    else
        buttons = (button_t*)realloc(buttons, (n_button+1)*sizeof(button_t));
    buttons[n_button].callback = callback;
    buttons[n_button].parent_menu = parentMenu;
    if(parentMenu != -1){
      buttons[n_button].position = menus[parentMenu].n_button-1;  
    }
    n_button++;
}

void updateMenuVect(HMENU new_menu, bool isRadio){
    if(!menus)
        menus = (menu_t*)malloc(sizeof(menu_t));
    else
        menus = (menu_t*)realloc(menus, (n_menu+1)*sizeof(menu_t));
    menus[n_menu].hMenu = new_menu;
    menus[n_menu].n_button = 0;
    menus[n_menu].is_radio = isRadio;
    n_menu++;
}

menuId addMenuTo(menuId parentId, const wchar_t* string, bool isRadio){
    HMENU parent = NULL;
    if(parentId < n_menu)
        parent = menus[parentId].hMenu;
    if(!parent){
        createMainMenu();
        parent = mainMenu;
    }
    HMENU new_menu = CreateMenu();
    AppendMenuW(parent, MF_POPUP, (UINT_PTR) new_menu, string);
    updateMenuVect(new_menu, isRadio);
    return n_menu-1;
}

buttonId addButtonTo(menuId parentId, const wchar_t* string, void (*callback)()){
    HMENU parent = NULL;
    if(parentId < n_menu){
        parent = menus[parentId].hMenu;
        menus[parentId].n_button++;
    }
    if(!parent){
        createMainMenu();
        parent = mainMenu;
    }
    AppendMenuW(parent, MF_STRING, n_button, string);
    updateButtonVect(callback, parentId);
    return n_button-1;
}

void checkRadioButton(buttonId button_id){
    if(button_id < n_button){
        menuId menu_id = buttons[button_id].parent_menu;
        if(menu_id < n_menu && menus[menu_id].is_radio)
            CheckMenuRadioItem(menus[menu_id].hMenu, 0, menus[menu_id].n_button-1, buttons[button_id].position, MF_BYPOSITION);
    }
}

void tickButton(buttonId button_id, bool state){
    button_t* b = &buttons[button_id];
    if(button_id < n_button){
        menuId menu_id = b->parent_menu;
        if(menu_id < n_menu){
            CheckMenuItem(menus[menu_id].hMenu, b->position, MF_BYPOSITION | (state ? MF_CHECKED : MF_UNCHECKED) );
        }
    }
}
#endif