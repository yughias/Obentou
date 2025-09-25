#include "peripherals/argument.h"

#include "SDL_MAINLOOP.h"

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

    if(!(*rom_path) && !(*bios_path)){
        argparse_usage(&argparse);
        exit(EXIT_SUCCESS);
    }
    
}