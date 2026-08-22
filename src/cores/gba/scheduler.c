#include "cores/gba/scheduler.h"
#include "cores/gba/gba.h"
#include "cores/gba/timer.h"

#include "core.h"

#include <stdio.h>

gba_scheduler_t* gba_scheduler_occupy_block(gba_scheduler_t* scheduler_pool, size_t size){
    for(int i = 0; i < size; i++){
        if(!scheduler_pool[i].used){
            scheduler_pool[i].used = true;
            scheduler_pool[i].next = NULL;
            return &scheduler_pool[i];
        }
    }

    printf("CANNOT FIND FREE SCHEDULER!\n");
}


void gba_scheduler_add_event(gba_scheduler_t** list, gba_scheduler_t* event){
    if(*list == NULL){
        *list = event;
        return;
    }

    if(event->remaining < (*list)->remaining){
        (*list)->remaining -= event->remaining;
        event->next = *list;
        (*list) = event;
        return;
    }
    
    event->remaining -= (*list)->remaining;

    gba_scheduler_t* p = (*list)->next;
    gba_scheduler_t* q = *list;
    while(p && event->remaining > p->remaining){
        event->remaining -= p->remaining;
        q = p;
        p = p->next;
    }

    q->next = event;
    event->next = p;
    if(p)
        p->remaining -= event->remaining;
}

void gba_scheduler_remove_event(gba_scheduler_t** list, gba_scheduler_t* event){
    event->used = false;

    if(event->next)
        event->next->remaining += event->remaining;

    if((*list) == event){
        *list = (*list)->next;
        return;
    }

    gba_scheduler_t* p = *list;
    while(p && p->next != event)
        p = p->next;

    p->next = event->next;
}

void gba_scheduler_step(gba_t* gba, gba_scheduler_t** scheduler, u32 cycles_step){
    if(*scheduler == NULL)
        printf("SCHEDULER HEAD CANNOT BE EMPTY!\n");
    
    gba->clock_before_scheduling = gba->cpu.cycles;

    gba_scheduler_t* closest_event = (*scheduler);
    while(cycles_step || !closest_event->remaining){
        u32 to_subtract = cycles_step < closest_event->remaining ? cycles_step : closest_event->remaining;
        closest_event->remaining -= to_subtract;
        cycles_step -= to_subtract;
        if(!closest_event->remaining){
            closest_event->used = false;
            *scheduler = closest_event->next;
            (closest_event->event)(gba, closest_event->arg);
        }
        closest_event = (*scheduler);
    }
}

void gba_scheduler_remove_if_present(gba_scheduler_t** list, gba_scheduler_t** event){
    if(*event){
        gba_scheduler_remove_event(list, *event);
        *event = NULL;
    }
}


gba_scheduler_t* gba_scheduler_create_add_event_0_args(gba_scheduler_t** list, gba_scheduler_t* pool, size_t pool_size, gba_event_t event, u64 remaining){
    gba_scheduler_t* block = gba_scheduler_occupy_block(pool, pool_size);
    block->event = event;
    block->remaining = remaining;
    gba_scheduler_add_event(list, block);
    return block;
}


gba_scheduler_t* gba_scheduler_create_add_event_1_args(gba_scheduler_t** list, gba_scheduler_t* pool, size_t pool_size, gba_event_t event, u32 arg, u64 remaining){
    gba_scheduler_t* block = gba_scheduler_create_add_event_0_args(list, pool, pool_size, event, remaining);
    block->arg = arg;
    return block;
}

void serialize_gba_scheduler_ptr_t(gba_scheduler_ptr_t* ptr, byte_vec_t* vec){
    gba_t* gba = core_get_emu_ptr();
    gba_scheduler_ptr_t sched_ptr = *ptr;
    uptr idx = sched_ptr ? (uptr)sched_ptr - (uptr)gba->scheduler_pool : -1;
    byte_vec_push_array(vec, (u8*)&idx, sizeof(uptr));
}

u8* deserialize_gba_scheduler_ptr_t(gba_scheduler_ptr_t* ptr, u8* data, u8* end) {
    gba_t* gba = core_get_emu_ptr();
    uptr idx;
    
    if (data + sizeof(uptr) > end) return NULL;
    memcpy(&idx, data, sizeof(uptr)); data += sizeof(uptr);

    *ptr = (gba_scheduler_ptr_t)(idx == -1 ? (uptr)NULL : (uptr)gba->scheduler_pool + idx);
    return data;
}

static const gba_event_t event_table[] = {
    gba_event_length_expired,
    gba_event_push_sample,
    gba_ppu_event_start_hblank,
    gba_ppu_event_start_scanline,
    gba_timer_event_overflow,
    gba_event_update_envelope,
    gba_event_update_lfsr,
    gba_event_update_sweep,
    gba_event_update_tone,
    gba_event_update_wave
};

void serialize_gba_event_t(gba_event_t* ptr, byte_vec_t* vec){
    gba_event_t func = *ptr;
    uptr idx = -1;

    for (int i = 0; i < sizeof(event_table) / sizeof(uptr); i++) {
        if(func == event_table[i]){
            idx = i;
            break;
        }
    }

    byte_vec_push_array(vec, (u8*)&idx, sizeof(uptr));
}

u8* deserialize_gba_event_t(gba_event_t* ptr, u8* data, u8* end) {
    uptr idx;
    
    if (data + sizeof(uptr) > end) return NULL;
    memcpy(&idx, data, sizeof(uptr)); data += sizeof(uptr);

    for (int i = 0; i < sizeof(event_table) / sizeof(uptr); i++) {
        if(idx == i){
            *ptr = event_table[i];
            break;
        }
    }

    return data;
}