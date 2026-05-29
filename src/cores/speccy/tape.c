// #include "cores/speccy/speccy.h"
// #include <tinyfiledialogs.h>

// #include <stdio.h>

// #define DATA                  0xFF
// #define HEADER                0x00
// #define HEADER_PULSES         8063
// #define DATA_PULSES           3223
// #define PILOT_TONE_T_STATES   2168
// #define SYNC_PULSE_1_T_STATES  667
// #define SYNC_PULSE_2_T_STATES  735
// #define BIT_0_T_STATES         855
// #define BIT_1_T_STATES        1710
// #define PAUSE_DURATION        1000

// typedef enum {BLOCK_START, PILOT_TONE, SYNC_TONE, DATA_LOAD, PAUSE} TAP_LOADER_STATE;

// FILE* tape_file_ptr;
// TAPE_TYPE tapeFormat = NO_TAPE;

// TAP_LOADER_STATE tapLoaderState;
// uint16_t blockLength;
// uint8_t byte;
// uint8_t bitIdx;
// size_t pulses;
// size_t pulse_duration;
// size_t clock_counter;
// bool polarity = false;

// void sendSignal(bool bit){
//     ULA &= ~(0b10000);
//     ULA |= bit << 4;
// }

// void tapLoaderEmitter(){
//     sendSignal(polarity);
//     clock_counter++;
//     if(clock_counter == pulse_duration){
//         clock_counter = 0;
//         pulses--;
//         polarity = !polarity;
//     }
// }

// void fetchBitFromTap(){
//     bool bit = byte & 0x80;
//     byte <<= 1;
//     bitIdx++;
//     if(bitIdx == 8){
//         bitIdx = 0;
//         blockLength--;
//         if(blockLength != 0)
//             fread(&byte, 1, 1, tape_file_ptr);
//     }
//     clock_counter = 0;
//     pulses = 2;
//     if(bit)
//         pulse_duration = BIT_1_T_STATES;
//     else
//         pulse_duration = BIT_0_T_STATES;
// }

// void trapTapeRoutine(){
//     uint8_t blockType = cpu.AF_ >> 8;
//     bool isLoad = cpu.AF_ & 1;
//     uint16_t address = cpu.IX;
//     uint16_t length = cpu.DE;

//     fread(&blockLength, 2, 1, tape_file_ptr);
//     uint8_t tapBlockType;
//     fread(&tapBlockType, 1, 1, tape_file_ptr);

//     if(tapBlockType != blockType){
//         stopTape();
//         printf("different block type in instant loading!\n");
//         return;
//     }

//     if(isLoad){
//         uint8_t checksum = blockType;
//         for(size_t n_bytes = 0; n_bytes < blockLength - 2; n_bytes++){
//             fread(&byte, 1, 1, tape_file_ptr);
//             cpu.writeMemory(&cpu, address, byte);
//             address = (address + 1) % MEMORY_SIZE;
//             checksum ^= byte;
//         }
//         fread(&byte, 1, 1, tape_file_ptr);
//         if(checksum != byte){
//             printf("different checksum in instant loading!\n");
//             return;
//         }
//     }


//     size_t actual = ftell(tape_file_ptr);
//     size_t filesize;
//     fseek(tape_file_ptr, 0, SEEK_END);
//     filesize = ftell(tape_file_ptr);
//     fseek(tape_file_ptr, actual, SEEK_SET);

//     if(actual == filesize){
//         stopTape();
//         showTapeStoppedMsg();
//     }

//     cpu.AF |= 1;
//     cpu.PC = 0x05E2;
// }

// void stopTape(){
//     if(tapeFormat == RAW_TAPE_SAVE)
//         fwrite(&byte, 1, 1, tape_file_ptr);

//     if(tape_file_ptr){
//         fclose(tape_file_ptr);
//         tapeFormat = NO_TAPE;
//         tape_file_ptr = NULL;
//     }
// }

