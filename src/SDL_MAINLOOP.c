#include "SDL_MAINLOOP.h"
#undef main

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define MAX_NAME  64

unsigned int displayWidth;
unsigned int displayHeight;
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
float aspectRatio;

// not accessible variables
SDL_Window* window;
SDL_Surface* surface;
SDL_Surface* windowIcon;

bool running = false;

#ifndef __EMSCRIPTEN__
Uint32 winFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
#else
Uint32 winFlags = SDL_WINDOW_HIDDEN;
#endif

char windowName[MAX_NAME+1];
char iconPath[MAX_NAME+1];

int main_argc;
char** main_argv;

bool render_every_frame = true;

SDL_Renderer* renderer = NULL;
SDL_Texture* drawBuffer = NULL;
GLuint globalShader = 0;

int render_width;
int render_height;
int win_width;
int win_height;
float render_ratio;
float win_ratio;
float localX;
float localY;
ScaleMode scale_mode = ANISOTROPIC; 

typedef struct shader_list_t {
    GLuint id;
    struct shader_list_t* next;
} shader_list_t;
shader_list_t* shader_list = NULL;

PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLVALIDATEPROGRAMPROC glValidateProgram;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM2FPROC glUniform2f;

// utility function for shader loading
GLuint compileProgram(const char*);
GLuint compileShader(const char*, GLuint);

void calculateRescaleVars();
void renderBufferToWindow();
int filterResize(void*, SDL_Event*);

#ifdef MAINLOOP_WINDOWS
#include <SDL2/SDL_syswm.h>
#include <windows.h>
#include <shlwapi.h>

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

int main(int argc, char* argv[]){
    main_argc = argc;
    main_argv = argv;

    #ifdef MAINLOOP_WINDOWS
    GetModuleFileName(NULL, absolutePath, 1024);
    PathRemoveFileSpec(absolutePath);
    #endif

    SDL_Init(
        SDL_INIT_VIDEO |
        SDL_INIT_AUDIO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_GAMECONTROLLER |
        SDL_INIT_SENSOR
        #ifndef __EMSCRIPTEN__
        | SDL_VIDEO_OPENGL
        #endif
    );

    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    displayWidth = displayMode.w;
    displayHeight = displayMode.h;
    strcpy(windowName, "window");

    setup();

    #ifdef MAINLOOP_WINDOWS
    hwnd = getWindowHandler();

    if(mainMenu && !(winFlags & SDL_WINDOW_FULLSCREEN_DESKTOP))
        SetMenu(hwnd, mainMenu);

    SDL_SetWindowSize(window, width, height);
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
    };
    #endif

    SDL_DestroyTexture(drawBuffer);
	SDL_DestroyRenderer(renderer);
    
    while(shader_list){
        shader_list_t* tmp = shader_list->next;
        free(shader_list);
        shader_list = tmp;
    }

    #ifdef MAINLOOP_WINDOWS
    free(buttons);
    free(menus);
    #endif

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void mainloop(){
    frameCount++;

    #ifdef MAINLOOP_WINDOWS
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
    SDL_GetMouseState(&mouseX, &mouseY);
    #ifndef __EMSCRIPTEN__
    mouseX -= localX;
    mouseY -= localY;
    mouseX *= (float)width/render_width;
    mouseY *= (float)height/render_height;
    #endif
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
            case SDL_WINDOWEVENT:
            if(event.window.event == SDL_WINDOWEVENT_CLOSE){
                Uint32 id = event.window.windowID;
                SDL_Window* target_win = SDL_GetWindowFromID(id);
                if(target_win == window)
                    running = 0;
                else
                    SDL_DestroyWindow(target_win);
            }
            break;

            case SDL_KEYDOWN:
            if(event.key.keysym.sym == exitButton)
                running = 0;
            break;
        }
    }

    loop();

    if(render_every_frame)
        renderPixels();
}

