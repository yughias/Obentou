#ifndef __TAPE_H__
#define __TAPE_H__

#define TRAP_TAPE_ROUTINE_ADDR 0x56B

typedef enum {NO_TAPE, RAW_TAPE_SAVE, RAW_TAPE_LOAD, TAP_TAPE_LOAD, TAP_INSTANT_LOAD} TAPE_TYPE;
extern TAPE_TYPE tapeFormat;

void startSaveRawTape(const char*);
void startLoadRawTape(const char*);
void emulateTape();
void stopTape();
void showTapeStoppedMsg();

void startLoadTapFile(const char*);
void instantLoadTapFile(const char*);
void trapTapeRoutine();

#endif