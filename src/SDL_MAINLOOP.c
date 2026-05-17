#include "SDL_MAINLOOP.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef TEST
#define main disable_main
#endif

static int last_element_clicked = -1;

#ifdef __APPLE__
#include <TargetConditionals.h>
#import <Cocoa/Cocoa.h>

@interface SdlMenuHandler : NSObject
- (void)menuClick:(id)sender;
@end

static SdlMenuHandler* macMenuHandler = nil;

@implementation SdlMenuHandler
- (void)menuClick:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    last_element_clicked = [item tag];
}
@end

void initMacMenu() {
    if(!macMenuHandler) {
        macMenuHandler = [[SdlMenuHandler alloc] init];
        [NSApp setMainMenu:[[NSMenu alloc] init]];
    }
}
#endif

#define MAX_NAME  64

typedef struct {
    menuId parent_menu;
    void (*callback)(void*);
    void* arg;
    size_t position;
} button_t;

size_t n_button = 0;
button_t* buttons = NULL;

typedef struct {
    void* hMenu;
    size_t n_button;
    bool is_radio;
} menu_t;

size_t n_menu = 0;
menu_t* menus = NULL;

void updateButtonVect(void (*callback)(void*), void* arg, menuId parentMenu) {
    buttons = (button_t*)realloc(buttons, (n_button+1)*sizeof(button_t));
    buttons[n_button].callback = callback;
    buttons[n_button].arg = arg;
    buttons[n_button].parent_menu = parentMenu;

    if(parentMenu != -1 && parentMenu < n_menu){
        buttons[n_button].position = menus[parentMenu].n_button; 
        menus[parentMenu].n_button++; 
    } else {
        buttons[n_button].position = 0;
    }
    n_button++;
}

void updateMenuVect(void* new_menu_handle, bool isRadio) {
    menus = (menu_t*)realloc(menus, (n_menu+1)*sizeof(menu_t));
    menus[n_menu].hMenu = new_menu_handle;
    menus[n_menu].n_button = 0;
    menus[n_menu].is_radio = isRadio;
    n_menu++;
}

void checkRadioButton(buttonId button_id);

#ifdef __EMSCRIPTEN__

EMSCRIPTEN_KEEPALIVE
void emscripten_handle_click(int buttonId) {
    last_element_clicked = buttonId;
}

EM_JS(void, js_init_menu_dom, (), {
    if (document.getElementById('sdl-menu-bar')) return;

    var bar = document.createElement('div');
    bar.id = 'sdl-menu-bar';
    document.body.prepend(bar); 
});

EM_JS(void, js_add_menu, (int id, int parentId, const char* namePtr), {
    var name = UTF8ToString(namePtr);
    var bar = document.getElementById('sdl-menu-bar');
    
    var item = document.createElement('div');
    item.className = 'sdl-menu-item submenu-arrow';
    item.id = 'menu-item-' + id;
    
    item.innerText = name; 
    var dropdown = document.createElement('div');
    dropdown.className = 'sdl-dropdown';
    dropdown.id = 'dropdown-' + id;
    item.appendChild(dropdown);

    if (parentId === -1) {
        bar.appendChild(item);
    } else {
        var parentDrop = document.getElementById('dropdown-' + parentId);
        if(parentDrop) {
            item.style.width = '100%'; 
            item.style.height = 'auto';
            item.style.padding = '6px 10px';
            parentDrop.appendChild(item);
        }
    }
});

EM_JS(void, js_add_button, (int id, int parentId, const char* namePtr), {
    var name = UTF8ToString(namePtr);
    
    if (parentId === -1) {
        var btn = document.createElement('button');
        btn.className = 'sdl-menu-btn';
        btn.innerText = name;
        btn.id = 'btn-' + id;
        
        btn.onclick = function(e) {
            e.stopPropagation(); 
            _emscripten_handle_click(id);
        };
        var bar = document.getElementById('sdl-menu-bar');
        bar.appendChild(btn);
        return;
    }
    
    var dropdown = document.getElementById('dropdown-' + parentId);
    
    if (dropdown) {
        var btn = document.createElement('button');
        btn.className = 'sdl-menu-btn';
        btn.innerText = name;
        btn.id = 'btn-' + id;
        
        btn.onclick = function(e) {
            e.stopPropagation(); 
            _emscripten_handle_click(id);
        };
        dropdown.appendChild(btn);
    }
});

