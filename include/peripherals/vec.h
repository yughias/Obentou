#ifndef __VEC_H__
#define __VEC_H__

#define REALLOC_VEC(type) \
while (vec->size > vec->allocated) \
{ \
    vec->allocated = vec->allocated ? vec->allocated * 2 : 1; \
    vec->data = realloc(vec->data, sizeof(type) * vec->allocated); \
}

#define DEFINE_VEC(name, type) \
typedef struct name##_t \
{ \
    type* data; \
    size_t size; \
    size_t allocated; \
} name##_t; \
\
static inline void name##_init(name##_t* vec) \
{ \
    vec->data = NULL; \
    vec->size = 0; \
    vec->allocated = 0; \
} \
\
static inline void name##_free(name##_t* vec) \
{ \
    free(vec->data); \
    name##_init(vec); \
} \
\
static inline void name##_push(name##_t* vec, type value) \
{ \
    vec->size++; \
    REALLOC_VEC(type) \
    vec->data[vec->size - 1] = value; \
} \
\
static inline void name##_push_empty(name##_t* vec) \
{ \
    vec->size++; \
    REALLOC_VEC(type) \
} \
\
static inline void name##_push_array(name##_t* vec, type* values, size_t count) \
{ \
    size_t old_size = vec->size; \
    vec->size += count; \
    REALLOC_VEC(type) \
    memcpy(vec->data + old_size, values, sizeof(type) * count); \
} \
\
static inline type name##_pop(name##_t* vec) \
{ \
    return vec->data[--vec->size]; \
} \
static inline void name##_shrink(name##_t* vec) \
{ \
    vec->allocated = vec->size; \
    vec->data = realloc(vec->data, sizeof(type) * vec->allocated); \
}

#endif