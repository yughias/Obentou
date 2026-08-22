#include "cores/gba/gamepak.h"

#include "cores/gba/gba.h"
#include "cores/gba/sram.h"
#include "cores/gba/flash.h"
#include "cores/gba/eeprom.h"

#include "zip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void gba_load_gamepak(gba_gamepak_t* gamepak, const archive_t* rom_archive){
    file_t* f = archive_get_file_by_ext(rom_archive, "gba");
    if (f) {
        gamepak->ROM_SIZE = f->size;
        gamepak->ROM = malloc(gamepak->ROM_SIZE);
        memcpy(gamepak->ROM, f->data, gamepak->ROM_SIZE);
    } else {
        printf("can't open rom\n");
        gamepak->ROM_SIZE = 0;
        gamepak->ROM = NULL;
        gamepak->internalData = NULL;
        gamepak->savMemory = NULL;
        gamepak->type = NO_GAMEPAK;
        return;
    }


    bool nes_mirror = gamepak->ROM_SIZE == 1 << 20;
    if(nes_mirror){
        // classic nes games that are 1Mb in size requires mirror!
        const int n_mirror = 4;
        gamepak->ROM = realloc(gamepak->ROM, gamepak->ROM_SIZE*n_mirror);
        for(int i = 1; i < n_mirror; i++)
            memcpy(gamepak->ROM + gamepak->ROM_SIZE*i, gamepak->ROM, gamepak->ROM_SIZE);
        gamepak->ROM_SIZE *= n_mirror;
    }

    gba_setup_gamepak_type(gamepak);

    if(gamepak->type != GAMEPAK_ROM_ONLY){
        file_t sav_file;
        char sav_path[FILENAME_MAX];
        path_set_ext(archive_get_path(rom_archive), sav_path, "sav");

        if(file_load(&sav_file, sav_path, false)){
            gamepak->savMemory = malloc(sav_file.size);
            gamepak->savMemorySize = sav_file.size;
            memcpy(gamepak->savMemory, sav_file.data, sav_file.size);
            file_delete(&sav_file);
        }
    }
}

void gba_setup_gamepak_type(gba_gamepak_t* gamepak){
    const char sram_tags[2][32] = {"SRAM_V", "SRAM_F_V"};
    const char flash64k_tags[2][32] = { "FLASH_V", "FLASH512_V"};
    const char flash128k_tag[32] = "FLASH1M_V";

    db_hash db_result = gba_db_search(&gamepak->ROM[GAMECODE_OFFSET]);
    if(db_result != DB_NOT_FOUND){
        gba_setup_save_memory_with_db(gamepak, db_result);
        return;
    }
    
    for(int i = 0; i < 2; i++)
        if(gba_rom_contains(gamepak->ROM, flash64k_tags[i], gamepak->ROM_SIZE)){
            gba_flash_setup_memory(gamepak, FLASH_64K_SIZE, DEFAULT_64K_ID_CODE);
            return;
        }

    if(gba_rom_contains(gamepak->ROM, flash128k_tag, gamepak->ROM_SIZE)){
        gba_flash_setup_memory(gamepak, FLASH_128K_SIZE, DEFAULT_128K_ID_CODE);
        return;
    }

    for(int i = 0; i < 2; i++)
        if(gba_rom_contains(gamepak->ROM, sram_tags[i], gamepak->ROM_SIZE)){
            gba_sram_setup_memory(gamepak, SRAM_SIZE);
            return;
        }

    gba_setup_rom_only_memory(gamepak);
}

bool gba_rom_contains(u8* rom, const char* string, size_t rom_size){
    size_t offset = 0;
    size_t str_len = strlen(string);
    while(offset + str_len < rom_size){
        if(!memcmp(rom + offset, string, str_len))
            return true;
        offset += 1;
    }

    return false;
}

void gba_setup_rom_only_memory(gba_gamepak_t* gamepak){
    gamepak->type = GAMEPAK_ROM_ONLY;
    printf("ROM ONLY!\n");
}

void gba_free_gamepak(gba_gamepak_t* gamepak){
    if(gamepak->ROM)
        free(gamepak->ROM);
    if(gamepak->savMemory)
        free(gamepak->savMemory);
    if(gamepak->internalData)
        free(gamepak->internalData);
}