EM_JS(void, js_set_button_title, (int id, const char* namePtr), {
    var btn = document.getElementById('btn-' + id);
    if(btn) btn.innerText = UTF8ToString(namePtr);
});

EM_JS(void, js_enable_button, (int id, bool state), {
    var btn = document.getElementById('btn-' + id);
    if(btn) btn.disabled = !state;
});

EM_JS(void, js_tick_button, (int id, bool state), {
    var btn = document.getElementById('btn-' + id);
    if(btn) {
        if(state && !btn.innerText.startsWith('✓ ')) btn.innerText = '✓ ' + btn.innerText;
        else if(!state && btn.innerText.startsWith('✓ ')) btn.innerText = btn.innerText.substring(2);
    }
});

EM_JS(void, js_set_menu_bar_visibility, (bool visible), {
    var bar = document.getElementById('sdl-menu-bar');
    var canvas = document.querySelector('canvas');
    if(bar) {
        bar.style.display = visible ? 'flex' : 'none';
        if(canvas) canvas.style.marginTop = visible ? '26px' : '0px';
    }
});

EM_JS(void, js_destroy_menus, (), {
    var bar = document.getElementById('sdl-menu-bar');
    if(bar) bar.remove();
});

EM_JS(void, js_create_widget_canvas, (int id, const char* namePtr, int w, int h), {
    var name = UTF8ToString(namePtr);
    var container = document.createElement('div');
    container.className = 'widget-container';
    container.id = 'widget-container-' + id;

    var header = document.createElement('div');
    header.className = 'widget-header';
    
    var label = document.createElement('div');
    label.className = 'widget-label';
    label.innerText = name;
    
    var closeBtn = document.createElement('div');
    closeBtn.className = 'widget-close-btn';
    closeBtn.innerText = '×';
    closeBtn.onclick = function() {
        container.remove();
        _notify_widget_closed(id); 
    };

    header.appendChild(label);
    header.appendChild(closeBtn);
    
    var canvas = document.createElement('canvas');
    canvas.className = 'widget-canvas';
    canvas.id = 'widget-' + id;
    canvas.width = w;
    canvas.height = h;
    canvas.style.width = w + 'px';
    canvas.style.height = h + 'px';
    container.style.left = (20 + (id * 20)) + "px";
    container.style.top = (60 + (id * 20)) + "px";

    var resizer = document.createElement('div');
    resizer.className = 'widget-resizer';
    
    container.appendChild(header);
    container.appendChild(canvas);
    container.appendChild(resizer);
    document.body.appendChild(container);

    var isDragging = false;
    var offsetX, offsetY;
    var isResizing = false;
    
    resizer.onmousedown = function(e) {
        isResizing = true;
        e.preventDefault();
        e.stopPropagation();
    };

    header.onmousedown = function(e) {
        isDragging = true;
        offsetX = e.clientX - container.offsetLeft;
        offsetY = e.clientY - container.offsetTop;
        var all = document.querySelectorAll('.widget-container');
        all.forEach(w => w.style.zIndex = "1000");
        container.style.zIndex = "1001";
    };

    window.addEventListener('mousemove', function(e) {
        if (isResizing) {
            container.style.width = (e.clientX - container.offsetLeft) + 'px';
            container.style.height = (e.clientY - container.offsetTop) + 'px';
        } else if (isDragging) {
            container.style.left = (e.clientX - offsetX) + 'px';
            container.style.top = (e.clientY - offsetY) + 'px';
        }
    });

    window.addEventListener('mouseup', function() {
        isResizing = false;
        isDragging = false;
    });
});

