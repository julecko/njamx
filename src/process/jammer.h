#ifndef PROCESS_JAMMER_H
#define PROCESS_JAMMER_H

#include "./network/network.h"
#include "network/socket.h"
#include "platform/atomic_compat.h"
#include "process/runtime_options.h"

typedef struct jammer_args {
    network_t *network;
    atomic_bool *stop_flag;
    network_socket_t network_socket;
    // use_real_mac/program_mac here are live-togglable via the CLI's
    // "randommac" command, re-read every jam cycle (see jammer.c).
    runtime_options_t *opts;
} jammer_args_t;

void *jammer_process(void *arg);

#endif
