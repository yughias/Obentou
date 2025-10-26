#include "cores/bytepusher/bytepusher.h"
#include "utils/sound.h"
#include "utils/controls.h"
#include "utils/archive.h"

#include "SDL_MAINLOOP.h"

void* BYTEPUSHER_init(const archive_t* rom_archive, const archive_t* bios_archive){
    bytepusher_t* bp = malloc(sizeof(bytepusher_t));
    file_t* rom = archive_get_file_by_ext(rom_archive, "bytepusher");
    if(!rom)
        rom = archive_get_file_by_ext(rom_archive, "bp");
    memcpy(bp->memory, rom->data, rom->size);
    
    return bp;
}

static void update_keys(bytepusher_t* bp){
    u8* memory = bp->memory;
    memory[0] = 0;
    memory[1] = 0;

    for(int i = 0; i < 16; i++){
        control_t btn = CONTROL_BYTEPUSHER_0 + i;
        bool pressed = controls_pressed(btn);
        memory[i < 8] |= pressed << (i & 7);
    }
}

static void play_sound_data(const bytepusher_t* bp){
    const u8* memory = bp->memory;
    const u8* audio_ram = memory + ((memory[6] << 16) | (memory[7] << 8));
    sound_queue_samples(audio_ram, 256);
}

static void render(const bytepusher_t* bp){
    const u8* memory = bp->memory;
    const u8* vram = memory + (memory[5]<<16);
    for(int y = 0; y < BYTEPUSHER_H; y++){
        for(int x = 0; x < BYTEPUSHER_W; x++){
            u8 col = vram[x + y*BYTEPUSHER_W];
            u8 blue = col % 6;
            u8 green = (col / 6) % 6;
            u8 red = (col / 36) % 6;
            pixels[x+y*BYTEPUSHER_W] = color(red*255/6, green*255/6, blue*255/6);
        }
    }
    renderPixels();
}

void BYTEPUSHER_run_frame(bytepusher_t* bp){
    update_keys(bp);

    u8* memory = bp->memory;
    u8* pc =  memory + (memory[2]<<16 | memory[3]<<8 | memory[4]);
    for(int i = 0; i < HERTZ_PER_FRAME; i++){
        memory[pc[3]<<16 | pc[4]<<8 | pc[5]] = memory[pc[0]<<16 | pc[1]<<8 | pc[2]];
        pc = memory + (pc[6]<<16 | pc[7]<<8 | pc[8]);
    }

    render(bp);
    play_sound_data(bp);
}

bool BYTEPUSHER_detect(const archive_t* rom_archive, const archive_t* bios_archive){
    bool out = false;
    out |= archive_get_file_by_ext(rom_archive, "bytepusher") != NULL;
    out |= archive_get_file_by_ext(rom_archive, "bp") != NULL;
    return out;
}

byte_vec_t BYTEPUSHER_savestate(void* ctx){
    byte_vec_t state;
    byte_vec_init(&state);
    serialize_bytepusher_t(ctx, &state);
    byte_vec_shrink(&state);
    return state;
}

void BYTEPUSHER_loadstate(void* ctx, byte_vec_t* state){
    deserialize_bytepusher_t(ctx, state->data);
}