SDL_Window* createWindowWithIcon(const char* title, int x, int y, int w, int h, Uint32 flags){
    SDL_Window* win = SDL_CreateWindow(title, x, y, w, h, flags);
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
        window = createWindowWithIcon(windowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, winFlags);

        #ifndef __EMSCRIPTEN__
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
        #endif
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        drawBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, width, height);
        SDL_LockTextureToSurface(drawBuffer, NULL, &surface);

        #ifndef __EMSCRIPTEN__
        setScaleMode(scale_mode);
        SDL_SetEventFilter(filterResize, NULL);
        calculateRescaleVars();

        glCreateShader = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
        glShaderSource = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
        glCompileShader = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
        glGetShaderiv = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
        glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
        glDeleteShader = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");
        glAttachShader = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
        glCreateProgram = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
        glLinkProgram = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
        glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)SDL_GL_GetProcAddress("glValidateProgram");
        glGetProgramiv = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
        glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)SDL_GL_GetProcAddress("glGetProgramInfoLog");
        glUseProgram = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
        glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation");
        glUniform1f = (PFNGLUNIFORM1FPROC)SDL_GL_GetProcAddress("glUniform1f");
        glUniform2f = (PFNGLUNIFORM2FPROC)SDL_GL_GetProcAddress("glUniform2f");
        #endif

        pixels = surface->pixels;
    } else {
        #ifndef __EMSCRIPTEN__
        SDL_DestroyTexture(drawBuffer);
        drawBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, width, height);
        SDL_LockTextureToSurface(drawBuffer, NULL, &surface);
        calculateRescaleVars();
        pixels = surface->pixels;
        #endif
    }
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
    #ifdef MAINLOOP_WINDOWS
    if(hwnd && window && mainMenu){
        if(winFlags & SDL_WINDOW_FULLSCREEN_DESKTOP)
            SetMenu(hwnd, mainMenu);
        else
            SetMenu(hwnd, NULL);
        SDL_SetWindowSize(window, width, height);
    }
    #endif

    if(!(winFlags & SDL_WINDOW_FULLSCREEN_DESKTOP))
        winFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    else 
        winFlags &= ~(SDL_WINDOW_FULLSCREEN_DESKTOP);
    
    if(window)
        SDL_SetWindowFullscreen(window, winFlags & SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void background(int col){
    SDL_FillRect(surface, &surface->clip_rect, col);
}

int color(int red, int green, int blue){
    return SDL_MapRGB(surface->format, red, green, blue);
}

void getRGB(int pixel, Uint8* red, Uint8* green, Uint8* blue){
    SDL_GetRGB(pixel, surface->format, red, green, blue);
}

void rect(int x, int y, int w, int h, int col){
    SDL_Rect rect = {x, y, w, h};
    SDL_FillRect(surface, &rect, col);
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

void noRender(){
    render_every_frame = false;
}

void autoRender(){
    render_every_frame = true;
}

void renderPixels(){
    SDL_UnlockTexture(drawBuffer);
    renderBufferToWindow();
    SDL_LockTextureToSurface(drawBuffer, NULL, &surface);
    pixels = surface->pixels;
}

GLuint compileShader(const char* source, GLuint shaderType) {
	GLuint result = glCreateShader(shaderType);
	glShaderSource(result, 1, &source, NULL);
	glCompileShader(result);

	//Check vertex shader for errors
	GLint shaderCompiled = GL_FALSE;
	glGetShaderiv( result, GL_COMPILE_STATUS, &shaderCompiled );
	if( shaderCompiled != GL_TRUE ) {
		fprintf(stderr, "Shader compilation error %d!\n", result);
		GLint logLength;
		glGetShaderiv(result, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0)
		{
			GLchar *log = (GLchar*)malloc(logLength);
			glGetShaderInfoLog(result, logLength, &logLength, log);
			fprintf(stdout, "compield Shader log:\n%s\n", log);
			free(log);
		}
		glDeleteShader(result);
		result = 0;
	}
	return result;
}

GLuint compileProgram(const char* fragFile) {
	GLuint programId = 0;
	GLuint fragShaderId;

	programId = glCreateProgram();

	FILE* fptr = fopen(fragFile, "rb");
	fseek(fptr, 0, SEEK_END);
	size_t size = ftell(fptr);
	rewind(fptr);
	char* fcontent = (char*)malloc(size+1);
	fread(fcontent, 1, size, fptr);
    fcontent[size] = '\0';

	fragShaderId = compileShader(fcontent, GL_FRAGMENT_SHADER);
	
	if(fragShaderId) {
		glAttachShader(programId, fragShaderId);
		glLinkProgram(programId);
		glValidateProgram(programId);

        glDeleteShader(fragShaderId);

		GLint logLen;
		glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLen);
		if(logLen > 0) {
			char* log = (char*) malloc(logLen * sizeof(char));

			glGetProgramInfoLog(programId, logLen, &logLen, log);
			fprintf(stdout, "Prog Info Log: \n%s\n", log);
			free(log);
		}
	}
	return programId;
}

Shader loadShader(const char* filename){
    shader_list_t* block = (shader_list_t*)malloc(sizeof(shader_list_t));
    block->next = NULL;
    if(!shader_list){
        shader_list = block;
    } else {
        block->next = shader_list;
        shader_list = block;
    }
    block->id = compileProgram(filename);
    return &block->id;
}

void noGlobalShader(){
    globalShader = 0;
}

void setGlobalShader(Shader sh){
    shader_list_t* p = shader_list;
    while(p->id != *sh)
        p = p->next;
    globalShader = p->id; 
}

void setScaleMode(ScaleMode mode){
    scale_mode = mode;
    if(!renderer)
        return;    

    switch(mode){
        case NEAREST:
        SDL_SetTextureScaleMode(drawBuffer, SDL_ScaleModeNearest);
        break;

        case LINEAR:
        SDL_SetTextureScaleMode(drawBuffer, SDL_ScaleModeLinear);
        break;

        case ANISOTROPIC:
        SDL_SetTextureScaleMode(drawBuffer, SDL_ScaleModeBest);
        break;
    }
}

void setVoidColor(int r, int g, int b){
    glClearColor(r/255.0f, g/255.0f, b/255.0f, 1.0f);
}

void calculateRescaleVars(){
    if(aspectRatio == 0)
        aspectRatio = (float)width/height;
    SDL_GetWindowSize(window, &win_width, &win_height);
    win_ratio = (float)win_width/win_height;
    if(win_ratio > aspectRatio){
        render_width = (float)aspectRatio*win_height + 0.5f;
        render_height = win_height;
    } else {
        render_width = win_width;
        render_height = win_width/aspectRatio + 0.5f;
    }
    localX = win_width/2-render_width/2;
    localY = win_height/2-render_height/2;
}

void setWindowSize(int w, int h){
    SDL_SetWindowSize(window, w, h);
    calculateRescaleVars();
}

void setAspectRatio(float ratio){
    aspectRatio = ratio;
    calculateRescaleVars();
}

void renderBufferToWindow(){
    SDL_RenderClear(renderer);

    #ifndef __EMSCRIPTEN__
    SDL_GL_BindTexture(drawBuffer, NULL, NULL);
    glUseProgram(globalShader);

    glViewport(win_width/2-render_width/2, win_height/2-render_height/2, render_width, render_height);
    glUniform1f(glGetUniformLocation(globalShader, "width"), render_width);
    glUniform1f(glGetUniformLocation(globalShader, "height"), render_height);
    glUniform1f(glGetUniformLocation(globalShader, "localX"), localX);
    glUniform1f(glGetUniformLocation(globalShader, "localY"), localY);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-1, 1);
        glTexCoord2f(1, 0); glVertex2f(1, 1);
        glTexCoord2f(1, 1); glVertex2f(1, -1);
        glTexCoord2f(0, 1); glVertex2f(-1, -1);
    glEnd();
    #else
    SDL_RenderCopy(renderer, drawBuffer, NULL, NULL);
    #endif

    SDL_RenderPresent(renderer);
}

int filterResize(void* userdata, SDL_Event* event){
    if(event->type == SDL_WINDOWEVENT)
        if(event->window.event == SDL_WINDOWEVENT_RESIZED || event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
            calculateRescaleVars();
            renderBufferToWindow();
            return 0;
        }

    return 1;
}

#ifdef MAINLOOP_WINDOWS
void getAbsoluteDir(char* dst){
    strcpy(dst, absolutePath);
    strcat(dst, "\\");
}

HWND getWindowHandler(){
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    return wmInfo.info.win.window;
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