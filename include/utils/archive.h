#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include "utils/file.h"

typedef struct archive_t archive_t;

archive_t* archive_load(const char* filename);
void archive_free(archive_t* archive);
file_t* archive_get_file_by_ext(const archive_t* archive, const char* ext);
/* Search archive for a file whose basename (filename without directory) matches
 * name; comparison is case-insensitive so "Pacman.6e" matches "pacman.6e". */
file_t* archive_get_file_by_name(const archive_t* archive, const char* name);
const char* archive_get_path(const archive_t* archive);

#endif