// void startSaveRawTape(const char* filename){
//     stopTape();
//     tape_file_ptr = fopen(filename, "wb");
//     bitIdx = 0;
//     tapeFormat = RAW_TAPE_SAVE;
// }

// void startLoadRawTape(const char* filename){
//     stopTape();
//     tape_file_ptr = fopen(filename, "rb");
//     bitIdx = 0;
//     fread(&byte, 1, 1, tape_file_ptr);
//     tapeFormat = RAW_TAPE_LOAD;
// }

// void startLoadTapFile(const char* filename){
//     stopTape();
//     tape_file_ptr = fopen(filename, "rb");
//     tapeFormat = TAP_TAPE_LOAD;
//     tapLoaderState = BLOCK_START;
// }

// void instantLoadTapFile(const char* filename){
//     startLoadTapFile(filename);
//     tapeFormat = TAP_INSTANT_LOAD;
// }

// void emulateTape(){
//     switch(tapeFormat){
//         bool bit;

//         case RAW_TAPE_SAVE:
//         bit = ULA & 0b1000;
//         byte <<= 1;
//         byte |= bit;
//         bitIdx++;
//         if(bitIdx == 8){
//             fwrite(&byte, 1, 1, tape_file_ptr);
//             bitIdx = 0;
//             byte = 0;
//         }
//         break;

//         case RAW_TAPE_LOAD:
//         bit = byte & 0x80;
//         byte <<= 1;
//         bitIdx++;
//         if(bitIdx == 8){
//             if(!fread(&byte, 1, 1, tape_file_ptr)){
//                 stopTape();
//                 showTapeStoppedMsg();
//             }
//             bitIdx = 0;
//         }
//         sendSignal(bit);
//         break;

//         case TAP_TAPE_LOAD:
//         switch(tapLoaderState){
//             case BLOCK_START:
//             if(!fread(&blockLength, 2, 1, tape_file_ptr)){
//                 stopTape();
//                 showTapeStoppedMsg();
//             }
//             fread(&byte, 1, 1, tape_file_ptr);
//             bitIdx = 0;
//             clock_counter = 0;
//             pulse_duration = PILOT_TONE_T_STATES;
//             polarity = true;
//             if(byte == DATA)
//                 pulses = DATA_PULSES;
//             if(byte == HEADER)
//                 pulses = HEADER_PULSES;
//             tapLoaderState = PILOT_TONE;
//             break;

//             case PILOT_TONE:
//             if(pulses == 0){
//                 tapLoaderState = SYNC_TONE;
//                 pulses = 2;
//             }
//             tapLoaderEmitter();
//             break;

//             case SYNC_TONE:
//             if(pulses == 2){
//                 pulse_duration = SYNC_PULSE_2_T_STATES;
//             }
//             if(pulses == 1){
//                 pulse_duration = SYNC_PULSE_2_T_STATES;
//             }
//             if(pulses == 0){
//                 tapLoaderState = DATA_LOAD;
//                 fetchBitFromTap();
//             }
//             tapLoaderEmitter();
//             break;

//             case DATA_LOAD:
//             if(blockLength == 0 && pulses == 0){
//                 tapLoaderState = PAUSE;
//                 pulses = 1;
//                 clock_counter = 0;
//                 pulse_duration = PAUSE_DURATION;
//             }
//             if(blockLength != 0 && pulses == 0)
//                 fetchBitFromTap();
//             tapLoaderEmitter();
//             break;

//             case PAUSE:
//             if(pulses == 0)
//                 tapLoaderState = BLOCK_START;
//             tapLoaderEmitter();
//             break;
//         }
//         break;

//         default:
//         break;
//     }
// }

// void showTapeStoppedMsg(){
//     #ifdef __EMSCRIPTEN__
//         return;
//     #endif

//     tinyfd_messageBox(
//                 "ZX SPECTRUM TAPE",
//                 "Tape has been stopped!",
//                 "ok",
//                 "info",
//                 0
//     );
// }