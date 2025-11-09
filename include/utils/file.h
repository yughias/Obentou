#ifndef __FILE_H__
#define __FILE_H__

#include "types.h"

typedef struct file_t {
    char path[FILENAME_MAX];
    u8* data;
    size_t size;
} file_t;

bool file_load(file_t* file, const char* filename, bool show_msg);
void file_delete(file_t* file);
void file_save(const char* filename, u8* data, size_t size);
void file_append(const char* filename, u8* data, size_t size);
const char* path_get_ext(const char* path);
void path_set_ext(const char* i_path, char* o_path, const char* ext);

#endif