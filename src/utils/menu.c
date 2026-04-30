#include "utils/menu.h"
#include "utils/sound.h"
#include "utils/argument.h"
#include "utils/state.h"
#include "utils/controls.h"
#include "utils/netplay.h"

#include "core.h"

#include "tinyfiledialogs.h"
#include "minIni.h"

#include "version.h"

#include "SDL_MAINLOOP.h"

#include <time.h>

#define MAX_SHOW_PATH_LENGTH 48

static buttonId pause_button;
static buttonId speed_buttons[4];
static buttonId fullscreen_button;
static buttonId default_bios_button;
static buttonId disable_illegal_input_button;
static buttonId autosave_button;
static buttonId keyboard_player_select_button[2];
static buttonId netplay_mode_buttons[NETPLAY_MODE_COUNT];
static buttonId netplay_port_button; 
static buttonId netplay_ip_button;
static buttonId netplay_input_delay_button;

typedef struct {
    int gamepad_idx;
    int player_idx;
} gamepad_assign_t;

static gamepad_assign_t gamepad_assign_args[MAX_GAMEPADS][2];
static int keyboard_player_select_id[2] = {0, 1};

static ctx_args_t speed_args[4];
static ctx_args_t load_recent_args[10];
static ctx_args_t slot_args[5];
static control_t control_args[CONTROL_COUNT];
static core_ctx_t* core_ctx_arg;

static buttonId audio_channel_buttons[MAX_AUDIO_CHANNELS];
static int apu_idxs[MAX_AUDIO_CHANNELS];
static int widget_idxs[MAX_WIDGETS];

static SDL_RendererLogicalPresentation fit_mode = SDL_LOGICAL_PRESENTATION_LETTERBOX;
static SDL_RendererLogicalPresentation stretch_mode = SDL_LOGICAL_PRESENTATION_STRETCH;
static SDL_RendererLogicalPresentation integer_mode = SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;

static NETPLAY_MODE netplay_modes[NETPLAY_MODE_COUNT] = {NETPLAY_NONE, NETPLAY_HOST, NETPLAY_CLIENT};

void menu_save_screenshot(core_ctx_t* ctx)
{
    const char* base_path = ctx->rom ? archive_get_path(ctx->rom) : archive_get_path(ctx->bios);
    char base[FILENAME_MAX];
    char path[FILENAME_MAX];

    path_set_ext(base_path, base, "");
    base[strlen(base) - 1] = '\0';

    time_t now = time(NULL);
    struct tm* tm = localtime(&now);

    char datetime[32];
    strftime(datetime, sizeof(datetime), "%Y%m%d_%H%M%S", tm);

    snprintf(path, sizeof(path), "%s_%s_%u.bmp", base, datetime, frameCount);

    SDL_SaveBMP(getMainWindowSurface(), path);
}

static void set_keyboard_player(int* id_ptr){
    int id = *id_ptr;
    ini_putl("GENERAL", "KEYBOARD_PLAYER", id, argument_get_ini_path());
    controls_set_keyboard_player(id);
}

static void set_gamepad_player(gamepad_assign_t* args){
    controls_set_gamepad_player(args->gamepad_idx, args->player_idx);
    
    char key[32];
    sprintf(key, "GAMEPAD_%d_PLAYER", args->gamepad_idx);
    ini_putl("GENERAL", key, args->player_idx, argument_get_ini_path());
}

static void menu_disable_illegal(void* dummy){
    bool val = ini_getbool("GENERAL", "DISABLE_ILLEGAL_INPUT", true, argument_get_ini_path());
    val ^= 1;
    ini_putbool("GENERAL", "DISABLE_ILLEGAL_INPUT", val, argument_get_ini_path());
    tickButton(disable_illegal_input_button, val);
    controls_disable_illegal_input(val);
}

static void menu_info(void* dummy){
    static const char* title = "Obentou";
    static const char* description = 
        "Made by yughias\n"
        "Visit yughias.github.io\n"
        "Version: " OBENTOU_VERSION
    ;

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title, description, getMainWindow());
}

