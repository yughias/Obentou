#include <stdlib.h>

#include "utils/rewind.h"

#include "utils/vec.h"
#include "types.h"

DEFINE_VEC(byte_vec, u8)

#define MAX_SIZE (60*60)

static int rewind_ring_size = 0;
static int rewind_idx = 0;
static byte_vec_t rewind_ring[MAX_SIZE];

static byte_vec_t xor_diff(const byte_vec_t* cur, const byte_vec_t* prev) {
    byte_vec_t out;
    out.size = cur->size;
    out.allocated = cur->size;
    out.data = malloc(out.allocated);

    for (size_t i = 0; i < cur->size; i++)
        out.data[i] = cur->data[i] ^ prev->data[i];

    return out;
}

static byte_vec_t rle0_encode(const byte_vec_t* in) {
    byte_vec_t out;
    out.size = 0;
    out.allocated = in->size * 3;
    out.data = malloc(out.allocated);

    size_t i = 0;
    while (i < in->size) {
        if (in->data[i] == 0x00) {
            size_t run = 1;
            while (i + run < in->size &&
                   in->data[i + run] == 0x00 &&
                   run < 0xFFFF)
                run++;

            out.data[out.size++] = 0x00;
            out.data[out.size++] = (uint8_t)(run & 0xFF);
            out.data[out.size++] = (uint8_t)((run >> 8) & 0xFF);

            i += run;
        } else {
            out.data[out.size++] = in->data[i++];
        }
    }

    byte_vec_shrink(&out);
    return out;
}

static byte_vec_t rle0_decode(const byte_vec_t* in, size_t expected_out_size) {
    byte_vec_t out;
    out.size = 0;
    out.allocated = expected_out_size;
    out.data = malloc(out.allocated);

    size_t i = 0;
    while (i < in->size) {
        if (in->data[i] == 0x00) {
            if (i + 2 >= in->size){
                printf("RLE0 corrupted\n");
                break;
            }

            uint16_t run = in->data[i+1] | (in->data[i+2] << 8);
            i += 3;

            memset(out.data + out.size, 0x00, run);
            out.size += run;
        } else {
            out.data[out.size++] = in->data[i++];
        }
    }

    return out;
}

static byte_vec_t savestate_step_back(const byte_vec_t* current, const byte_vec_t* diff_rle) {
    byte_vec_t decoded = rle0_decode(diff_rle, current->size);
    byte_vec_t prev = xor_diff(current, &decoded);
    byte_vec_free(&decoded);
    return prev;
}

void rewind_init(){
    rewind_clear();
    rewind_ring_size = 0;
    rewind_idx = 0;
}

void rewind_push(byte_vec_t* state){
    if(rewind_ring_size == MAX_SIZE){
        byte_vec_free(&rewind_ring[rewind_idx]);
    } else {
        rewind_ring_size++;
    }

    rewind_ring[rewind_idx] = *state;

    rewind_idx = (rewind_idx + 1) % MAX_SIZE;
}

byte_vec_t* rewind_pop(){
    rewind_idx = (rewind_idx - 1 + MAX_SIZE) % MAX_SIZE;
    rewind_ring_size--;
    return &rewind_ring[rewind_idx];
}

byte_vec_t* rewind_peek(){
    int idx = (rewind_idx - 1 + MAX_SIZE) % MAX_SIZE;
    return &rewind_ring[idx];
}

void rewind_clear(){
    for(int i = 0; i < rewind_ring_size; i++){
        int idx = (rewind_idx - 1 - i + MAX_SIZE) % MAX_SIZE;
        byte_vec_free(&rewind_ring[idx]);
    }

    memset(rewind_ring, 0, sizeof(rewind_ring));

    rewind_ring_size = 0;
    rewind_idx = 0;
}

void rewind_add_state(byte_vec_t* state){
    if(rewind_ring_size){
        byte_vec_t* last = rewind_peek();

        byte_vec_t xored = xor_diff(state, last);
        byte_vec_t rle   = rle0_encode(&xored);

        byte_vec_free(last);
        *last = rle;

        byte_vec_free(&xored);
    }

    rewind_push(state);
}

byte_vec_t* rewind_recover_state(){
    if(rewind_ring_size < 2)
        return NULL;
    
    byte_vec_t* cur = rewind_pop();
    byte_vec_t* diff = rewind_peek();

    byte_vec_t prev = savestate_step_back(cur, diff);

    byte_vec_free(diff);
    byte_vec_free(cur);

    *diff = prev;

    return diff;
}

#include "math.h"

void rewind_print_info(){
    int sum = 0;
    for(int i = 0; i < MAX_SIZE; i++)
        sum += rewind_ring[i].allocated;

    printf("rewind size: %f Mib\n", sum / pow(2, 20));
}