void gba_setup_save_memory_with_db(gba_gamepak_t* gamepak, db_hash hash){
    switch(hash){
        case 0xF:
        gba_setup_rom_only_memory(gamepak);
        break;

        case 0xE:
        gba_sram_setup_memory(gamepak, SRAM_SIZE);
        break;

        case 0xD: case 0xC: case 0xB: case 0xA:
        case 0x9: case 0x8: case 0x7: case 0x6:
        case 0x5: case 0x4:
        gba_flash_setup_memory(gamepak, gba_db_get_size(hash), gba_db_get_id_code(hash));
        break;
    
        case 0x0: case 0x1: case 0x2: case 0x3:
        printf("EEPROM %zu DETECTED!\n", gba_db_get_size(hash));
        gba_eeprom_setup_memory(gamepak, gba_db_get_size(hash));
        break;

        default:
        printf("UNKNOWN DB HASH!\n");
        gba_setup_rom_only_memory(gamepak);
        break;
    }
}

void gba_update_waitstates(gba_gamepak_t* gamepak, arm7tdmi_t* cpu, u16 waitcnt_reg){
    gba_t* gba = cpu->master;
    prefetcher_t* prefetcher = &gba->prefetcher;
    static const u8 wait_n[4] = {4, 3, 2, 8};
    static const u8 wait0_s[2] = {2, 1};
    static const u8 wait1_s[2] = {4, 1};
    static const u8 wait2_s[2] = {8, 1};

    gamepak->sram_wait = wait_n[waitcnt_reg & 0b11];
    prefetcher->enabled = (waitcnt_reg >> 14) & 0b1;
    prefetcher->address = 0;
    prefetcher->size = 0;
    prefetcher->cycle_counter = cpu->cycles;

    gamepak->waitstates[0][N_WAIT_IDX] = wait_n[(waitcnt_reg >> 2) & 0b11];
    gamepak->waitstates[0][S_WAIT_IDX] = wait0_s[(waitcnt_reg >> 4) & 0b1];

    gamepak->waitstates[1][N_WAIT_IDX] = wait_n[(waitcnt_reg >> 5) & 0b11];
    gamepak->waitstates[1][S_WAIT_IDX] = wait1_s[(waitcnt_reg >> 7) & 0b1];

    gamepak->waitstates[2][N_WAIT_IDX] = wait_n[(waitcnt_reg >> 8) & 0b11];
    gamepak->waitstates[2][S_WAIT_IDX] = wait2_s[(waitcnt_reg >> 10) & 0b1];
}

void serialize_gba_gamepak_t(gba_gamepak_t* gamepak, byte_vec_t* vec) {
    byte_vec_push_array(vec, (u8*)gamepak->waitstates, sizeof(gamepak->waitstates));
    byte_vec_push(vec, gamepak->sram_wait);

    switch (gamepak->type) {
        case GAMEPAK_SRAM:
        byte_vec_push_array(vec, gamepak->savMemory, gamepak->savMemorySize);
        break;

        case GAMEPAK_EEPROM:
        byte_vec_push_array(vec, gamepak->savMemory, gamepak->savMemorySize);
        byte_vec_push_array(vec, gamepak->internalData, sizeof(eeprom_t));
        break;

        case GAMEPAK_FLASH:
        byte_vec_push_array(vec, gamepak->savMemory, gamepak->savMemorySize);
        byte_vec_push_array(vec, gamepak->internalData, sizeof(flash_t));
        break;
    
        default:
        break;
    }
}

u8* deserialize_gba_gamepak_t(gba_gamepak_t* gamepak, u8* data, u8* end) {
    if (data + sizeof(gamepak->waitstates) > end) return NULL;
    memcpy(gamepak->waitstates, data, sizeof(gamepak->waitstates));
    data += sizeof(gamepak->waitstates);

    if (data + sizeof(gamepak->sram_wait) > end) return NULL;
    memcpy(&gamepak->sram_wait, data, sizeof(gamepak->sram_wait));
    data += sizeof(gamepak->sram_wait);

    switch (gamepak->type) {
        case GAMEPAK_SRAM:
        if (data + gamepak->savMemorySize > end) return NULL;
        memcpy(gamepak->savMemory, data, gamepak->savMemorySize);
        data += gamepak->savMemorySize;
        break;

        case GAMEPAK_EEPROM:
        if (data + gamepak->savMemorySize > end) return NULL;
        memcpy(gamepak->savMemory, data, gamepak->savMemorySize);
        data += gamepak->savMemorySize;
        if (data + sizeof(eeprom_t) > end) return NULL;
        memcpy(gamepak->internalData, data, sizeof(eeprom_t));
        data += sizeof(eeprom_t);
        break;

        case GAMEPAK_FLASH:
        if (data + gamepak->savMemorySize > end) return NULL;
        memcpy(gamepak->savMemory, data, gamepak->savMemorySize);
        data += gamepak->savMemorySize;
        if (data + sizeof(flash_t) > end) return NULL;
        memcpy(gamepak->internalData, data, sizeof(flash_t));
        data += sizeof(flash_t);
        break;
    
        default:
        break;
    }

    return data;
}