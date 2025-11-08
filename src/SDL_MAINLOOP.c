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

static SDL_AtomicInt is_grabbed;
static SDL_RendererLogicalPresentation scaling_mode = SDL_LOGICAL_PRESENTATION_LETTERBOX;

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

void renderBufferToWindow();
bool filterResize(void*, SDL_Event*);

static bool is_fullscreen;

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#include <dwmapi.h>

static bool menu_rendered;
static int menu_height;

HWND hwnd = NULL;
HMENU mainMenu = NULL;

typedef struct {
    menuId parent_menu;
    void (*callback)(void*);
    void* arg;
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
void updateButtonVect(void (*callback)(), void*, menuId);
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

    #ifdef _WIN32
    menu_height = GetSystemMetrics(SM_CYMENU);
    #endif

    setup();

    SDL_ShowWindow(window);

    deltaTime = 0;
    #ifdef __EMSCRIPTEN__
    a_clock = emscripten_get_now();
    b_clock = emscripten_get_now();
    emscripten_set_main_loop(emscripten_mainloop, 0, 1);
    #else 
    a_clock = SDL_GetTicksNS();
    b_clock = SDL_GetTicksNS();

    running = true;

    while (running) {
        Uint64 targetFrameTime = (Uint64)(1e9 / frameRate);
        a_clock = SDL_GetTicksNS();
        Uint64 deltaNS = a_clock - b_clock;

        if (deltaNS >= targetFrameTime) {
            deltaTime = deltaNS / 1e6f;
            mainloop();

            b_clock = a_clock;
        } else {
            Uint64 remaining = targetFrameTime - deltaNS;
            if(remaining > 1e6)
                SDL_DelayPrecise(remaining - 1e6);
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
                    (*buttons[button_id].callback)(buttons[button_id].arg);
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
    #if _WIN32
    if(!is_fullscreen){
        if(!menu_rendered){
            SetMenu(hwnd, mainMenu);
            menu_rendered = true;
        }
    } else {
        if(menu_rendered && m_y > 0){
            SetMenu(hwnd, NULL);
            menu_rendered = false;
        } else if(!menu_rendered && m_y <= menu_height){
            SetMenu(hwnd, mainMenu);
            menu_rendered = true;
        }
    }
    #endif
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
    SDL_SetAtomicInt(&is_grabbed, 0);
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_EVENT_QUIT:
            SDL_SetAtomicInt(&is_grabbed, 1);
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

SDL_Window* getMainWindow(){
    return window;
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
        #endif

        renderer = SDL_CreateRenderer(window, NULL);
        drawBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        SDL_SetTextureScaleMode(drawBuffer, SDL_SCALEMODE_NEAREST);
        SDL_LockTextureToSurface(drawBuffer, NULL, &surface);

        pixels = surface->pixels;
    } else {
        SDL_DestroyTexture(drawBuffer);
        drawBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        SDL_SetTextureScaleMode(drawBuffer, SDL_SCALEMODE_NEAREST);
        SDL_LockTextureToSurface(drawBuffer, NULL, &surface);
        pixels = surface->pixels;
    }

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
    is_fullscreen ^= 1; 

    SDL_SetWindowFullscreen(window, is_fullscreen);
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

    SDL_SetRenderLogicalPresentation(renderer, render_width, render_height, scaling_mode);
}

void setScalingMode(SDL_RendererLogicalPresentation mode){
    scaling_mode = mode;
    if(!renderer)
        return;
    int w, h;
    SDL_GetRenderLogicalPresentation(renderer, &w, &h, NULL);
    SDL_SetRenderLogicalPresentation(renderer, w, h, scaling_mode);
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
        event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || 
        event->type == SDL_EVENT_WINDOW_EXPOSED
    ){
        SDL_SetAtomicInt(&is_grabbed, 1);
        if(running)
            renderBufferToWindow();
        return false;
    }

    return true;
}

bool isGrabbed(){
    return SDL_GetAtomicInt(&is_grabbed);
}

#ifdef _WIN32
HWND getWindowHandler(){
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
}

void createMainMenu(){
    if(!mainMenu)
        mainMenu = CreateMenu();
}

void updateButtonVect(void (*callback)(), void* arg, menuId parentMenu){
    buttons = (button_t*)realloc(buttons, (n_button+1)*sizeof(button_t));

    buttons[n_button].callback = callback;
    buttons[n_button].arg = arg;
    buttons[n_button].parent_menu = parentMenu;

    if(parentMenu != -1){
        buttons[n_button].position = menus[parentMenu].n_button; 
        menus[parentMenu].n_button++; 
    } else {
        buttons[n_button].position = 0;
    }

    n_button++;
}

void updateMenuVect(HMENU new_menu, bool isRadio){
    menus = (menu_t*)realloc(menus, (n_menu+1)*sizeof(menu_t));
    menus[n_menu].hMenu = new_menu;
    menus[n_menu].n_button = 0;
    menus[n_menu].is_radio = isRadio;
    n_menu++;
}

menuId addMenuTo(menuId parentId, const wchar_t* string, bool isRadio){
    menu_rendered = false;
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
    updateButtonVect(NULL, NULL, parentId);
    return n_menu-1;
}

buttonId addButtonTo(menuId parentId, const wchar_t* string, void (*callback)(), void* arg){
    menu_rendered = false;
    HMENU parent = NULL;

    if(parentId < n_menu){
        parent = menus[parentId].hMenu;
    }

    if(!parent){
        createMainMenu();
        parent = mainMenu;
    }

    AppendMenuW(parent, MF_STRING, n_button, string);
    updateButtonVect(callback, arg, parentId);

    return n_button-1;
}

void destroyAllMenus(){
    if(!mainMenu)
        return;
    menu_rendered = false;
    DestroyMenu(mainMenu);
    free(buttons);
    free(menus);
    mainMenu = NULL;
    buttons = NULL;
    menus = NULL;
    n_button = 0;
    n_menu = 0;
}

void checkRadioButton(buttonId button_id){
   if(button_id >= n_button) return;

    menuId menu_id = buttons[button_id].parent_menu;
    if(menu_id >= n_menu) return;

    if(menus[menu_id].is_radio)
        CheckMenuRadioItem(
            menus[menu_id].hMenu,             // menu handle
            0,                                // first item
            menus[menu_id].n_button - 1,      // last item
            buttons[button_id].position,      // position within the menu
            MF_BYPOSITION
        );
}

void tickButton(buttonId button_id, bool state){
    if(button_id >= n_button) return;

    button_t* b = &buttons[button_id];
    menuId menu_id = b->parent_menu;

    if(menu_id >= n_menu) return;

    CheckMenuItem(
        menus[menu_id].hMenu,
        b->position,
        MF_BYPOSITION | (state ? MF_CHECKED : MF_UNCHECKED)
    );
}

void enableButton(buttonId button_id, bool state){
    if(button_id >= n_button) return;

    button_t* b = &buttons[button_id];
    menuId menu_id = b->parent_menu;

    if(menu_id >= n_menu) return;

    EnableMenuItem(
        menus[menu_id].hMenu,
        b->position,
        MF_BYPOSITION | (state ? MF_ENABLED : MF_DISABLED)
    );
}

void setButtonTitle(buttonId button_id, const wchar_t* string){
    if(button_id >= n_button) return;

    button_t* b = &buttons[button_id];
    menuId menu_id = b->parent_menu;

    if(menu_id >= n_menu) return;

    ModifyMenuW(
        menus[menu_id].hMenu,
        b->position,
        MF_BYPOSITION | MF_STRING,
        button_id,
        string
    );
}
#endif