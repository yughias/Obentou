#ifndef __FILE_H__
#define __FILE_H__

#include "types.h"

typedef struct file_t {
    path_t path;
    u8* data;
    size_t size;
} file_t;

void file_load(file_t* file, const char* filename);
void file_delete(file_t* file);
void file_save(file_t* file);
const char* path_get_ext(const char* path);

#endif