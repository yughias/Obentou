#include "utils/state.h"
#include "utils/serializer.h"
#include "utils/argument.h"

#include "minIni.h"
#include "core.h"

#include "SDL_MAINLOOP.h"

static byte_vec_t state_save(core_ctx_t* ctx) {
    if(!ctx->core)
        return (byte_vec_t){.data = NULL, .size = 0};

    if(ctx->core->savestate){
        printf("saved\n");
        return ctx->core->savestate(ctx->emu);
    } else {
        printf("no savestate for %s\n", ctx->core->name);
    }

    return (byte_vec_t){.data = NULL, .size = 0};
}

static void state_load(core_ctx_t* ctx, byte_vec_t* state) {
    if(!ctx->core)
        return;

    if(ctx->core->loadstate){
        printf("loaded\n");
        ctx->core->loadstate(ctx->emu, state);
    } else {
        printf("no savestate for %s\n", ctx->core->name);
    }
}

bool state_get_autosave() {
    return ini_getbool("STATE", "AUTOLOAD", false, argument_get_ini_path());
}

void state_set_autosave(bool autosave) {
    ini_putbool("STATE", "AUTOLOAD", autosave, argument_get_ini_path());
}

int state_get_active_slot() {
    return ini_getl("STATE", "SLOT", 0, argument_get_ini_path());
}

static void state_get_slot_path(char* slot_path, const char* rom_path, int slot) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%01d.state.bmp", slot);
    path_set_ext(rom_path, slot_path, buf);
}

void state_set_active_slot(int* slot) {
    long val = *slot;
    ini_putl("STATE", "SLOT", val, argument_get_ini_path());
}

void state_save_slot(core_ctx_t* ctx) {
    char path[FILENAME_MAX];
    state_get_slot_path(path, archive_get_path(ctx->rom), state_get_active_slot());
    byte_vec_t state = state_save(ctx);
    int slot = state_get_active_slot();
    SDL_SaveBMP(getMainWindowSurface(), path);
    file_append(path, state.data, state.size);
    byte_vec_free(&state);
}

void state_load_slot(core_ctx_t* ctx) {
    char path[FILENAME_MAX];
    state_get_slot_path(path, archive_get_path(ctx->rom), state_get_active_slot());
    int slot = state_get_active_slot();
    file_t file;
    file_load(&file, path, false);
    if(!file.data)
        return;
    size_t bmp_size = *(uint32_t*)(&file.data[2]);
    state_load(ctx, &(byte_vec_t){.data = file.data + bmp_size, .size = file.size - bmp_size});
    file_delete(&file);
}

void state_save_autosave(core_ctx_t* ctx) {
    int slot = 0;
    int prev_slot = state_get_active_slot();
    state_set_active_slot(&slot);
    state_save_slot(ctx);
    state_set_active_slot(&prev_slot);
}

void state_load_autosave(core_ctx_t* ctx) {
    int slot = 0;
    int prev_slot = state_get_active_slot();
    state_set_active_slot(&slot);
    state_load_slot(ctx);
    state_set_active_slot(&prev_slot);
}

void state_switch_autosave() {
    state_set_autosave(!state_get_autosave());
}