#include "SDL_MAINLOOP.h"
#include "utils/sound.h"
#include "utils/controls.h"
#include "utils/camera.h"
#include "utils/argument.h"
#include "utils/menu.h"
#include "utils/state.h"

#include "core.h"
#include "math.h"

static core_ctx_t emu_ctx;

DEFINE_VEC(state, byte_vec_t);

state_t states;

byte_vec_t xor_diff(const byte_vec_t* cur, const byte_vec_t* prev) {
    byte_vec_t out;
    out.size = cur->size;
    out.allocated = cur->size;
    out.data = malloc(out.allocated);

    for (size_t i = 0; i < cur->size; i++)
        out.data[i] = cur->data[i] ^ prev->data[i];

    return out;
}

byte_vec_t rle0_encode(const byte_vec_t* in) {
    byte_vec_t out;
    out.size = 0;
    out.allocated = in->size * 3;
    out.data = malloc(out.allocated);

    size_t i = 0;
    while (i < in->size) {
        if (in->data[i] == 0x00) {
            size_t run = 1;
            while (i + run < in->size &&
                   in->data[i + run] == 0x00 &&
                   run < 0xFFFF)
                run++;

            out.data[out.size++] = 0x00;
            out.data[out.size++] = (uint8_t)(run & 0xFF);
            out.data[out.size++] = (uint8_t)((run >> 8) & 0xFF);

            i += run;
        } else {
            out.data[out.size++] = in->data[i++];
        }
    }

    byte_vec_shrink(&out);
    return out;
}

byte_vec_t rle0_decode(const byte_vec_t* in, size_t expected_out_size) {
    byte_vec_t out;
    out.size = 0;
    out.allocated = expected_out_size;
    out.data = malloc(out.allocated);

    size_t i = 0;
    while (i < in->size) {
        if (in->data[i] == 0x00) {
            if (i + 2 >= in->size){
                printf("RLE0 corrupted\n");
                break; // corrupted
            }

            uint16_t run = in->data[i+1] | (in->data[i+2] << 8);
            i += 3;

            memset(out.data + out.size, 0x00, run);
            out.size += run;
        } else {
            out.data[out.size++] = in->data[i++];
        }
    }

    return out;
}

byte_vec_t savestate_step_back(const byte_vec_t* current, const byte_vec_t* diff_rle) {
    // decode RLE
    byte_vec_t decoded = rle0_decode(diff_rle, current->size);

    // xor to reconstruct previous
    byte_vec_t prev = xor_diff(current, &decoded);

    byte_vec_free(&decoded);
    return prev;
}

static void obentou_exit(){
    sound_close();
    camera_close();
    controls_free();
    core_ctx_close(&emu_ctx);
    controls_save_maps();
}

void setup(){
    state_init(&states);
    setTitle("MULTI-SYSTEM EMU");
    onExit = obentou_exit;
    controls_load_maps();

    const char* rom_path;
    const char* bios_path;
    const char* force_core;
    argument_get(&rom_path, &bios_path, &force_core);

    core_ctx_init(&emu_ctx, rom_path, bios_path, force_core);

    core_restart(&emu_ctx);

    setWindowSize(512, 512);
}

void loop(){
    const bool* ks = SDL_GetKeyboardState(NULL);
    // --- ROLLBACK LOGIC FIX ---
    if(ks[SDL_SCANCODE_1] && states.size >= 2){
        //printf("Starting rollback...\n");

        // The current state is at index states.size - 1
        // The *differential* state is at index states.size - 2

        byte_vec_t* cur = &states.data[states.size - 1]; // Current state (full or differential)
        byte_vec_t* diff = &states.data[states.size - 2]; // Differential state

        byte_vec_t prev = savestate_step_back(cur, diff);

        // 1. Free the memory for the differential state (states.data[size - 2])
        byte_vec_free(diff);
        
        // 2. Free the memory for the state that was being replaced (states.data[size - 1])
        byte_vec_free(cur);
        
        // 3. Move the reconstructed *prev* state into the position of the *differential* state
        *diff = prev; // This is the new current state!

        // 4. DECREASE THE SIZE BY TWO to remove the differential and the old 'current' state
        states.size -= 1; // It was previously at index states.size - 2, now it is states.size - 1
        
        // The original state at states.data[states.size - 1] is now the last valid state.
        // Note: 'prev' is the same as 'diff' now, which points to the new current state.
        emu_ctx.core->loadstate(emu_ctx.emu, &prev); 
    } else {
        if (emu_ctx.core && emu_ctx.core->savestate) {
            byte_vec_t state = emu_ctx.core->savestate(emu_ctx.emu);

            if (states.size) {
                byte_vec_t* last = &states.data[states.size - 1];

                byte_vec_t xored = xor_diff(&state, last);
                byte_vec_t rle   = rle0_encode(&xored);

                byte_vec_free(last);
                *last = rle;

                byte_vec_free(&xored);
            }

            state_push(&states, state);
        }
    }

    int sum = 0;
    for(int i = 0; i < states.size; i++){
        sum += states.data[i].allocated;
    }

    //printf("number of states: %llu, size: %f Mb\n", states.size, sum / powf(2, 20));
    //printf("%f\n", 1000.0f/deltaTime);

    controls_update();
    camera_update();

    if(hotkeys_released(CONTROL_HOTKEY_SAVESTATE)){
        state_save_slot(&emu_ctx);
    }

    if(hotkeys_released(CONTROL_HOTKEY_LOADSTATE)){
        state_load_slot(&emu_ctx);
    }

    if(controls_double_click()){
        menu_fullscreen();
    }

    if(hotkeys_released(CONTROL_HOTKEY_OPEN))
        menu_open_rom(&emu_ctx);

    if(hotkeys_released(CONTROL_HOTKEY_OPEN_BIOS))
        menu_open_bios(&emu_ctx);

    if(hotkeys_released(CONTROL_HOTKEY_SPEEDUP) && emu_ctx.speed_level < 3){
        ctx_args_t speed_args = {.ctx = &emu_ctx, .value = emu_ctx.speed_level + 1};
        core_ctx_set_speed(&speed_args);
    }

    if(hotkeys_released(CONTROL_HOTKEY_SLOWDOWN) && emu_ctx.speed_level != 0){
        ctx_args_t speed_args = {.ctx = &emu_ctx, .value = emu_ctx.speed_level - 1};
        core_ctx_set_speed(&speed_args);
    }

    if(hotkeys_released(CONTROL_HOTKEY_RESET))
        core_restart(&emu_ctx);

    if(hotkeys_released(CONTROL_HOTKEY_PAUSE))
        core_switch_pause(&emu_ctx);

    core_ctx_run_frame(&emu_ctx);
}