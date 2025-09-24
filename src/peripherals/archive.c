#include "peripherals/archive.h"
#include "peripherals/vec.h"

#include <stdlib.h>
#include <string.h>

#include "zip.h"

#if defined(_WIN32) || defined(_WIN64)
    #define strcasecmp _stricmp
#endif

DEFINE_VEC(files, file_t)

typedef struct archive {
    path_t path;
    files_t files;
} archive_t;

static void archive_load_zip(archive_t* archive, const char* filename) {
    struct zip_t *zip = zip_open(filename, 0, 'r');
    int n = zip_entries_total(zip);
    for (int i = 0; i < n; ++i) {
        zip_entry_openbyindex(zip, i);
        const char *name = zip_entry_name(zip);
        int isdir = zip_entry_isdir(zip);
        if (!isdir) {
            files_push_empty(&archive->files);
            file_t* file = &archive->files.data[archive->files.size - 1];
            strcpy(file->path, name);
            zip_entry_read(zip, (void**)(&file->data), &file->size);
        }
        zip_entry_close(zip);
    }
zip_close(zip);
}

archive_t* archive_load(const char* filename) {
    archive_t* archive = malloc(sizeof(archive_t));
    files_init(&archive->files);
    strcpy(archive->path, filename);
    const char* ext = path_get_ext(filename);
    if(!strcmp(ext, "zip")){
        archive_load_zip(archive, filename);
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