static void menu_switch_autosave(void* dummy){
    state_switch_autosave();
    tickButton(autosave_button, state_get_autosave());
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

        #ifdef __EMSCRIPTEN__
        char file_to_delete[FILENAME_MAX];
        argument_get_path(file_to_delete, "RECENTS", rom_arg);
        if(file_to_delete[0]) 
            remove(file_to_delete);
        #endif

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

static void apu_mute_channel(void* audio_idx){
    int idx = *(int*)audio_idx;
    sound_channel_muted[idx] ^= 1;
    tickButton(audio_channel_buttons[idx], !sound_channel_muted[idx]);
}

static void menu_open_wave_viewer(void* audio_idx){
    int idx = *(int*)audio_idx;
    const sound_channel_t *ch = &core_ctx_arg->core->sound_channels[idx];
    sound_open_wave_viewer(ch->name, ch->min, ch->max, idx);
}

static void menu_open_widget(void* widget_idx){
    int idx = *(int*)widget_idx;
    const widget_t* w = &core_ctx_arg->core->widgets[idx];
    createWidget(w->name, 1, 1, w->draw, core_ctx_arg->emu);
}

static void netplay_set_mode(void* mode){
    netplay_wanted_mode = *(int*)mode;
}

static void netplay_set_port(void* arg){
    char text[64];
    sprintf(text, "%d", netplay_port);
    const char* port = tinyfd_inputBox("Set Port", "Enter Port", text);
    if(!port || !atoi(port))
        return;
    netplay_port = atoi(port);
    sprintf(text, "Port: %d", netplay_port);
    setButtonTitle(netplay_port_button, text);
}

static void netplay_set_ip(void* arg){
    char text[64];
    const char* ip = tinyfd_inputBox("Set IP", "Enter IP", netplay_host_ip);
    if(!ip)
        return;
    strcpy(netplay_host_ip, ip);
    sprintf(text, "Connect to: %s", netplay_host_ip);
    setButtonTitle(netplay_ip_button, text);
}

static void netplay_set_input_delay(void* arg){
    char text[64];
    const char* ip = tinyfd_inputBox("Set Input Delay", "Enter Input Delay", "0");
    if(!ip)
        return;
    netplay_input_delay = atoi(ip);
    sprintf(text, "Input Delay: %d", netplay_input_delay);
    setButtonTitle(netplay_ip_button, text);
}

void menu_create(core_ctx_t* ctx){
    core_ctx_arg = ctx;
    destroyAllMenus();

    char* label = malloc(1024);

    for(int g = 0; g < MAX_GAMEPADS; g++){
        for(int p = 0; p < 2; p++){
            gamepad_assign_args[g][p].gamepad_idx = g;
            gamepad_assign_args[g][p].player_idx = p;
        }
    }

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
            autosave_button = addButtonTo(state_menu, "Auto Load on Open", (void*)menu_switch_autosave, NULL);
            buttonId slot_btns[5];
            tickButton(autosave_button, state_get_autosave());
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
    menu_change_scaling_mode(&fit_mode);
    checkRadioButton(fit_button);

    if(ctx->core)
        addButtonTo(video_menu, "Screenshot", (void*)menu_save_screenshot, ctx);

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
    disable_illegal_input_button = addButtonTo(input_menu, "Disable illegal input", (void*)menu_disable_illegal, NULL);
    tickButton(disable_illegal_input_button, ini_getbool("GENERAL", "DISABLE_ILLEGAL_INPUT", true, argument_get_ini_path()));
    menuId peripherals_menu = addMenuTo(input_menu, "Peripherals", false);
    menuId keyboard_menu = addMenuTo(peripherals_menu, "Keyboard", true);
    for(int i = 0; i < 2; i++){
        char name[2] = {'1' + i, 0};
        keyboard_player_select_button[i] = addButtonTo(keyboard_menu, name, (void*)set_keyboard_player, &keyboard_player_select_id[i]);
    }
    checkRadioButton(keyboard_player_select_button[ini_getl("GENERAL", "KEYBOARD_PLAYER", 0, argument_get_ini_path())]);
    
    for(int i = 0; i < MAX_GAMEPADS; i++) {
        char pad_name[64];
        int pad_id;
        controls_get_gamepad_info(i, pad_name, 64, &pad_id);

        if(pad_id == -1)
            continue;

        char menu_name[128];
        snprintf(menu_name, sizeof(menu_name), "%d: %s", i, pad_name);
        menuId gp_menu = addMenuTo(peripherals_menu, menu_name, true);

        buttonId p_btns[2];
        for(int p = 0; p < MAX_PLAYERS; p++) {
            char p_name[2] = {'1' + p, 0};
            p_btns[p] = addButtonTo(gp_menu, p_name, (void*)set_gamepad_player, &gamepad_assign_args[i][p]);
        }

        int current_player = controls_get_gamepad_player(i);
        checkRadioButton(p_btns[current_player]);
    }

    menuId hotkey_menu = addMenuTo(input_menu, "Hotkeys", false);
    create_input_button_menu(hotkey_menu, "Main key", CONTROL_HOTKEY_BEGIN, CONTROL_HOTKEY_END, true);
    create_input_button_menu(hotkey_menu, "Secondary key", CONTROL_HOTKEY_CMD_BEGIN, CONTROL_HOTKEY_CMD_END, false);
    
    if(ctx->core)
        create_input_button_menu(input_menu, ctx->core->name, ctx->core->control_begin, ctx->core->control_end, true);

    for(int i = 0; i < n_cores; i++){
        if(ctx->core && !strcmp(ctx->core->name, cores[i].name))
            continue;
        create_input_button_menu(input_menu, cores[i].name, cores[i].control_begin, cores[i].control_end, true);
    }

    if(ctx->core){
        menuId core_menu = addMenuTo(emu_menu, "Core", false);

        for(int i = 0; i < MAX_WIDGETS; i++) {
            const widget_t* wid = &ctx->core->widgets[i];
            widget_idxs[i] = i;
            if(!wid->name)
                break;
            addButtonTo(core_menu, wid->name, menu_open_widget, widget_idxs + i);
        }

        if(ctx->core->sound_channels[0].name) {
            menuId apu_menu = addMenuTo(core_menu, "APU", false);
            for(int i = 0; i < MAX_AUDIO_CHANNELS; i++){
                if(ctx->core->sound_channels[i].name){
                    apu_idxs[i] = i;
                    menuId audio_channel_menu = addMenuTo(apu_menu, ctx->core->sound_channels[i].name, false);
                    audio_channel_buttons[i] = addButtonTo(audio_channel_menu, "Enabled", apu_mute_channel, apu_idxs + i);
                    tickButton(audio_channel_buttons[i], true);
                    addButtonTo(audio_channel_menu, "Waveform", menu_open_wave_viewer, apu_idxs + i);
                }
            }
        }
    }

    #ifndef __EMSCRIPTEN__
    menuId netplay_menu = addMenuTo(-1, "Netplay", false);
    menuId netmode_menu = addMenuTo(netplay_menu, "Mode", true);
    netplay_mode_buttons[0] = addButtonTo(netmode_menu, "None", netplay_set_mode, (void*)&netplay_modes[0]);
    netplay_mode_buttons[1] = addButtonTo(netmode_menu, "Host", netplay_set_mode, (void*)&netplay_modes[1]);
    netplay_mode_buttons[2] = addButtonTo(netmode_menu, "Client", netplay_set_mode, (void*)&netplay_modes[2]);
    checkRadioButton(netplay_mode_buttons[netplay_wanted_mode]);
    sprintf(label, "Port: %d", netplay_port);
    netplay_port_button = addButtonTo(netplay_menu, label, netplay_set_port, NULL);
    sprintf(label, "Connect to: %s", netplay_host_ip);
    netplay_ip_button = addButtonTo(netplay_menu, label, netplay_set_ip, NULL);
    sprintf(label, "Input Delay: %d", netplay_input_delay);
    netplay_input_delay_button = addButtonTo(netplay_menu, label, netplay_set_input_delay, NULL);
    #endif

    addButtonTo(-1, "About", (void*)menu_info, NULL);

    free(label);
}

void menu_tick_pause(bool paused){
    tickButton(pause_button, paused);
}
