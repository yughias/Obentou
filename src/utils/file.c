#include "utils/file.h"

#include "SDL3/SDL.h"

bool file_load(file_t* file, const char* filename){
    memset(file, 0, sizeof(file_t));
    strcpy(file->path, filename);
    file->data = SDL_LoadFile(filename, &file->size);
    return file->data;
}

void file_save(const char* filename, u8* data, size_t size){
    SDL_SaveFile(filename, data, size);
}

void file_append(const char* filename, u8* data, size_t size){
    FILE* fptr = fopen(filename, "ab+");
    if(!fptr) return;
    fwrite(data, 1, size, fptr);
    fclose(fptr);
}

void file_delete(file_t* file){
    SDL_free(file->data);
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