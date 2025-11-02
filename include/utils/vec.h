#ifndef __VEC_H__
#define __VEC_H__

#include <stdlib.h>
#include <string.h>

#define DEFINE_VEC(name, type) \
typedef struct name##_t { \
    type* data; \
    size_t size; \
    size_t allocated; \
} name##_t; \
\
static inline void name##_init(name##_t* vec) { \
    vec->data = NULL; \
    vec->size = 0; \
    vec->allocated = 0; \
} \
\
static inline void name##_free(name##_t* vec) { \
    free(vec->data); \
    name##_init(vec); \
} \
\
static inline void name##_ensure_capacity(name##_t* vec, size_t needed) { \
    if (needed > vec->allocated) { \
        size_t new_alloc = vec->allocated ? vec->allocated : 1; \
        while (new_alloc < needed) new_alloc *= 2; \
        type* new_data = realloc(vec->data, sizeof(type) * new_alloc); \
        if (!new_data) abort(); /* or handle error */ \
        vec->data = new_data; \
        vec->allocated = new_alloc; \
    } \
} \
\
static inline void name##_push(name##_t* vec, type value) { \
    name##_ensure_capacity(vec, vec->size + 1); \
    vec->data[vec->size++] = value; \
} \
\
static inline void name##_push_empty(name##_t* vec) { \
    name##_ensure_capacity(vec, vec->size + 1); \
    vec->size++; \
} \
\
static inline void name##_push_array(name##_t* vec, const type* values, size_t count) { \
    if (count == 0) return; \
    size_t old_size = vec->size; \
    name##_ensure_capacity(vec, vec->size + count); \
    memcpy(vec->data + old_size, values, sizeof(type) * count); \
    vec->size += count; \
} \
\
static inline type name##_pop(name##_t* vec) { \
    return vec->data[--vec->size]; \
} \
\
static inline void name##_shrink(name##_t* vec) { \
    vec->allocated = vec->size; \
    vec->data = realloc(vec->data, sizeof(type) * vec->allocated); \
}

#endif
