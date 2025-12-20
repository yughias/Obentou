#include "utils/menu.h"
#include "utils/sound.h"
#include "utils/argument.h"
#include "utils/state.h"

#include "core.h"

#include "tinyfiledialogs.h"
#include "minIni.h"

#include "version.h"

#include "SDL_MAINLOOP.h"

#define MAX_SHOW_PATH_LENGTH 48

static buttonId pause_button;
static buttonId speed_buttons[4];
static buttonId fullscreen_button;
static buttonId default_bios_button;
static ctx_args_t speed_args[4];
static ctx_args_t load_recent_args[10];
static ctx_args_t slot_args[5];
static control_t control_args[CONTROL_COUNT];
static core_ctx_t* core_ctx_arg;

static SDL_RendererLogicalPresentation fit_mode = SDL_LOGICAL_PRESENTATION_LETTERBOX;
static SDL_RendererLogicalPresentation stretch_mode = SDL_LOGICAL_PRESENTATION_STRETCH;
static SDL_RendererLogicalPresentation integer_mode = SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;

static void menu_info(){
    static const char* title = "Obentou";
    static const char* description = 
        "Made by yughias\n"
        "Visit yughias.github.io\n"
        "Version: " OBENTOU_VERSION
    ;

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title, description, getMainWindow());
}

static void controls_input_box_scancode(control_t* scancode_ptr){
    const char* input = tinyfd_inputBox("Input box", "Enter scancode", controls_get_scancode_name(*scancode_ptr));
    controls_set_scancode((control_t)*scancode_ptr, input); 
    menu_create(core_ctx_arg);
}

static void controls_input_box_gamepad(control_t* gamepad_ptr){
    const char* input = tinyfd_inputBox("Input box", "Enter gamepad", controls_get_gamepad_name(*gamepad_ptr));
    controls_set_gamepad((control_t)*gamepad_ptr, input); 
    menu_create(core_ctx_arg);
}

static void create_input_button_menu(menuId hotkey_menu, const char* name, control_t control_begin, control_t control_end, bool show_gamepad){
    menuId submenu = addMenuTo(hotkey_menu, name, false);
    menuId scancode_submenu = addMenuTo(submenu, "Keyboard", false);
    menuId gamepad_submenu;

    if(show_gamepad)
        gamepad_submenu = addMenuTo(submenu, "Gamepad", false);

    for(int i = control_begin; i <= control_end; i++){
        char name[32];
        control_args[i] = i;
        sprintf(name, "%s : %s", controls_names[i], controls_get_scancode_name(i));
        addButtonTo(scancode_submenu, name, (void*)controls_input_box_scancode, &control_args[i]);
        if(show_gamepad){
            sprintf(name, "%s : %s", controls_names[i], controls_get_gamepad_name(i));
            addButtonTo(gamepad_submenu, name, (void*)controls_input_box_gamepad, &control_args[i]);
        }
    }
}

static bool compose_recent_text(char* out, size_t len, int idx){
    char path[FILENAME_MAX];
    char arg[16];

    bool is_rom;
    bool is_bios;

    out[0] = 0;

    snprintf(arg, sizeof(arg), "ROM%d", idx);
    argument_get_path(path, "RECENTS", arg);
    is_rom = path[0];
    if(is_rom){
        strcat(out, "Rom: ");
        strcat(out, path);
    }

    snprintf(arg, sizeof(arg), "BIOS%d", idx);
    argument_get_path(path, "RECENTS", arg);
    is_bios = path[0];
    if(is_rom && is_bios)
        strcat(out, " | ");
    if(is_bios){
        strcat(out, "Bios: ");
        strcat(out, path);
    }

    return is_rom || is_bios;
}

static void get_bios_path_button_text(char* out, size_t len, const char* core_name){
    char default_bios_path[FILENAME_MAX];
    argument_get_default_bios(default_bios_path, core_name);

    int bios_length = strlen(default_bios_path);

    out[0] = 0;

    if(bios_length <= MAX_SHOW_PATH_LENGTH){
        strcat(out, "Bios Path: ");
        strcat(out, default_bios_path[0] ? default_bios_path : "None");
    } else {
        strcat(out, "Bios Path: ...");
        strcat(out, default_bios_path + bios_length - MAX_SHOW_PATH_LENGTH);
    }
}

