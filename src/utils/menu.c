#include "utils/menu.h"
#include "utils/sound.h"

#include "core.h"

#include "SDL_MAINLOOP.h"

static buttonId pause_button;
static buttonId speed_buttons[4];
static buttonId fullscreen_button;
static ctx_set_speed_args_t speed_args[4];

static SDL_RendererLogicalPresentation fit_mode = SDL_LOGICAL_PRESENTATION_LETTERBOX;
static SDL_RendererLogicalPresentation stretch_mode = SDL_LOGICAL_PRESENTATION_STRETCH;
static SDL_RendererLogicalPresentation integer_mode = SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;

typedef struct open_ctx_t {
    core_ctx_t* ctx;
    bool rom;
    bool bios;
} open_ctx_t;

static void menu_fullscreen(){
    static bool is_fullscreen = false;
    is_fullscreen ^= 1;
    fullScreen();
    tickButton(fullscreen_button, is_fullscreen);
}

static void menu_change_scaling_mode(SDL_RendererLogicalPresentation* mode){
    setScalingMode(*mode);
}

static void on_file_chosen(void *userdata, const char * const *filelist, int filter){
    open_ctx_t* menu_ctx = (open_ctx_t*)userdata;
    core_ctx_t* ctx = menu_ctx->ctx;

    if(filelist == NULL || filelist[0] == NULL)
        goto end;

    const char* rom_path = menu_ctx->rom ? filelist[0] : NULL;
    const char* bios_path = menu_ctx->bios ? filelist[0] : NULL;

    core_ctx_close(ctx);
    core_ctx_init(ctx, rom_path, bios_path, NULL);
    core_restart(ctx);

end:
    sound_pause(ctx->pause);
    free(menu_ctx);
    SDL_SetAtomicInt(&ctx->must_wait, 0);
}

void menu_open_rom(core_ctx_t* ctx){
    open_ctx_t* open_ctx = malloc(sizeof(open_ctx_t));
    open_ctx->ctx = ctx;
    open_ctx->rom = true;
    open_ctx->bios = false;

    SDL_SetAtomicInt(&ctx->must_wait, 1);
    sound_pause(true);

    SDL_PropertiesID ids = SDL_CreateProperties();
    SDL_SetPointerProperty(ids, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, getMainWindow());
    SDL_SetStringProperty(ids, SDL_PROP_FILE_DIALOG_TITLE_STRING, "Select ROM");
    SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFILE, on_file_chosen, open_ctx, ids);
    SDL_DestroyProperties(ids);
}

void menu_open_bios(core_ctx_t* ctx){
    open_ctx_t* open_ctx = malloc(sizeof(open_ctx_t));
    open_ctx->ctx = ctx;
    open_ctx->rom = false;
    open_ctx->bios = true;

    SDL_SetAtomicInt(&ctx->must_wait, 1);
    sound_pause(true);

    SDL_PropertiesID ids = SDL_CreateProperties();
    SDL_SetPointerProperty(ids, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, getMainWindow());
    SDL_SetStringProperty(ids, SDL_PROP_FILE_DIALOG_TITLE_STRING, "Select Bios");
    SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFILE, on_file_chosen, open_ctx, ids);
    SDL_DestroyProperties(ids);
}

void menu_speed_check(int speed_level){
    checkRadioButton(speed_buttons[speed_level]);
}

void menu_create(core_ctx_t* ctx){
    for(int i = 0; i < 4; i++){
        speed_args[i].ctx = ctx;
        speed_args[i].speed = i;
    }

    menuId file_menu = addMenuTo(-1, L"File", false);
    addButtonTo(file_menu, L"Open Rom", (void*)menu_open_rom, ctx);
    addButtonTo(file_menu, L"Open Bios", (void*)menu_open_bios, ctx);

    menuId emu_menu = addMenuTo(-1, L"Emu", false);
    menuId speed_menu = addMenuTo(emu_menu, L"Speed", true);
    menuId video_menu = addMenuTo(-1, L"Video", false);

    fullscreen_button = addButtonTo(video_menu, L"Fullscreen", (void*)menu_fullscreen, NULL);
    menuId scaling_menu = addMenuTo(video_menu, L"Scaling", true);

    buttonId fit_button = addButtonTo(scaling_menu, L"Fit", (void*)menu_change_scaling_mode, &fit_mode);
    addButtonTo(scaling_menu, L"Integer", (void*)menu_change_scaling_mode, &integer_mode);
    addButtonTo(scaling_menu, L"Stretch", (void*)menu_change_scaling_mode, &stretch_mode);
    checkRadioButton(fit_button);

    pause_button = addButtonTo(emu_menu, L"Pause", (void*)core_switch_pause, ctx);
    addButtonTo(emu_menu, L"Restart", (void*)core_restart, ctx);

    for(int i = 0; i < 4; i++){
        wchar_t speed_str[3] = {L'0' + (1 << i), L'x', 0};
        speed_buttons[i] = addButtonTo(speed_menu, speed_str, (void*)core_ctx_set_speed, &speed_args[i]);
        if(i == 0)    
            checkRadioButton(speed_buttons[i]);
    }
}

void menu_tick_pause(bool paused){
    tickButton(pause_button, paused);
}