EM_JS(void, js_update_widget_canvas, (int id, int* buffer, int w, int h), {
    var canvas = document.getElementById('widget-' + id);
    if (!canvas) return;
    var ctx = canvas.getContext('2d');
    var imgData = ctx.createImageData(w, h);
    
    var data = imgData.data;
    var src = buffer >> 2;
    for (var i = 0; i < w * h; i++) {
        var pixel = HEAP32[src + i];
        var idx = i * 4;
        data[idx]     = (pixel >> 16) & 0xFF; // R
        data[idx + 1] = (pixel >> 8)  & 0xFF; // G
        data[idx + 2] =  pixel        & 0xFF; // B
        data[idx + 3] = 255;                  // Alpha
    }
    ctx.putImageData(imgData, 0, 0);
});

EM_JS(void, js_resize_widget_dom, (int id, int w, int h), {
    var canvas = document.getElementById('widget-' + id);
    if (canvas) {
        canvas.width = w;
        canvas.height = h;
    }
});

EM_JS(void, js_destroy_widget_canvas, (int id), {
    var el = document.getElementById('widget-container-' + id);
    if (el) el.remove();
});

#endif

int width = 800;
int height = 600;
int stride = 4;
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

static SDL_Window* window;
static SDL_Surface* back_surface;
static SDL_Surface* front_surface;
static SDL_Surface* windowIcon;

static bool running;
static bool has_rendered;

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

static HWND hwnd = NULL;
static HMENU mainMenu = NULL;

HWND getWindowHandler();
void createMainMenu();

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
    if(deltaTime > millis_per_frame * 2)
        deltaTime = millis_per_frame;
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

#define MAX_WIDGETS 16

typedef struct widget_t {
    bool valid;
    int width;
    int height;
    #ifndef __EMSCRIPTEN__
    SDL_Window* window;
    SDL_Texture* texture;
    SDL_Renderer* renderer;
    #else
    int* em_pixels;
    #endif
    const char* name;
    bool (*callback)(void*);
    void* data;
} widget_t;

static widget_t widgets[MAX_WIDGETS];
static widget_t* current_widget;

static void destroyWidget(int i);

#ifdef __EMSCRIPTEN__

EMSCRIPTEN_KEEPALIVE
void notify_widget_closed(int id) {
    if (id >= 0 && id < MAX_WIDGETS) {
        if (widgets[id].valid) {
            if (widgets[id].em_pixels) {
                free(widgets[id].em_pixels);
                widgets[id].em_pixels = NULL;
            }
            widgets[id].valid = false;
            printf("Widget %d (%s) removed from C array.\n", id, widgets[id].name);
        }
    }
}

#endif

int main(int argc, char** argv){
    main_argc = argc;
    main_argv = argv;

    #ifdef _WIN32
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$",  "r", stdin);
    }
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

    if(onExit)
        (*onExit)();

    SDL_DestroyTexture(drawBuffer);
	SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(back_surface);
    SDL_DestroySurface(front_surface);

    destroyAllMenus();
    destroyAllWidgets();

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}