void menu_fullscreen(){
    static bool is_fullscreen = false;
    is_fullscreen ^= 1;
    fullScreen();
    tickButton(fullscreen_button, is_fullscreen);
}

static void menu_change_scaling_mode(SDL_RendererLogicalPresentation* mode){
    setScalingMode(*mode);
}

static void menu_clear_recent(core_ctx_t* ctx){
    char rom_arg[16];
    char bios_arg[16];

    for(int i = 0; i < 10; i++){
        snprintf(rom_arg, sizeof(rom_arg), "ROM%d", i);
        snprintf(bios_arg, sizeof(bios_arg), "BIOS%d", i);

        argument_set_path("", "RECENTS", rom_arg);
        argument_set_path("", "RECENTS", bios_arg);
    }

    menu_create(ctx);
}

static void menu_load_recent(ctx_args_t* args){
    char rom_path[FILENAME_MAX];
    char bios_path[FILENAME_MAX];
    char rom_arg[16];
    char bios_arg[16];

    snprintf(rom_arg, sizeof(rom_arg), "ROM%d", args->value);
    snprintf(bios_arg, sizeof(bios_arg), "BIOS%d", args->value);

    argument_get_path(rom_path, "RECENTS", rom_arg);
    argument_get_path(bios_path, "RECENTS", bios_arg);

    core_ctx_t* ctx = args->ctx;

    core_ctx_close(ctx);
    core_ctx_init(ctx, rom_path, bios_path, NULL);
    core_restart(ctx);
}

static void menu_open(core_ctx_t* ctx, bool is_rom){
    sound_pause(true);

    char* filelist = tinyfd_openFileDialog(
        is_rom ? "Select ROM" : "Select Bios",
        NULL,
        0,
        NULL,
        NULL,
        0
    );

    if(filelist == NULL)
        return;

    const char* rom_path = is_rom ? filelist : NULL;
    const char* bios_path = !is_rom ? filelist : NULL;

    core_ctx_close(ctx);
    core_ctx_init(ctx, rom_path, bios_path, NULL);
    core_restart(ctx);
}

void menu_open_rom(core_ctx_t* ctx){
    menu_open(ctx, true);
}

void menu_open_bios(core_ctx_t* ctx){
    menu_open(ctx, false);
}

void menu_select_default_bios(core_ctx_t* ctx){
    sound_pause(true);

    char* selected_default_bios = tinyfd_openFileDialog(
        "Select Default Bios",
        NULL,
        0,
        NULL,
        NULL,
        0
    );

    const char* core_name = ctx->core->name;

    argument_set_default_bios(selected_default_bios, core_name);

    char new_path[FILENAME_MAX];
    get_bios_path_button_text(new_path, FILENAME_MAX, core_name);
    setButtonTitle(default_bios_button, new_path);
}

void menu_speed_check(int speed_level){
    checkRadioButton(speed_buttons[speed_level]);
}

