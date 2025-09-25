#ifndef __FILE_H__
#define __FILE_H__

#include "types.h"

typedef struct file_t {
    char path[FILENAME_MAX];
    u8* data;
    size_t size;
} file_t;

bool file_load(file_t* file, const char* filename);
void file_delete(file_t* file);
void file_save(file_t* file);
const char* path_get_ext(const char* path);

#endif