void mainloop(){
    frameCount++;
    has_rendered = false;

    #ifdef _WIN32
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {   
        if(msg.message == WM_COMMAND){
            unsigned int button_id = LOWORD(msg.wParam); 
            if(button_id < n_button){
                last_element_clicked = button_id;
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    #endif
    if (last_element_clicked >= 0 && last_element_clicked < n_button) {
        menuId parentId = buttons[last_element_clicked].parent_menu;
        if(parentId >= 0 && parentId < n_menu && menus[parentId].is_radio) {
            checkRadioButton(last_element_clicked);
        }

        if (buttons[last_element_clicked].callback) {
            (*buttons[last_element_clicked].callback)(buttons[last_element_clicked].arg);
        }

        last_element_clicked = -1;
    }
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
            {
                Uint32 id = event.window.windowID;
                SDL_Window* target_win = SDL_GetWindowFromID(id);
                if(target_win == window)
                    running = 0;
                else
                    SDL_DestroyWindow(target_win);
                #ifndef __EMSCRIPTEN__
                for(int i = 0; i < MAX_WIDGETS; i++){
                    if(widgets[i].valid && widgets[i].window == target_win)
                        destroyWidget(i);
                }
                #endif
            }
            break;

            case SDL_EVENT_KEY_DOWN:
            if(event.key.key == exitButton)
                running = 0;
            break;
        }
    }

    loop();
    renderBufferToWindow();

    // widget loop
    int* tmp_pix = pixels;
    int tmp_w = width;
    int tmp_h = height;
    int tmp_stride = stride;
    for(int i = 0; i < MAX_WIDGETS; i++){
        if(widgets[i].valid){
            width = widgets[i].width;
            height = widgets[i].height;
            current_widget = &widgets[i];
            #ifdef __EMSCRIPTEN__
            pixels = widgets[i].em_pixels;
            memset(pixels, 0, width*height*sizeof(int));
            stride = width;
            bool res = widgets[i].callback(widgets[i].data);
            if(res)
                js_update_widget_canvas(i, pixels, width, height);
            #else
            SDL_Surface* surface;
            SDL_RenderClear(widgets[i].renderer);
            SDL_LockTextureToSurface(widgets[i].texture, NULL, &surface);
            SDL_FillSurfaceRect(surface, NULL, 0);
            stride = surface->pitch / sizeof(Uint32);
            pixels = (int*)surface->pixels;
            bool res = widgets[i].callback(widgets[i].data);
            SDL_UnlockTexture(widgets[i].texture);
            if(res){
                SDL_RenderTexture(widgets[i].renderer, widgets[i].texture, NULL, NULL);
                SDL_RenderPresent(widgets[i].renderer);
            }
            #endif
            current_widget = NULL;
        }
    }
    pixels = tmp_pix;
    width = tmp_w;
    height = tmp_h;
    stride = tmp_stride;
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
    if(width == w && height == h)
        return;
    width = w;
    height = h;

    if(current_widget) {
        current_widget->width = w;
        current_widget->height = h;
        #ifdef __EMSCRIPTEN__
        current_widget->em_pixels = (int*)realloc(current_widget->em_pixels, w * h * sizeof(int));
        int widget_id = (int)(current_widget - widgets);
        js_resize_widget_dom(widget_id, w, h);
        pixels = current_widget->em_pixels;
        stride = w;
        #else
        SDL_UnlockTexture(current_widget->texture);
        SDL_DestroyTexture(current_widget->texture);
        current_widget->texture = SDL_CreateTexture(current_widget->renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        SDL_SetTextureScaleMode(current_widget->texture, SDL_SCALEMODE_NEAREST);
        SDL_SetRenderLogicalPresentation(current_widget->renderer, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        SDL_Surface* s;
        SDL_LockTextureToSurface(current_widget->texture, NULL, &s);
        stride = s->pitch / sizeof(Uint32);
        pixels = (int*)s->pixels;
        #endif
        return;
    }

    if(!window){
        width = w;
        height = h;
        window = createWindowWithIcon(windowName, width, height, winFlags);

        #ifdef _WIN32
        hwnd = getWindowHandler();
        #endif

        renderer = SDL_CreateRenderer(window, NULL);
        drawBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        back_surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_XRGB8888);
        front_surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_XRGB8888);
        SDL_SetTextureScaleMode(drawBuffer, SDL_SCALEMODE_NEAREST);
    } else {
        SDL_DestroyTexture(drawBuffer);
        drawBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        back_surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_XRGB8888);
        front_surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_XRGB8888);
        SDL_SetTextureScaleMode(drawBuffer, SDL_SCALEMODE_NEAREST);
    }

    stride = back_surface->pitch / sizeof(Uint32);
    pixels = back_surface->pixels;

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
    SDL_FillSurfaceRect(back_surface, NULL, col);
    renderPixels();
}

