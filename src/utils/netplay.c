#include "utils/netplay.h"
#include "utils/controls.h"

#include "core.h"

#ifndef __EMSCRIPTEN__

#include <SDL3_net/SDL_net.h>

u16 netplay_port = 4567;
char netplay_host_ip[32] = "127.0.0.1";
NETPLAY_MODE netplay_wanted_mode = NETPLAY_NONE;
NETPLAY_MODE netplay_actual_mode = NETPLAY_NONE;

NET_StreamSocket* netplay_socket = NULL;

static void netplay_start_host() {
    NET_Server* server_socket = NET_CreateServer(NULL, netplay_port);
    while (NET_AcceptClient(server_socket, &netplay_socket) && !netplay_socket) {
        SDL_Delay(100);
        printf("Waiting for client...\n");
    }

    NET_DestroyServer(server_socket);

    netplay_actual_mode = NETPLAY_HOST;
}

static void netplay_start_client() {
    NET_Address* net_address = NET_ResolveHostname(netplay_host_ip);
    NET_WaitUntilResolved(net_address, -1);
    netplay_socket = NET_CreateClient(net_address, netplay_port);
    NET_WaitUntilConnected(netplay_socket, -1);

    NET_UnrefAddress(net_address);
    netplay_actual_mode = NETPLAY_CLIENT;
}

void netplay_init() {
    NET_Init();
}

void netplay_quit() {
    netplay_close_session();
    NET_Quit();
}

void netplay_start_session() {
    netplay_close_session();
    switch (netplay_wanted_mode) {
        case NETPLAY_NONE:
        netplay_actual_mode = NETPLAY_NONE;
        break;

        case NETPLAY_HOST:
        netplay_start_host();
        break;

        case NETPLAY_CLIENT:
        netplay_start_client();
        break;

        default:
        netplay_actual_mode = NETPLAY_NONE;
        break;
    }
}

void netplay_close_session() {
    if (netplay_socket)
        NET_DestroyStreamSocket(netplay_socket);
    netplay_socket = NULL;
    netplay_actual_mode = NETPLAY_NONE;
}

bool netplay_is_connected() {
    return netplay_actual_mode != NETPLAY_NONE;
}

void netplay_send_inputs(const core_t* core) {
    int n_inputs = core->control_end - core->control_begin + 1;
    bool inputs[n_inputs];
    int port = netplay_actual_mode == NETPLAY_HOST ? 0 : 1;

    for(int i = 0; i < n_inputs; i++) {
        inputs[i] = controls_pressed(core->control_begin + i, port);
    }
    
    for(int i = 0; i < n_inputs; i++) {
        NET_WriteToStreamSocket(netplay_socket, &inputs[i], sizeof(bool));
    }
}

void netplay_recv_inputs(const core_t* core) {
    int port = netplay_actual_mode == NETPLAY_HOST ? 1 : 0;

    for(int i = 0; i < core->control_end - core->control_begin + 1; i++) {
        bool input;
        NET_WaitUntilInputAvailable((void**)&netplay_socket, 1, -1);
        NET_ReadFromStreamSocket(netplay_socket, &input, sizeof(bool));
        controls_override(core->control_begin + i, input, port);
    }
}

#else

u16 netplay_port;
char netplay_host_ip[32];
NETPLAY_MODE netplay_wanted_mode;
NETPLAY_MODE netplay_actual_mode;

void netplay_init(){}
void netplay_quit(){}
void netplay_start_session(){}
void netplay_close_session(){}
bool netplay_is_connected(){ return false; }
void netplay_send_inputs(const core_t* core){}
void netplay_recv_inputs(const core_t* core){}

#endif