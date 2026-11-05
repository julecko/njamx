#ifndef PROCESS_JAMMER_H
#define PROCESS_JAMMER_H

#include <stdatomic.h>
#include <stdint.h>

#include "./network/network.h"
#include "network/socket.h"

typedef struct jammer_args {
    network_t *network;
    const char *interface;
    atomic_bool *stop_flag;
    network_socket_t network_socket;
    // If non-NULL, forged ARP replies use this MAC (6 bytes) as the
    // sender address instead of a freshly generated random one. Must
    // stay valid for the jammer thread's whole lifetime.
    const uint8_t *sender_mac_override;
} jammer_args_t;

void *jammer_process(void *arg);

#endif
