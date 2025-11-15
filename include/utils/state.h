#ifndef __STATE_H__
#define __STATE_H__

#include <stdbool.h>

typedef struct core_ctx_t core_ctx_t;
bool state_get_autosave();
void state_set_autosave(bool autosave);
void state_switch_autosave();
int state_get_active_slot();
void state_set_active_slot(int* slot);
void state_save_slot(core_ctx_t* ctx);
void state_load_slot(core_ctx_t* ctx);
void state_load_autosave(core_ctx_t* ctx);
void state_save_autosave(core_ctx_t* ctx);

#endif
