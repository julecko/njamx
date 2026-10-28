#ifndef PROCESS_JAMMER_H
#define PROCESS_JAMMER_H

#include <stdatomic.h>

#include "./network/network.h"
#include "network/socket.h"

typedef struct jammer_args {
    network_t *network;
    const char *interface;
    atomic_bool *stop_flag;
    network_socket_t network_socket;
} jammer_args_t;

void *jammer_process(void *arg);

#endif
