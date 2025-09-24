#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include "peripherals/file.h"

typedef struct archive archive_t;

archive_t* archive_load(const char* filename);
void archive_free(archive_t* archive);
file_t* archive_get_file_by_ext(const archive_t* archive, const char* ext);
const char* archive_get_path(const archive_t* archive);

#endif