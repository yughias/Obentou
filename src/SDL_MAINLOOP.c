#include "SDL_MAINLOOP.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
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
    void* hButton;
    unsigned long signal_id;
    char* title;
    bool checked;
    bool enabled;
} button_t;

size_t n_button = 0;
button_t* buttons = NULL;

typedef struct {
    void* hMenu;
    size_t n_button;
    bool is_radio;
    menuId parent_menu;
    buttonId linked_button;
    char* title;
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
    buttons[n_button].hButton = NULL;
    buttons[n_button].signal_id = 0;
    buttons[n_button].title = NULL;
    buttons[n_button].checked = false;
    buttons[n_button].enabled = true;
    n_button++;
}

void updateMenuVect(void* new_menu_handle, bool isRadio) {
    menus = (menu_t*)realloc(menus, (n_menu+1)*sizeof(menu_t));
    menus[n_menu].hMenu = new_menu_handle;
    menus[n_menu].n_button = 0;
    menus[n_menu].is_radio = isRadio;
    menus[n_menu].parent_menu = (menuId)-1;
    menus[n_menu].linked_button = (buttonId)-1;
    menus[n_menu].title = NULL;
    n_menu++;
}

void checkRadioButton(buttonId button_id);
void tickButton(buttonId button_id, bool state);

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

#if defined(__linux__)
typedef struct {
    SDL_FRect rect;
    menuId menu_id;
} linux_top_hit_t;

typedef struct {
    SDL_FRect rect;
    buttonId button_id;
    menuId submenu_id;
    int depth;
} linux_item_hit_t;

static bool linux_menu_open;
static menuId linux_menu_path[16];
static int linux_menu_depth;
static linux_top_hit_t* linux_top_hits;
static size_t linux_top_hits_count;
static linux_item_hit_t* linux_item_hits;
static size_t linux_item_hits_count;

static bool linux_point_in_rect(float x, float y, const SDL_FRect* r){
    return x >= r->x && y >= r->y && x < (r->x + r->w) && y < (r->y + r->h);
}

static menuId linux_menu_by_button(buttonId button_id){
    for(menuId i = 0; i < n_menu; i++){
        if(menus[i].linked_button == button_id)
            return i;
    }
    return (menuId)-1;
}

static void linux_menu_close(void){
    linux_menu_open = false;
    linux_menu_depth = 0;
}

static bool linux_menu_handle_motion(float x, float y){
    if(!linux_menu_open || is_fullscreen)
        return false;

    for(size_t i = 0; i < linux_top_hits_count; i++){
        if(linux_point_in_rect(x, y, &linux_top_hits[i].rect)){
            linux_menu_path[0] = linux_top_hits[i].menu_id;
            linux_menu_depth = 1;
            return true;
        }
    }

    for(size_t i = 0; i < linux_item_hits_count; i++){
        if(!linux_point_in_rect(x, y, &linux_item_hits[i].rect))
            continue;
        linux_menu_depth = linux_item_hits[i].depth + 1;
        if(linux_item_hits[i].submenu_id < n_menu){
            linux_menu_path[linux_item_hits[i].depth + 1] = linux_item_hits[i].submenu_id;
            linux_menu_depth++;
        }
        return true;
    }

    return false;
}

static bool linux_menu_handle_click(float x, float y){
    if(is_fullscreen)
        return false;

    for(size_t i = 0; i < linux_top_hits_count; i++){
        if(!linux_point_in_rect(x, y, &linux_top_hits[i].rect))
            continue;
        if(linux_menu_open && linux_menu_depth > 0 && linux_menu_path[0] == linux_top_hits[i].menu_id){
            linux_menu_close();
        } else {
            linux_menu_open = true;
            linux_menu_path[0] = linux_top_hits[i].menu_id;
            linux_menu_depth = 1;
        }
        return true;
    }

    if(!linux_menu_open)
        return false;

    for(size_t i = 0; i < linux_item_hits_count; i++){
        if(!linux_point_in_rect(x, y, &linux_item_hits[i].rect))
            continue;

        button_t* b = &buttons[linux_item_hits[i].button_id];
        if(!b->enabled)
            return true;

        if(linux_item_hits[i].submenu_id < n_menu){
            linux_menu_depth = linux_item_hits[i].depth + 2;
            linux_menu_path[linux_item_hits[i].depth + 1] = linux_item_hits[i].submenu_id;
        } else {
            last_element_clicked = (int)linux_item_hits[i].button_id;
            linux_menu_close();
        }
        return true;
    }

    linux_menu_close();
    return false;
}

