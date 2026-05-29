// #include <frontend.h>
// #include <SDL_MAINLOOP.h>
// #include <tinyfiledialogs.h>
// #include "cores/speccy/loader.h"
// #include "cores/speccy/speccy.h"

// INPUT_DEVICE activeInputDevice = KEYBOARD;
// bool emulationStopped = false;

// void frontend_stopEmulation();
// void frontend_resetEmulation();
// void frontend_saveZ80();
// void frontend_selectROM();
// void frontend_selectZ80();
// void frontend_selectSCR();
// void frontend_instantTAP();
// void frontend_stopTape();
// void frontend_loadTAP();
// void frontend_loadRawTape();
// void frontend_saveRawTape();
// void frontend_useKeyboard();
// void frontend_useKempston();
// void frontend_useCursor();
// void frontend_manageFullScreen();
// void frontend_setScaleNearest();
// void frontend_setScaleLinear();
// void frontend_info();

// void updateHotkeys(const Uint8* keystate){
//     if(isKeyReleased){
//         switch(keyReleased){
//             case SDLK_ESCAPE:
//             if(exitButton == SDLK_UNKNOWN)
//                 frontend_manageFullScreen();
//             break;

//             case SDLK_F1:
//             frontend_stopEmulation();
//             break;

//             case SDLK_F2:
//             frontend_selectROM();
//             break;

//             case SDLK_F3:
//             frontend_selectZ80();
//             break;

//             case SDLK_F4:
//             frontend_saveZ80();
//             break;

//             case SDLK_F5:
//             frontend_manageFullScreen();
//             break;

//             case SDLK_F6:
//             activeInputDevice = (activeInputDevice + 1) % INPUT_DEVICE_LENGTH;
//             break;

//             case SDLK_F8:
//             frontend_stopTape();
//             break;

//             case SDLK_F9:
//             frontend_instantTAP();
//             break;

//             case SDLK_F10:
//             frontend_loadTAP();
//             break;

//             case SDLK_F11:
//             frontend_saveRawTape();
//             break;

//             case SDLK_F12:
//             frontend_loadRawTape();
//             break;
//         }
//     }
// }

// void initMenu(){
//     menuId emulationMenu = addMenuTo(-1, L"Emulation", false);
//     addButtonTo(emulationMenu, L"Stop/Resume", frontend_stopEmulation);
//     addButtonTo(emulationMenu, L"Reset", frontend_resetEmulation);
//     addButtonTo(emulationMenu, L"Save machine state", frontend_saveZ80);
    
//     menuId loadMenu = addMenuTo(-1, L"Load ...", false);
//     addButtonTo(loadMenu, L".rom", frontend_selectROM);
//     addButtonTo(loadMenu, L".z80", frontend_selectZ80);
//     addButtonTo(loadMenu, L".tap", frontend_instantTAP);
//     addButtonTo(loadMenu, L".scr", frontend_selectSCR);

//     menuId tapeMenu = addMenuTo(-1, L"Tape ...", false);
//     addButtonTo(tapeMenu, L"Stop tape", frontend_stopTape);
//     addButtonTo(tapeMenu, L"Load .tap", frontend_loadTAP);
//     addButtonTo(tapeMenu, L"Load raw data", frontend_loadRawTape);
//     addButtonTo(tapeMenu, L"Save raw data", frontend_saveRawTape);

//     menuId inputMenu = addMenuTo(-1, L"Input ...", true);
//     buttonId keyboardBtn = addButtonTo(inputMenu, L"Keyboard", frontend_useKeyboard);
//     addButtonTo(inputMenu, L"Kempston", frontend_useKempston);
//     addButtonTo(inputMenu, L"Cursor", frontend_useCursor);
//     checkRadioButton(keyboardBtn);

//     menuId videoMenu = addMenuTo(-1, L"Video ...", false);
//     addButtonTo(videoMenu, L"Fullscreen", frontend_manageFullScreen);

//     menuId scaleMenu = addMenuTo(videoMenu, L"Filter ...", true);
//     buttonId nearestBtn = addButtonTo(scaleMenu, L"Nearest", frontend_setScaleNearest);
//     addButtonTo(scaleMenu, L"Linear", frontend_setScaleLinear);
//     checkRadioButton(nearestBtn);

//     addButtonTo(-1, L"Info", frontend_info);
// }

// void frontend_stopEmulation(){ emulationStopped = !emulationStopped; }
// void frontend_resetEmulation(){ stopTape(); freeAll(); initAll(); }

