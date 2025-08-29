#include "cores/bytepusher/bytepusher.h"

void* bytepusher_init(const char* filename, SDL_AudioDeviceID device_id){
    bytepusher_t* bp = malloc(sizeof(bytepusher_t));
    FILE* fp = fopen(filename, "rb");
    fread(bp->memory, 1, MEMORY_SIZE, fp);
    fclose(fp);
    bp->audio_dev = device_id;
    
    return bp;
}

static void update_keys(bytepusher_t* bp){
    u8* memory = bp->memory;
    const Uint8* ks = SDL_GetKeyboardState(NULL);
    memory[0] = 0;
    memory[1] = 0;

    if(ks[SDL_SCANCODE_1])
        memory[1] |= 1<<1;
    if(ks[SDL_SCANCODE_2])
        memory[1] |= 1<<2;
    if(ks[SDL_SCANCODE_3])
        memory[1] |= 1<<3;
    if(ks[SDL_SCANCODE_4])
        memory[0] |= 1<<4;
    if(ks[SDL_SCANCODE_Q])
        memory[1] |= 1<<4;
    if(ks[SDL_SCANCODE_W])
        memory[1] |= 1<<5;
    if(ks[SDL_SCANCODE_E])
        memory[1] |= 1<<6;
    if(ks[SDL_SCANCODE_R])
        memory[0] |= 1<<5;
    if(ks[SDL_SCANCODE_A])
        memory[1] |= 1<<7;
    if(ks[SDL_SCANCODE_S])
        memory[0] |= 1<<0;
    if(ks[SDL_SCANCODE_D])
        memory[0] |= 1<<1;
    if(ks[SDL_SCANCODE_F])
        memory[0] |= 1<<6;
    if(ks[SDL_SCANCODE_Z])
        memory[0] |= 1<<2;
    if(ks[SDL_SCANCODE_X])
        memory[1] |= 1<<0;
    if(ks[SDL_SCANCODE_C])
        memory[0] |= 1<<3;
    if(ks[SDL_SCANCODE_V])
        memory[0] |= 1<<7;
}

static void play_sound_data(const bytepusher_t* bp){
    const u8* memory = bp->memory;
    const u8* audio_ram = memory + ((memory[6] << 16) | (memory[7] << 8));
    SDL_QueueAudio(bp->audio_dev, audio_ram, 256);
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
}

void bytepusher_run_frame(bytepusher_t* bp){
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

bool bytepusher_detect(const char* filename){
    return SDL_strcasestr(filename, ".bytepusher") != NULL;
}