static void linux_push_top_hit(SDL_FRect rect, menuId menu_id){
    linux_top_hits = realloc(linux_top_hits, (linux_top_hits_count + 1) * sizeof(*linux_top_hits));
    linux_top_hits[linux_top_hits_count].rect = rect;
    linux_top_hits[linux_top_hits_count].menu_id = menu_id;
    linux_top_hits_count++;
}

static void linux_push_item_hit(SDL_FRect rect, buttonId button_id, menuId submenu_id, int depth){
    linux_item_hits = realloc(linux_item_hits, (linux_item_hits_count + 1) * sizeof(*linux_item_hits));
    linux_item_hits[linux_item_hits_count].rect = rect;
    linux_item_hits[linux_item_hits_count].button_id = button_id;
    linux_item_hits[linux_item_hits_count].submenu_id = submenu_id;
    linux_item_hits[linux_item_hits_count].depth = depth;
    linux_item_hits_count++;
}

static float linux_text_width(const char* s){
    if(!s) return 0.0f;
    return (float)(strlen(s) * 8);
}

static void linux_menu_draw_popup(menuId menu_id, float x, float y, int depth){
    if(menu_id >= n_menu)
        return;

    const float item_h = 20.0f;
    const float pad = 8.0f;
    size_t item_count = 0;
    float w = 120.0f;

    for(buttonId i = 0; i < n_button; i++){
        if(buttons[i].parent_menu != menu_id)
            continue;
        float tw = linux_text_width(buttons[i].title ? buttons[i].title : "");
        if(tw + pad * 2 + 12 > w)
            w = tw + pad * 2 + 12;
        item_count++;
    }

    if(!item_count)
        return;

    SDL_FRect popup = {x, y, w, item_h * (float)item_count};
    SDL_SetRenderDrawColor(renderer, 35, 35, 38, 240);
    SDL_RenderFillRect(renderer, &popup);
    SDL_SetRenderDrawColor(renderer, 95, 95, 100, 255);
    SDL_RenderRect(renderer, &popup);

    size_t idx = 0;
    for(buttonId i = 0; i < n_button; i++){
        if(buttons[i].parent_menu != menu_id)
            continue;

        button_t* b = &buttons[i];
        SDL_FRect item_rect = {x, y + item_h * (float)idx, w, item_h};
        menuId sub = linux_menu_by_button(i);
        bool selected = (depth + 1 < linux_menu_depth && linux_menu_path[depth + 1] == sub);

        if(selected){
            SDL_SetRenderDrawColor(renderer, 70, 70, 76, 255);
            SDL_RenderFillRect(renderer, &item_rect);
        }

        const char* title = b->title ? b->title : "";
        char line[512];
        snprintf(line, sizeof(line), "%s%s%s", b->checked ? "\xE2\x9C\x93 " : "", title, (sub < n_menu) ? " >" : "");

        if(b->enabled){
            SDL_SetRenderDrawColor(renderer, 225, 225, 225, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 140, 140, 140, 255);
        }
        SDL_RenderDebugText(renderer, item_rect.x + 4, item_rect.y + 5, line);

        linux_push_item_hit(item_rect, i, sub, depth);

        if(selected && sub < n_menu){
            linux_menu_draw_popup(sub, item_rect.x + item_rect.w, item_rect.y, depth + 1);
        }
        idx++;
    }
}

