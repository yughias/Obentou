#include "peripherals/archive.h"
#include "peripherals/vec.h"

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
    #define strcasecmp _stricmp
#endif

DEFINE_VEC(files, file_t)

typedef struct archive {
    path_t path;
    files_t files;
} archive_t;

archive_t* archive_load(const char* filename) {
    archive_t* archive = malloc(sizeof(archive_t));
    files_init(&archive->files);
    strcpy(archive->path, filename);
    const char* ext = path_get_ext(filename);
    if(!strcmp(ext, "zip")){
        printf("zip not supported!\n");
        exit(EXIT_FAILURE);
    } else {
        files_push_empty(&archive->files);
        file_t* file = &archive->files.data[0];
        strcpy(file->path, filename);
        file_load(file, filename);
    }
    return archive;
}

void archive_free(archive_t* archive) {
    files_free(&archive->files);
    free(archive);
}

file_t* archive_get_file_by_ext(const archive_t* archive, const char* ext) {
    if(!archive)
        return NULL;
    for(int i = 0; i < archive->files.size; i++){
        file_t* file = &archive->files.data[i];
        if(!strcasecmp(path_get_ext(file->path), ext)){
            return file;
        }
    }
    return NULL;
}

const char* archive_get_path(const archive_t* archive) {
    return archive->path;
}