int color(int red, int green, int blue){
    return SDL_MapSurfaceRGB(front_surface, red, green, blue);
}

void getRGB(int pixel, Uint8* r, Uint8* g, Uint8* b){
        SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(front_surface->format), SDL_GetSurfacePalette(front_surface), r, g, b, NULL);
}

void rect(int x, int y, int w, int h, int col){
    SDL_Rect rect = {x, y, w, h};
    SDL_FillSurfaceRect(front_surface, &rect, col);
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
    SDL_Surface* tmp = back_surface;
    back_surface = front_surface;
    front_surface = tmp;
    pixels = back_surface->pixels;
    has_rendered = true;
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
    SDL_Surface* locked_surface;
    SDL_LockTextureToSurface(drawBuffer, NULL, &locked_surface);
    SDL_BlitSurface(front_surface, NULL, locked_surface, NULL);
    SDL_UnlockTexture(drawBuffer);

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, drawBuffer, NULL, NULL);
    SDL_RenderPresent(renderer);
}

bool hasRendered(){
    return has_rendered;
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

SDL_Surface* getMainWindowSurface(){
    return front_surface;
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
#endif

#if defined(_WIN32) || defined(__EMSCRIPTEN__) || defined(__APPLE__)
menuId addMenuTo(menuId parentId, const char* string, bool isRadio){
    #ifdef _WIN32
        menu_rendered = false;
        HMENU parent = NULL;
        if(parentId < n_menu) parent = (HMENU)menus[parentId].hMenu;
        if(!parent){
            createMainMenu();
            parent = mainMenu;
        }
        wchar_t* lstring = malloc(strlen(string)*sizeof(wchar_t)+1);
        mbstowcs(lstring, string, strlen(string)+1);
        HMENU new_menu = CreateMenu();
        AppendMenuW(parent, MF_POPUP, (UINT_PTR) new_menu, lstring);
        free(lstring);
        
        updateMenuVect(new_menu, isRadio);
    #elif defined(__EMSCRIPTEN__)
        js_init_menu_dom();
        js_add_menu(n_menu, parentId, string);
        updateMenuVect(NULL, isRadio);
    #elif defined(__APPLE__)
        initMacMenu();
        NSMenu* parent = (parentId >= n_menu) ? [NSApp mainMenu] : (__bridge NSMenu*)menus[parentId].hMenu;
        
        NSMenu* newMenu = [[NSMenu alloc] initWithTitle:[NSString stringWithUTF8String:string]];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:string] action:nil keyEquivalent:@""];
        
        [item setSubmenu:newMenu];
        [parent addItem:item];
        
        updateMenuVect((__bridge void*)newMenu, isRadio);
    #endif

    updateButtonVect(NULL, NULL, parentId);
    return n_menu-1;
}

buttonId addButtonTo(menuId parentId, const char* string, void (*callback)(void*), void* arg){
    #ifdef _WIN32
        menu_rendered = false;
        HMENU parent = NULL;
        if(parentId < n_menu) parent = (HMENU)menus[parentId].hMenu;
        if(!parent){
            createMainMenu();
            parent = mainMenu;
        }
        wchar_t* lstring = malloc(strlen(string)*sizeof(wchar_t)+1);
        mbstowcs(lstring, string, strlen(string)+1);
        AppendMenuW(parent, MF_STRING, n_button, lstring);
        free(lstring);
    #elif defined(__EMSCRIPTEN__)
        js_init_menu_dom();
        js_add_button(n_button, parentId, string);
    #elif defined(__APPLE__)
        initMacMenu();
        NSMenu* parent = (parentId >= n_menu) ? [NSApp mainMenu] : (__bridge NSMenu*)menus[parentId].hMenu;
        
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:string] action:@selector(menuClick:) keyEquivalent:@""];
        [item setTarget:macMenuHandler];
        [item setTag:n_button];
        [parent addItem:item];
    #endif

    updateButtonVect(callback, arg, parentId);
    return n_button-1;
}

