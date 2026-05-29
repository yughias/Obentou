#include "cores/speccy/speccy.h"

#include "utils/archive.h"

size_t master_clock_counter;

z80_t cpu = {
    .readMemory  = readMemory,
    .writeMemory = writeMemory,
    .readIO      = readIO,
    .writeIO     = writeIO
};

void initAll(){
    z80_init(&cpu);
    initMemory();
    initAudio();
}

void freeAll(){
    freeMemory();
    freeAudio();
}

void emulateCpu(){
    if(!cpu.cycles)
        z80_step(&cpu);
    cpu.cycles--;
}

void sendInterrupt(){
    cpu.INTERRUPT_PENDING = true;
}

void emulateHardware(){
    updateColorFlash();

    for(master_clock_counter = 0; master_clock_counter < CLOCK_PER_FRAME; master_clock_counter++){
        // if(cpu.PC == TRAP_TAPE_ROUTINE_ADDR && cpu.cycles == 0 && tapeFormat == TAP_INSTANT_LOAD){
        //     trapTapeRoutine();
        // }

        emulateCpu();
        emulateAy();
        //sendAudioToDevice();
        //emulateTape();
        
        emulateUlaRender(master_clock_counter);
    }

    sendInterrupt();
}

void* SPECCY_init(const archive_t* rom_archive, const archive_t* bios_archive){
    void* x = malloc(0);

    initAll();

    return x;
}

void SPECCY_run_frame(void* ctx){
    const bool* keys = SDL_GetKeyboardState(NULL);
    emulateKeyboard((const Uint8*)keys);
    emulateHardware();
    renderPixels();
}

bool SPECCY_detect(const archive_t* rom_archive, const archive_t* bios_archive){
    return true;
}