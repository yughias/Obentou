#include "utils/argument.h"

#include "SDL_MAINLOOP.h"
#include "minIni.h"

#include "argparse.h"


static const char *const usages[] = {
    "obentou.exe <rom path> [options]",
    "obentou.exe [options]",
    NULL,
};

static const char brief_description[] = "\nObentou is a multi system emulator supporting different emulators";
static const char description[] = "";

void argument_get(const char** rom_path, const char** bios_path, const char** force_core, int* autorun) {
    *rom_path = NULL;
    *bios_path = NULL;
    *force_core = NULL;
    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_GROUP("Options"),
        OPT_STRING('r', "rom", rom_path, "the rom to be loaded", NULL, 0, 0),
        OPT_STRING('b', "bios", bios_path, "the bios to be loaded, if the system requires it", NULL, 0, 0),
        OPT_STRING('c', "core", force_core, "force core", NULL, 0, 0),
        OPT_INTEGER('a', "autorun", autorun, "auto execute rom and quit after X frames", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse, brief_description, description);
    int argc = getArgc();
    const char **argv = (const char**)getArgvs();
    argc = argparse_parse(&argparse, argc, argv);

    if(argc)
        *rom_path = argv[0];    
}

void argument_get_default_bios(char* path, const char* core_name){
    argument_get_path(path, core_name, "BIOS");
}

void argument_set_default_bios(const char* path, const char* core_name){
    argument_set_path(path, core_name, "BIOS");
}

const char* argument_get_ini_path(){
    static char ini_path[FILENAME_MAX];
    if(ini_path[0])
        return ini_path;

    const char* base_path = SDL_GetBasePath();
    snprintf(ini_path, FILENAME_MAX, "%sconfig.ini", base_path);
    return ini_path;
}

static void get_fullpath(char* out_path, const char* in_path){
    if(!in_path || !in_path[0]){
        out_path[0] = 0;
    } else {
        _fullpath(out_path, in_path, FILENAME_MAX);
    }
}

void argument_update_recents(const char* rom_path_, const char* bios_path_) {
    char rom_key[16];
    char bios_key[16];
    char rom_value[FILENAME_MAX];
    char bios_value[FILENAME_MAX];
    char existing_rom[FILENAME_MAX];
    char existing_bios[FILENAME_MAX];
    char rom_path[FILENAME_MAX];
    char bios_path[FILENAME_MAX];
    int existing_index = -1;

    get_fullpath(rom_path, rom_path_);
    get_fullpath(bios_path, bios_path_);

    // find existing rom and bios pair
    for (int i = 0; i < 10; i++) {
        snprintf(rom_key, sizeof(rom_key), "ROM%d", i);
        snprintf(bios_key, sizeof(bios_key), "BIOS%d", i);

        argument_get_path(existing_rom, "RECENTS", rom_key);
        argument_get_path(existing_bios, "RECENTS", bios_key);

        if (!strcmp(existing_rom, rom_path) && !strcmp(existing_bios, bios_path)) {
            existing_index = i;
            break;
        }
    }

    // trim spaces
    for(int i = existing_index; existing_index != -1 && i < 9; i++){
        snprintf(rom_key, sizeof(rom_key), "ROM%d", i + 1);
        snprintf(bios_key, sizeof(bios_key), "BIOS%d", i + 1);

        argument_get_path(rom_value, "RECENTS", rom_key);
        argument_get_path(bios_value, "RECENTS", bios_key);

        snprintf(rom_key, sizeof(rom_key), "ROM%d", i);
        snprintf(bios_key, sizeof(bios_key), "BIOS%d", i);

        argument_set_path(rom_value, "RECENTS", rom_key);
        argument_set_path(bios_value, "RECENTS", bios_key);
    }

    // downshift
    for(int i = 9; i > 0; i--){
        snprintf(rom_key, sizeof(rom_key), "ROM%d", i - 1);
        snprintf(bios_key, sizeof(bios_key), "BIOS%d", i - 1);

        argument_get_path(rom_value, "RECENTS", rom_key);
        argument_get_path(bios_value, "RECENTS", bios_key);
        
        snprintf(rom_key, sizeof(rom_key), "ROM%d", i);
        snprintf(bios_key, sizeof(bios_key), "BIOS%d", i);

        argument_set_path(rom_value, "RECENTS", rom_key);
        argument_set_path(bios_value, "RECENTS", bios_key);
    }

    // insert new rom + bios
    argument_set_path(rom_path, "RECENTS", "ROM0");
    argument_set_path(bios_path, "RECENTS", "BIOS0");
}


void argument_get_path(char* path, const char* section, const char* key){
    char buf[FILENAME_MAX];
    ini_gets(section, key, "", buf, FILENAME_MAX, argument_get_ini_path());
    get_fullpath(path, buf);
}

void argument_set_path(const char* path, const char* section, const char* key){
    char buf[FILENAME_MAX];
    if(path && path[0])
        _fullpath(buf, path, FILENAME_MAX);
    else
        buf[0] = 0;
    ini_puts(section, key, buf, argument_get_ini_path());
}