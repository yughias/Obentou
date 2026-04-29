#ifndef __NETPLAY_H__
#define __NETPLAY_H__

#include "types.h"

typedef enum NETPLAY_MODE { NETPLAY_NONE, NETPLAY_HOST, NETPLAY_CLIENT, NETPLAY_MODE_COUNT } NETPLAY_MODE;

typedef struct core_t core_t;

extern u16 netplay_port;
extern char netplay_host_ip[32];
extern NETPLAY_MODE netplay_wanted_mode;
extern NETPLAY_MODE netplay_actual_mode;

void netplay_init();
void netplay_quit();

void netplay_start_session();
void netplay_close_session();

bool netplay_is_connected();

void netplay_send_inputs(const core_t* core);
void netplay_recv_inputs(const core_t* core);

#endif