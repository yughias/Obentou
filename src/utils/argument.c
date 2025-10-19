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

void argument_get(const char** rom_path, const char** bios_path, const char** force_core) {
    *rom_path = NULL;
    *bios_path = NULL;
    *force_core = NULL;
    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_GROUP("Options"),
        OPT_STRING('r', "rom", rom_path, "the rom to be loaded", NULL, 0, 0),
        OPT_STRING('b', "bios", bios_path, "the bios to be loaded, if the system requires it", NULL, 0, 0),
        OPT_STRING('c', "core", force_core, "force core", NULL, 0, 0),
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
    char buf[FILENAME_MAX];
    const char* base_path = SDL_GetBasePath();
    ini_gets(core_name, "BIOS", "", buf, FILENAME_MAX, argument_get_ini_path());
    if(!buf[0])
        path[0] = '\0';
    else {
        if(strlen(buf) > 1 && (buf[0] < 'A' || buf[0] > 'Z') && buf[1] != ':')
            snprintf(path, FILENAME_MAX, "%s%s", base_path, buf);
        else
            snprintf(path, FILENAME_MAX, "%s", buf);
    }
}

void argument_set_default_bios(const char* path, const char* core_name){
    ini_puts(core_name, "BIOS", path, argument_get_ini_path());
}

const char* argument_get_ini_path(){
    static char ini_path[FILENAME_MAX];
    if(ini_path[0])
        return ini_path;

    const char* base_path = SDL_GetBasePath();
    snprintf(ini_path, FILENAME_MAX, "%sconfig.ini", base_path);
    return ini_path;
}

void argument_update_recents(const char* rom_path, const char* bios_path) {
    char rom_key[16];
    char bios_key[16];
    char rom_value[512];
    char bios_value[512];
    char existing_rom[512];
    char existing_bios[512];
    int existing_index = -1;

    const char* ini_path = argument_get_ini_path();

    // find existing rom and bios pair
    for (int i = 0; i < 10; i++) {
        snprintf(rom_key, sizeof(rom_key), "ROM%d", i);
        snprintf(bios_key, sizeof(bios_key), "BIOS%d", i);

        ini_gets("RECENTS", rom_key, "", existing_rom, sizeof(existing_rom), ini_path);
        ini_gets("RECENTS", bios_key, "", existing_bios, sizeof(existing_bios), ini_path);

        if (!strcmp(existing_rom, rom_path) && !strcmp(existing_bios, bios_path)) {
            existing_index = i;
            break;
        }
    }

    // trim spaces
    for(int i = existing_index; existing_index != -1 && i < 9; i++){
        snprintf(rom_key, sizeof(rom_key), "ROM%d", i + 1);
        snprintf(bios_key, sizeof(bios_key), "BIOS%d", i + 1);

        ini_gets("RECENTS", rom_key, "", rom_value, sizeof(rom_value), ini_path);
        ini_gets("RECENTS", bios_key, "", bios_value, sizeof(bios_value), ini_path);
        ini_puts("RECENTS", rom_key, "", ini_path);
        ini_puts("RECENTS", bios_key, "", ini_path);

        snprintf(rom_key, sizeof(rom_key), "ROM%d", i);
        snprintf(bios_key, sizeof(bios_key), "BIOS%d", i);

        ini_puts("RECENTS", rom_key, rom_value, ini_path);
        ini_puts("RECENTS", bios_key, bios_value, ini_path);
    }

    // downshift
    for(int i = 9; i > 0; i--){
        snprintf(rom_key, sizeof(rom_key), "ROM%d", i - 1);
        snprintf(bios_key, sizeof(bios_key), "BIOS%d", i - 1);

        ini_gets("RECENTS", rom_key, "", rom_value, sizeof(rom_value), ini_path);
        ini_gets("RECENTS", bios_key, "", bios_value, sizeof(bios_value), ini_path);
        
        snprintf(rom_key, sizeof(rom_key), "ROM%d", i);
        snprintf(bios_key, sizeof(bios_key), "BIOS%d", i);

        ini_puts("RECENTS", rom_key, rom_value, ini_path);
        ini_puts("RECENTS", bios_key, bios_value, ini_path);
    }

    // insert new rom + bios
    ini_puts("RECENTS", "ROM0", rom_path, ini_path);
    ini_puts("RECENTS", "BIOS0", bios_path, ini_path);
}
