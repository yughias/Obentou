#include "peripherals/file.h"

#include "SDL3/SDL.h"

void file_load(file_t* file, const char* filename){
    memset(file, 0, sizeof(file_t));
    strcpy(file->path, filename);
    file->data = SDL_LoadFile(filename, &file->size);
}

void file_save(file_t* file){
    SDL_SaveFile(file->path, file->data, file->size);
}

void file_delete(file_t* file){
    SDL_free(file->data);
}

const char* path_get_ext(const char* path){
    const char* dot = strrchr(path, '.');
    if(!dot) return NULL;
    return dot + 1;
}