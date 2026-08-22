#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "types.h"

#include "utils/serializer.h"

typedef struct gba_t gba_t;

typedef void (*gba_event_t)(gba_t* gba, u32 arg);
typedef struct gba_scheduler_t gba_scheduler_t;
typedef gba_scheduler_t* gba_scheduler_ptr_t;

typedef struct byte_vec_t byte_vec_t;
void serialize_gba_scheduler_ptr_t(gba_scheduler_ptr_t*, byte_vec_t*);
u8* deserialize_gba_scheduler_ptr_t(gba_scheduler_ptr_t*, u8*, u8*);
void serialize_gba_event_t(gba_event_t*, byte_vec_t*);
u8* deserialize_gba_event_t(gba_event_t*, u8*, u8*);

#define SCHEDULER_STRUCT(X) \
    X(bool, used, 1, 0) \
    X(u64, remaining, 1, 0) \
    X(u32, arg, 1, 0) \
    X(gba_event_t, event, 1, 1) \
    X(gba_scheduler_ptr_t, next, 1, 1)

DECLARE_SERIALIZABLE_STRUCT(gba_scheduler, SCHEDULER_STRUCT)

gba_scheduler_t* gba_scheduler_occupy_block(gba_scheduler_t* scheduler_pool, size_t size);
void gba_scheduler_add_event(gba_scheduler_t** list, gba_scheduler_t* event);
void gba_scheduler_remove_event(gba_scheduler_t** list, gba_scheduler_t* event);
void gba_scheduler_step(gba_t* gba, gba_scheduler_t** list, u32 cycles_step);

void gba_scheduler_remove_if_present(gba_scheduler_t** list, gba_scheduler_t** event);
gba_scheduler_t* gba_scheduler_create_add_event_0_args(gba_scheduler_t** list, gba_scheduler_t* pool, size_t pool_size, gba_event_t event, u64 remaining);
gba_scheduler_t* gba_scheduler_create_add_event_1_args(gba_scheduler_t** list, gba_scheduler_t* pool, size_t pool_size, gba_event_t event, u32 arg, u64 remaining);

#endif