#include "utils/menu.h"
#include "utils/sound.h"
#include "utils/argument.h"

#include "core.h"

#include "minIni.h"

#include "SDL_MAINLOOP.h"

#define MAX_SHOW_PATH_LENGTH 48

static buttonId pause_button;
static buttonId speed_buttons[4];
static buttonId fullscreen_button;
static buttonId default_bios_button;
static ctx_args_t speed_args[4];
static ctx_args_t load_recent_args[10];

static SDL_RendererLogicalPresentation fit_mode = SDL_LOGICAL_PRESENTATION_LETTERBOX;
static SDL_RendererLogicalPresentation stretch_mode = SDL_LOGICAL_PRESENTATION_STRETCH;
static SDL_RendererLogicalPresentation integer_mode = SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;


static void chars_to_wchars(wchar_t* out, const char* in){
    int i;
    for(i = 0; i < strlen(in); i++){
        out[i] = in[i];
    }
    out[i] = 0;
}

static bool compose_recent_text(wchar_t* out, int idx){
    char path[FILENAME_MAX];
    char arg[16];

    bool is_rom;
    bool is_bios;

    out[0] = 0;

    snprintf(arg, sizeof(arg), "ROM%d", idx);
    ini_gets("RECENTS", arg, "", path, FILENAME_MAX, argument_get_ini_path());
    is_rom = path[0];
    if(is_rom){
        chars_to_wchars(out + wcslen(out), "Rom: ");
        chars_to_wchars(out + wcslen(out), path);
    }

    snprintf(arg, sizeof(arg), "BIOS%d", idx);
    ini_gets("RECENTS", arg, "", path, FILENAME_MAX, argument_get_ini_path());
    is_bios = path[0];
    if(is_rom && is_bios)
        chars_to_wchars(out + wcslen(out), " | ");
    if(is_bios){
        chars_to_wchars(out + wcslen(out), "Bios: ");
        chars_to_wchars(out + wcslen(out), path);
    }

    return is_rom || is_bios;
}

typedef struct open_ctx_t {
    core_ctx_t* ctx;
    bool rom;
    bool bios;
} open_ctx_t;

static void get_bios_path_button_text(wchar_t* out, const char* core_name){
    char default_bios_path[FILENAME_MAX];
    argument_get_default_bios(default_bios_path, core_name);
    
    int len = strlen(default_bios_path);

    if(len <= MAX_SHOW_PATH_LENGTH){
        chars_to_wchars(out, "Bios Path: ");
        chars_to_wchars(out + wcslen(out), default_bios_path[0] ? default_bios_path : "None");
    } else {
        chars_to_wchars(out, "Bios Path: ...");
        chars_to_wchars(out + wcslen(out), default_bios_path + len - MAX_SHOW_PATH_LENGTH);
    }
}

static void on_default_bios_chosen(void *userdata, const char * const *filelist, int filter){
    core_ctx_t* ctx = (core_ctx_t*)userdata;
    char* core_name = (char*)ctx->core->name;
    
    if(filelist == NULL)
        goto end;

    argument_set_default_bios(filelist[0], core_name);

    wchar_t new_path[FILENAME_MAX];
    get_bios_path_button_text(new_path, core_name);
    setButtonTitle(default_bios_button, new_path);
    
end:
    sound_pause(ctx->pause);
    SDL_SetAtomicInt(&ctx->must_wait, 0);
}

static void menu_fullscreen(){
    static bool is_fullscreen = false;
    is_fullscreen ^= 1;
    fullScreen();
    tickButton(fullscreen_button, is_fullscreen);
}

static void menu_change_scaling_mode(SDL_RendererLogicalPresentation* mode){
    setScalingMode(*mode);
}

static void menu_load_recent(ctx_args_t* args){
    char rom_path[FILENAME_MAX];
    char bios_path[FILENAME_MAX];
    char rom_arg[16];
    char bios_arg[16];

    snprintf(rom_arg, sizeof(rom_arg), "ROM%d", args->value);
    snprintf(bios_arg, sizeof(bios_arg), "BIOS%d", args->value);

    ini_gets("RECENTS", rom_arg, "", rom_path, FILENAME_MAX, argument_get_ini_path());
    ini_gets("RECENTS", bios_arg, "", bios_path, FILENAME_MAX, argument_get_ini_path());

    core_ctx_t* ctx = args->ctx;
    int idx = args->value;

    core_ctx_close(ctx);
    core_ctx_init(ctx, rom_path, bios_path, NULL);
    core_restart(ctx);
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

void menu_select_default_bios(core_ctx_t* ctx){
    SDL_SetAtomicInt(&ctx->must_wait, 1);
    sound_pause(true);

    SDL_PropertiesID ids = SDL_CreateProperties();
    SDL_SetPointerProperty(ids, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, getMainWindow());
    SDL_SetStringProperty(ids, SDL_PROP_FILE_DIALOG_TITLE_STRING, "Select Default Bios");
    SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFILE, on_default_bios_chosen, ctx, ids);
    SDL_DestroyProperties(ids);
}

void menu_speed_check(int speed_level){
    checkRadioButton(speed_buttons[speed_level]);
}

void menu_create(core_ctx_t* ctx){
    destroyAllMenus();

    wchar_t* label = malloc(sizeof(wchar_t) * 1024);

    for(int i = 0; i < 4; i++){
        speed_args[i].ctx = ctx;
        speed_args[i].value = i;
    }

    menuId file_menu = addMenuTo(-1, L"File", false);
    menuId recent_menu = addMenuTo(file_menu, L"Recent", false);
    addButtonTo(file_menu, L"Open Rom", (void*)menu_open_rom, ctx);
    if(!ctx->core){
        addButtonTo(file_menu, L"Open Bios", (void*)menu_open_bios, ctx);
    } else {
        bool has_bios = ctx->core->has_bios;
        menuId bios_menu = file_menu;
        if(has_bios){
            bios_menu = addMenuTo(file_menu, L"Bios", false);
        }
        addButtonTo(bios_menu, has_bios ? L"Open" : L"Open Bios", (void*)menu_open_bios, ctx);
        if(has_bios){
            get_bios_path_button_text(label, ctx->core->name);
            default_bios_button = addButtonTo(bios_menu, label, (void*)menu_select_default_bios, ctx);
        }
    }

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

    for(int i = 0; i < 10; i++){
        load_recent_args[i].ctx = ctx;
        load_recent_args[i].value = i;
        if(!compose_recent_text(label, i))
            break;
        addButtonTo(recent_menu, label, (void*)menu_load_recent, &load_recent_args[i]);
    }

    free(label);
}

void menu_tick_pause(bool paused){
    tickButton(pause_button, paused);
}