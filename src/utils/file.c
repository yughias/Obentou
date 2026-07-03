#include "utils/file.h"

#include "SDL3/SDL.h"

#include <stdlib.h>

bool file_load(file_t* file, const char* filename, bool show_msg){
    memset(file, 0, sizeof(file_t));
    strcpy(file->path, filename);
    SDL_IOStream* fptr = SDL_IOFromFile(filename, "rb");
    if(!fptr)
        return false;
    SDL_SeekIO(fptr, 0, SDL_IO_SEEK_END);
    file->size = SDL_TellIO(fptr);
    SDL_SeekIO(fptr, 0, SDL_IO_SEEK_SET);
    file->data = malloc(file->size);
    SDL_ReadIO(fptr, file->data, file->size);
    SDL_CloseIO(fptr);
    if(!file->data && show_msg){
        char* buf = (char*)malloc(1024);
        snprintf(buf, 1024, "Failed to load %s", filename);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Load File Error", buf, NULL);
        free(buf);
    }
    return file->data;
}

void file_save(const char* filename, u8* data, size_t size){
    SDL_SaveFile(filename, data, size);
}

void file_append(const char* filename, u8* data, size_t size){
    SDL_IOStream* fptr = SDL_IOFromFile(filename, "ab+");
    if(!fptr) return;
    SDL_WriteIO(fptr, data, size);
    SDL_CloseIO(fptr);
}

void file_delete(file_t* file){
    free(file->data);
}

const char* path_get_ext(const char* path){
    const char* dot = strrchr(path, '.');
    if(!dot) return "";
    return dot + 1;
}

void path_set_ext(const char* i_path, char* o_path, const char* ext){
    const char* dot = strrchr(i_path, '.');
    if(!dot) return;
    sprintf(o_path, "%.*s.%s", (int)(dot - i_path), i_path, ext);
}