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

u8 netplay_input_delay;

#define NETPLAY_RING_SIZE 256

static uint8_t local_input_ring[NETPLAY_RING_SIZE][32]; 
static uint8_t remote_input_ring[NETPLAY_RING_SIZE][32];

static u64 netplay_current_frame;
static u64 remote_frames_received;

static bool send_bytes(const u8* buffer, int n_bytes) {
    return NET_WriteToStreamSocket(netplay_socket, buffer, n_bytes);
}

static bool recv_bytes(u8* buffer, int n_bytes) {
    int bytes_read = 0;
    while (bytes_read < n_bytes) {
        NET_WaitUntilInputAvailable((void**)&netplay_socket, 1, -1);
        int res = NET_ReadFromStreamSocket(netplay_socket, buffer + bytes_read, n_bytes - bytes_read);
        if (res < 0)
            return false;
        bytes_read += res;
    }

    return true;
}

static void netplay_start_host(core_ctx_t* ctx) {
    NET_Server* server_socket = NET_CreateServer(NULL, netplay_port);
    while (NET_AcceptClient(server_socket, &netplay_socket) && !netplay_socket) {
        SDL_Delay(100);
        printf("Waiting for client...\n");
    }

    NET_DestroyServer(server_socket);

    printf("sending state...\n");
    byte_vec_t state = ctx->core->savestate(ctx);
    printf("size: %llu\n", state.size);
    send_bytes((u8*)&state.size, sizeof(u32));
    send_bytes(state.data, state.size);
    byte_vec_free(&state);

    netplay_actual_mode = NETPLAY_HOST;
}

static void netplay_start_client(core_ctx_t* ctx) {
    NET_Address* net_address = NET_ResolveHostname(netplay_host_ip);
    NET_WaitUntilResolved(net_address, -1);
    netplay_socket = NET_CreateClient(net_address, netplay_port);
    NET_WaitUntilConnected(netplay_socket, -1);
    NET_UnrefAddress(net_address);

    printf("receiving state...\n");
    byte_vec_t state;
    byte_vec_init(&state);
    recv_bytes((u8*)&state.size, sizeof(u32));
    printf("size: %llu\n", state.size);
    state.allocated = state.size;
    state.data = malloc(state.allocated);
    recv_bytes(state.data, state.size);
    ctx->core->loadstate(ctx, &state);
    byte_vec_free(&state);

    netplay_actual_mode = NETPLAY_CLIENT;
}

void netplay_init() {
    NET_Init();
}

void netplay_quit() {
    netplay_close_session();
    NET_Quit();
}

void netplay_start_session(core_ctx_t* ctx) {
    netplay_close_session();
    netplay_current_frame = 0;
    remote_frames_received = 0;
    memset(local_input_ring, 0, sizeof(local_input_ring));
    memset(remote_input_ring, 0, sizeof(remote_input_ring));
    switch (netplay_wanted_mode) {
        case NETPLAY_NONE:
        netplay_actual_mode = NETPLAY_NONE;
        break;

        case NETPLAY_HOST:
        netplay_start_host(ctx);
        break;

        case NETPLAY_CLIENT:
        netplay_start_client(ctx);
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
    int n_bytes = (n_inputs + 7) / 8;
    uint8_t buffer[n_bytes];
    memset(buffer, 0, n_bytes);

    int local_port = netplay_actual_mode == NETPLAY_HOST ? 0 : 1;

    for (int i = 0; i < n_inputs; i++) {
        if (controls_pressed(core->control_begin + i, local_port)) {
            buffer[i / 8] |= (1 << (i % 8));
        }
    }

    uint32_t target_frame = netplay_current_frame + netplay_input_delay;
    memcpy(local_input_ring[target_frame % NETPLAY_RING_SIZE], buffer, n_bytes);

    NET_WriteToStreamSocket(netplay_socket, buffer, n_bytes);
}

static bool drain_bytes(int n_bytes) {
    while (true) {
        int wait_res = NET_WaitUntilInputAvailable((void**)&netplay_socket, 1, 0); 
        if (wait_res < 0) return false;
        if (wait_res == 0) break;

        uint8_t buffer[n_bytes];
        if(!recv_bytes(buffer, n_bytes))
            return false;

        uint32_t opponent_target = remote_frames_received + netplay_input_delay;
        memcpy(remote_input_ring[opponent_target % NETPLAY_RING_SIZE], buffer, n_bytes);
        remote_frames_received++;
    }

    return true;
}

void netplay_recv_inputs(const core_t* core) {
    int n_inputs = core->control_end - core->control_begin + 1;
    int n_bytes = (n_inputs + 7) / 8;

    int local_port = netplay_actual_mode == NETPLAY_HOST ? 0 : 1;
    int remote_port = netplay_actual_mode == NETPLAY_HOST ? 1 : 0;

    if(!drain_bytes(n_bytes))
        return; /// disconnected

    int needed_remote_frames = (int)netplay_current_frame - netplay_input_delay + 1;

    while ((int)remote_frames_received < needed_remote_frames) {
        int wait_res = NET_WaitUntilInputAvailable((void**)&netplay_socket, 1, -1);
        if (wait_res < 0) return; // Handle disconnect

        uint8_t buffer[n_bytes];
        if(!recv_bytes(buffer, n_bytes))
            return; // disconnected

        uint32_t opponent_target = remote_frames_received + netplay_input_delay;
        memcpy(remote_input_ring[opponent_target % NETPLAY_RING_SIZE], buffer, n_bytes);
        remote_frames_received++;
    }

    uint8_t* current_local = local_input_ring[netplay_current_frame % NETPLAY_RING_SIZE];
    uint8_t* current_remote = remote_input_ring[netplay_current_frame % NETPLAY_RING_SIZE];

    for (int i = 0; i < n_inputs; i++) {
        bool l_input = (current_local[i / 8] >> (i % 8)) & 1;
        controls_override(core->control_begin + i, l_input, local_port);

        bool r_input = (current_remote[i / 8] >> (i % 8)) & 1;
        controls_override(core->control_begin + i, r_input, remote_port);
    }

    netplay_current_frame++;
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