void destroyAllMenus(){
    #ifdef _WIN32
        if(!mainMenu) return;
        menu_rendered = false;
        DestroyMenu(mainMenu);
        mainMenu = NULL;
    #elif defined(__EMSCRIPTEN__)
        js_destroy_menus();
    #elif defined(__APPLE__)
        [NSApp setMainMenu:nil];
        macMenuHandler = nil;
    #endif

    free(buttons);
    free(menus);
    buttons = NULL;
    menus = NULL;
    n_button = 0;
    n_menu = 0;
}

void checkRadioButton(buttonId button_id){
   if(button_id >= n_button) return;

    menuId menu_id = buttons[button_id].parent_menu;
    if(menu_id >= n_menu) return;

    if(menus[menu_id].is_radio) {
        #ifdef _WIN32
            CheckMenuRadioItem(
                (HMENU)menus[menu_id].hMenu,
                0,
                menus[menu_id].n_button - 1,
                buttons[button_id].position,
                MF_BYPOSITION
            );
        #elif defined(__EMSCRIPTEN__)
            for(size_t i=0; i < n_button; i++) {
                if(buttons[i].parent_menu == menu_id) {
                    js_tick_button(i, (i == button_id));
                }
            }
        #elif defined(__APPLE__)
            for(size_t i = 0; i < n_button; i++) {
                if(buttons[i].parent_menu == menu_id) {
                    tickButton(i, (i == button_id));
                }
            }
        #endif
    }
}

void tickButton(buttonId button_id, bool state){
    if(button_id >= n_button) return;

    #ifdef _WIN32
        button_t* b = &buttons[button_id];
        menuId menu_id = b->parent_menu;
        if(menu_id >= n_menu) return;

        CheckMenuItem(
            (HMENU)menus[menu_id].hMenu,
            b->position,
            MF_BYPOSITION | (state ? MF_CHECKED : MF_UNCHECKED)
        );
    #elif defined(__EMSCRIPTEN__)
        js_tick_button(button_id, state);
    #elif defined(__APPLE__)
        button_t* b = &buttons[button_id];
        if(b->parent_menu < n_menu) {
            NSMenu* parent = (__bridge NSMenu*)menus[b->parent_menu].hMenu;
            NSMenuItem* item = [parent itemWithTag:button_id];
            [item setState:state ? NSControlStateValueOn : NSControlStateValueOff];
        }
    #endif
}

void enableButton(buttonId button_id, bool state){
    if(button_id >= n_button) return;

    #ifdef _WIN32
        button_t* b = &buttons[button_id];
        menuId menu_id = b->parent_menu;
        if(menu_id >= n_menu) return;

        EnableMenuItem(
            (HMENU)menus[menu_id].hMenu,
            b->position,
            MF_BYPOSITION | (state ? MF_ENABLED : MF_DISABLED)
        );
    #elif defined(__EMSCRIPTEN__)
        js_enable_button(button_id, state);
    #elif defined(__APPLE__)
        button_t* b = &buttons[button_id];
        if(b->parent_menu < n_menu) {
            NSMenu* parent = (__bridge NSMenu*)menus[b->parent_menu].hMenu;
            NSMenuItem* item = [parent itemWithTag:button_id];
            [item setEnabled:state];
        }
    #endif
}

void setButtonTitle(buttonId button_id, const char* string){
    if(button_id >= n_button) return;

    #ifdef _WIN32
        button_t* b = &buttons[button_id];
        menuId menu_id = b->parent_menu;
        if(menu_id >= n_menu) return;

        wchar_t* lstring = malloc(strlen(string)*sizeof(wchar_t)+1);
        mbstowcs(lstring, string, strlen(string)+1);

        ModifyMenuW(
            (HMENU)menus[menu_id].hMenu,
            b->position,
            MF_BYPOSITION | MF_STRING,
            button_id,
            lstring
        );
        free(lstring);
    #elif defined(__EMSCRIPTEN__)
        js_set_button_title(button_id, string);
    #elif defined(__APPLE__)
        button_t* b = &buttons[button_id];
        if(b->parent_menu < n_menu) {
            NSMenu* parent = (__bridge NSMenu*)menus[b->parent_menu].hMenu;
            NSMenuItem* item = [parent itemWithTag:button_id];
            [item setTitle:[NSString stringWithUTF8String:string]];
        }
    #endif
}
#endif