static void linux_menu_render(void){
    linux_top_hits_count = 0;
    linux_item_hits_count = 0;

    if(is_fullscreen)
        return;

    const float bar_h = 24.0f;
    const float item_pad = 10.0f;
    float x = 0.0f;

    SDL_SetRenderDrawColor(renderer, 25, 25, 28, 255);
    SDL_FRect bar = {0, 0, (float)width, bar_h};
    SDL_RenderFillRect(renderer, &bar);
    SDL_SetRenderDrawColor(renderer, 70, 70, 75, 255);
    SDL_RenderLine(renderer, 0, bar_h - 1, (float)width, bar_h - 1);

    for(menuId i = 0; i < n_menu; i++){
        if(menus[i].parent_menu < n_menu)
            continue;

        const char* title = menus[i].title ? menus[i].title : "";
        float w = linux_text_width(title) + item_pad * 2;
        SDL_FRect rect = {x, 0, w, bar_h};
        linux_push_top_hit(rect, i);

        if(linux_menu_open && linux_menu_depth > 0 && linux_menu_path[0] == i){
            SDL_SetRenderDrawColor(renderer, 60, 60, 66, 255);
            SDL_RenderFillRect(renderer, &rect);
        }

        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        SDL_RenderDebugText(renderer, x + item_pad, 6, title);
        x += w;
    }

    if(linux_menu_open && linux_menu_depth > 0){
        linux_menu_draw_popup(linux_menu_path[0], 0.0f, bar_h, 0);
    }
}
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
            if(event.key.key == exitButton){
                #if defined(__linux__)
                if(linux_menu_open){
                    linux_menu_close();
                    break;
                }
                #endif
                running = 0;
            }
            break;

            #if defined(__linux__)
            case SDL_EVENT_MOUSE_MOTION:
            linux_menu_handle_motion(event.motion.x, event.motion.y);
            break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if(event.button.button == SDL_BUTTON_LEFT){
                linux_menu_handle_click(event.button.x, event.button.y);
            }
            break;
            #endif
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
        SDL_SetRenderLogicalPresentation(current_widget->renderer, w, h, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
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
    #if defined(__linux__)
    if(is_fullscreen)
        linux_menu_close();
    #endif
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
    #if defined(__linux__)
    linux_menu_render();
    #endif
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

#if defined(_WIN32) || defined(__EMSCRIPTEN__) || defined(__APPLE__) || defined(__linux__)
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
    #elif defined(__linux__)
        updateMenuVect(NULL, isRadio);
    #endif

    updateButtonVect(NULL, NULL, parentId);
    menuId created_menu = n_menu - 1;
    buttonId linked_button = n_button - 1;
    menus[created_menu].parent_menu = parentId;
    menus[created_menu].linked_button = linked_button;
    size_t menu_title_len = strlen(string);
    menus[created_menu].title = malloc(menu_title_len + 1);
    if(menus[created_menu].title)
        memcpy(menus[created_menu].title, string, menu_title_len + 1);
    buttons[linked_button].title = malloc(menu_title_len + 1);
    if(buttons[linked_button].title)
        memcpy(buttons[linked_button].title, string, menu_title_len + 1);
    return n_menu-1;
}

buttonId addButtonTo(menuId parentId, const char* string, void (*callback)(void*), void* arg){
    void* button_handle = NULL;
    unsigned long signal_id = 0;
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
    #elif defined(__linux__)
        (void)button_handle;
        (void)signal_id;
    #endif

    updateButtonVect(callback, arg, parentId);
    buttons[n_button - 1].hButton = button_handle;
    buttons[n_button - 1].signal_id = signal_id;
    size_t len = strlen(string);
    buttons[n_button - 1].title = (char*)malloc(len + 1);
    if(buttons[n_button - 1].title){
        memcpy(buttons[n_button - 1].title, string, len + 1);
    }
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
    #elif defined(__linux__)
        linux_menu_close();
        free(linux_top_hits);
        free(linux_item_hits);
        linux_top_hits = NULL;
        linux_item_hits = NULL;
        linux_top_hits_count = 0;
        linux_item_hits_count = 0;
    #endif

    for(size_t i = 0; i < n_button; i++){
        free(buttons[i].title);
    }
    for(size_t i = 0; i < n_menu; i++){
        free(menus[i].title);
    }
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
        #elif defined(__linux__)
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
    #elif defined(__linux__)
        buttons[button_id].checked = state;
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
    #elif defined(__linux__)
        buttons[button_id].enabled = state;
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
    #elif defined(__linux__)
        button_t* b = &buttons[button_id];
        size_t len = strlen(string);
        char* new_title = (char*)malloc(len + 1);
        if(new_title){
            memcpy(new_title, string, len + 1);
            free(b->title);
            b->title = new_title;
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
        SDL_SetRenderLogicalPresentation(wid->renderer, w, h, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
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