// void frontend_saveZ80(){
//     const char* filterPatterns[] = { "*.z80" };
//     const char* selectedFile = tinyfd_saveFileDialog(
//         "save spectrum state\n",
//         NULL,
//         1,
//         filterPatterns,
//         NULL
//     );
//     if(selectedFile)
//         saveState(selectedFile);
// }

// void frontend_selectROM(){
//     const char* filterPatterns[] = { "*.rom" };
//     const char* selectedFile = tinyfd_openFileDialog(
//         "select 16k ROM\n",
//         NULL,
//         1,
//         filterPatterns,
//         NULL,
//         0
//     );
//     if(selectedFile){
//         z80_init(&cpu);
//         loadROM(selectedFile);
//     }
// }

// void frontend_selectZ80(){
//     const char* filterPatterns[] = { "*.z80" };
//     const char* selectedFile = tinyfd_openFileDialog(
//         "select z80 file\n",
//         NULL,
//         1,
//         filterPatterns,
//         NULL,
//         0
//     );
//     if(selectedFile){
//         frontend_resetEmulation();
//         loadState(selectedFile);
//     }
// }

// void frontend_selectSCR(){
//     const char* filterPatterns[] = { "*.scr" };
//     const char* selectedFile = tinyfd_openFileDialog(
//         "select scr file\n",
//         NULL,
//         1,
//         filterPatterns,
//         NULL,
//         0
//     );
//     if(selectedFile){
//         frontend_resetEmulation();
//         loadSCR(selectedFile);
//     }
// }

// void frontend_instantTAP(){
//     const char* filterPatterns[] = { "*.tap" };
//     const char* selectedFile = tinyfd_openFileDialog(
//         "instant load tap file\n",
//         NULL,
//         1,
//         filterPatterns,
//         NULL,
//         0
//     );
//     if(selectedFile){
//         frontend_resetEmulation();
//         loadState("data/autoloader.z80");
//         instantLoadTapFile(selectedFile);
//     }

// }

// void frontend_stopTape(){
//     stopTape();
//     showTapeStoppedMsg();
// }

// void frontend_loadTAP(){
//     const char* filterPatterns[] = { "*.tap" };
//     const char* selectedFile = tinyfd_openFileDialog(
//         "select tap file\n",
//         NULL,
//         1,
//         filterPatterns,
//         NULL,
//         0
//     );
//     if(selectedFile)
//         startLoadTapFile(selectedFile);
// }

// void frontend_loadRawTape(){
//     const char* filterPatterns[] = { "*.bin" };
//     const char* selectedFile = tinyfd_openFileDialog(
//         "select raw tape data\n",
//         NULL,
//         1,
//         filterPatterns,
//         NULL,
//         0
//     );
//     if(selectedFile)
//         startLoadRawTape(selectedFile);
// }

// void frontend_saveRawTape(){
//     const char* filterPatterns[] = { "*.bin" };
//     const char* selectedFile = tinyfd_saveFileDialog(
//         "save raw tape data to file\n",
//         NULL,
//         1,
//         filterPatterns,
//         NULL
//     );
//     if(selectedFile)
//         startSaveRawTape(selectedFile);
// }

// void frontend_useKeyboard(){ activeInputDevice = KEYBOARD; }
// void frontend_useKempston(){ activeInputDevice = KEMPSTON; }
// void frontend_useCursor(){ activeInputDevice = CURSOR; }

// void frontend_manageFullScreen(){
//     fullScreen();
//     exitButton = exitButton == SDLK_ESCAPE ? SDLK_UNKNOWN : SDLK_ESCAPE;
// }

// void frontend_setScaleNearest(){
//     setScaleMode(NEAREST);
// }


// void frontend_setScaleLinear(){
//     setScaleMode(LINEAR);
// }

// void frontend_info(){
//     tinyfd_messageBox(
//                 "ZX SPECTRUM EMULATOR",
//                 "This is a ZX Spectrum 48k emulator by yughias!\n"
//                 "Visit github page: https://github.com/yughias",
//                 "ok",
//                 "info",
//                 0
//     );
// }

// #ifdef __EMSCRIPTEN__

// void emscripten_resetEmulation(){
//     stopTape();
//     freeMemory();
//     resetAy();
//     z80_init(&cpu);
//     initMemory();
// }

// void emscripten_loadTape(){
//     stopTape();
//     startLoadTapFile("file.bin");
// }

// void emscripten_instantLoadTape(){
//     emscripten_resetEmulation();
//     loadState("data/autoloader.z80");
//     instantLoadTapFile("file.bin");
// }

// void emscripten_loadZ80(){
//     emscripten_resetEmulation();
//     loadState("file.bin");
// }

// #endif