void createWidget(const char* name, int w, int h, bool (*callback)(void*), void* userdata){
    for(int i = 0; i < MAX_WIDGETS; i++){
        if(widgets[i].valid && !strcmp(widgets[i].name, name))
            return;
    }

    int idx = -1;
    for(int i = 0; i < MAX_WIDGETS; i++){
        if(!widgets[i].valid){
            idx = i;
            break;
        }
    }

    if(idx == -1){
        printf("No free widget slots for: %s\n", name);
        return;
    }

    widget_t* wid = &widgets[idx];
    wid->valid = true;
    wid->name = name;
    wid->width = w;
    wid->height = h;
    wid->callback = callback;
    wid->data = userdata;

    #ifdef __EMSCRIPTEN__
        wid->em_pixels = (int*)malloc(w * h * sizeof(int));
        js_create_widget_canvas(idx, name, w, h);
    #else
        wid->window = SDL_CreateWindow(name, w, h, SDL_WINDOW_RESIZABLE);
        wid->renderer = SDL_CreateRenderer(wid->window, NULL);
        wid->texture = SDL_CreateTexture(wid->renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        SDL_SetTextureScaleMode(wid->texture, SDL_SCALEMODE_NEAREST);
        SDL_SetRenderLogicalPresentation(wid->renderer, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        SDL_SetWindowMinimumSize(wid->window, SDL_max(w, 512), SDL_max(h, 512));
    #endif
}

static void destroyWidget(int i){
    widget_t* w = &widgets[i];
    if(w->valid){
        #ifdef __EMSCRIPTEN__
        js_destroy_widget_canvas(i);
        free(w->em_pixels);
        #else
        SDL_DestroyWindow(w->window);
        SDL_DestroyTexture(w->texture);
        SDL_DestroyRenderer(w->renderer);
        #endif
        memset(w, 0, sizeof(widget_t));
    }
}

void destroyAllWidgets(){
    for(int i = 0; i < MAX_WIDGETS; i++){
        destroyWidget(i);
    }
}

void gridWindows() {
    #ifndef __EMSCRIPTEN__
    const int padX = 8, padY = 64;

    int count = 1;
    SDL_Window *windows[MAX_WIDGETS + 1];

    windows[0] = getMainWindow();
    for (int i = 0; i < MAX_WIDGETS; i++) {
        if (widgets[i].valid)
            windows[count++] = widgets[i].window;
    }

    SDL_Rect display;
    SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &display);

    int maxMinW = 1, maxMinH = 1;
    for (int i = 0; i < count; i++) {
        int minW = 0, minH = 0;
        SDL_GetWindowMinimumSize(windows[i], &minW, &minH);
        if (minW > maxMinW) maxMinW = minW;
        if (minH > maxMinH) maxMinH = minH;
    }

    int cols  = SDL_clamp((int)SDL_ceil(SDL_sqrt((float)count * display.w / display.h)), 1, count);
    int rows  = (count + cols - 1) / cols;
    int cellW = SDL_max((display.w - padX * (cols + 1)) / cols, maxMinW);
    int cellH = SDL_max((display.h - padY * (rows + 1)) / rows, maxMinH);

    for (int i = 0; i < count; i++) {
        SDL_SetWindowPosition(windows[i], display.x + padX + (i % cols) * (cellW + padX),
                                          display.y + padY + (i / cols) * (cellH + padY));
        SDL_SetWindowSize(windows[i], cellW, cellH);
    }
    #endif
}