void menu_create(core_ctx_t* ctx){
    core_ctx_arg = ctx;
    destroyAllMenus();

    char* label = malloc(1024);

    for(int i = 0; i < 4; i++){
        speed_args[i].ctx = ctx;
        speed_args[i].value = i;
    }

    for(int i = 0; i < 5; i++){
        slot_args[i].ctx = ctx;
        slot_args[i].value = i;
    }

    menuId file_menu = addMenuTo(-1, "File", false);
    menuId recent_menu = addMenuTo(file_menu, "Recent", false);
    addButtonTo(file_menu, "Open Rom", (void*)menu_open_rom, ctx);
    if(!ctx->core){
        addButtonTo(file_menu, "Open Bios", (void*)menu_open_bios, ctx);
    } else {
        bool has_bios = ctx->core->has_bios;
        menuId bios_menu = file_menu;
        if(has_bios){
            bios_menu = addMenuTo(file_menu, "Bios", false);
        }
        addButtonTo(bios_menu, has_bios ? "Open" : "Open Bios", (void*)menu_open_bios, ctx);
        if(has_bios){
            get_bios_path_button_text(label, 1024, ctx->core->name);
            default_bios_button = addButtonTo(bios_menu, label, (void*)menu_select_default_bios, ctx);
        }

        if(ctx->core->savestate && ctx->core->loadstate){
            menuId state_menu = addMenuTo(-1, "State", false);
            menuId slot_menu = addMenuTo(state_menu, "Select Slot", true);
            menuId auto_savestate = addButtonTo(state_menu, "Auto Load on Open", (void*)state_switch_autosave, NULL);
            buttonId slot_btns[5];
            tickButton(auto_savestate, state_get_autosave());
            addButtonTo(state_menu, "Save State", (void*)state_save_slot, ctx);
            addButtonTo(state_menu, "Load State", (void*)state_load_slot, ctx);
            for(int i = 0; i < sizeof(slot_btns) / sizeof(slot_btns[0]); i++){
                if(!i){
                    slot_btns[i] = addButtonTo(slot_menu, "0 (autosave)", (void*)state_set_active_slot, &slot_args[i].value);
                } else {
                    char name[2] = {'0' + i, 0};
                    slot_btns[i] = addButtonTo(slot_menu, name, (void*)state_set_active_slot, &slot_args[i].value);
                }        
            }
            checkRadioButton(slot_btns[state_get_active_slot()]);
        }
    }

    menuId emu_menu = addMenuTo(-1, "Emu", false);
    menuId speed_menu = addMenuTo(emu_menu, "Speed", true);
    menuId video_menu = addMenuTo(-1, "Video", false);

    fullscreen_button = addButtonTo(video_menu, "Fullscreen", (void*)menu_fullscreen, NULL);
    menuId scaling_menu = addMenuTo(video_menu, "Scaling", true);

    buttonId fit_button = addButtonTo(scaling_menu, "Fit", (void*)menu_change_scaling_mode, &fit_mode);
    addButtonTo(scaling_menu, "Integer", (void*)menu_change_scaling_mode, &integer_mode);
    addButtonTo(scaling_menu, "Stretch", (void*)menu_change_scaling_mode, &stretch_mode);
    checkRadioButton(fit_button);

    pause_button = addButtonTo(emu_menu, "Pause", (void*)core_switch_pause, ctx);
    addButtonTo(emu_menu, "Restart", (void*)core_restart, ctx);

    for(int i = 0; i < 4; i++){
        char speed_str[3] = {'0' + (1 << i), 'x', 0};
        speed_buttons[i] = addButtonTo(speed_menu, speed_str, (void*)core_ctx_set_speed, &speed_args[i]);
        if(i == 0)    
            checkRadioButton(speed_buttons[i]);
    }

    for(int i = 0; i < 10; i++){
        load_recent_args[i].ctx = ctx;
        load_recent_args[i].value = i;
        if(!compose_recent_text(label, 1024, i))
            break;
        addButtonTo(recent_menu, label, (void*)menu_load_recent, &load_recent_args[i]);
    }

    addButtonTo(recent_menu, "Clear History", (void*)menu_clear_recent, ctx);

    menuId input_menu = addMenuTo(-1, "Input", false);
    menuId hotkey_menu = addMenuTo(input_menu, "Hotkeys", false);
    create_input_button_menu(hotkey_menu, "Main key", CONTROL_HOTKEY_BEGIN, CONTROL_HOTKEY_END, true);
    create_input_button_menu(hotkey_menu, "Secondary key", CONTROL_HOTKEY_CMD_BEGIN, CONTROL_HOTKEY_CMD_END, false);
    
    if(ctx->core)
        create_input_button_menu(input_menu, ctx->core->name, ctx->core->control_begin, ctx->core->control_end, true);

    for(int i = 0; i < sizeof(cores) / sizeof(cores[0]); i++){
        if(ctx->core && !strcmp(ctx->core->name, cores[i].name))
            continue;
        create_input_button_menu(input_menu, cores[i].name, cores[i].control_begin, cores[i].control_end, true);
    }

    addButtonTo(-1, "About", (void*)menu_info, NULL);

    free(label);
}

void menu_tick_pause(bool paused){
    tickButton(pause_button, paused);
}