#ifndef __REWIND_H__
#define __REWIND_H__

typedef struct byte_vec_t byte_vec_t;

void rewind_init();
void rewind_clear();
void rewind_add_state(byte_vec_t* state);
void rewind_print_info();
byte_vec_t* rewind_recover_state();


#endif
