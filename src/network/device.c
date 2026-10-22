#include "network/device.h"

#include <stdint.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

device_t device_create(uint32_t ip) {
    return (device_t){.ip = ip, .role = DEVICE_ROLE_CLIENT, .state = DEVICE_STATE_DEAD};
};

#ifdef DEBUG_MODE
static const char *device_state_to_str(device_state_t state) {
    switch (state) {
        case DEVICE_STATE_DEAD:
            return "dead";
        case DEVICE_STATE_ADDED:
            return "added";
        case DEVICE_STATE_ALIVE:
            return "alive";
    }
    return "unknown";
}

static const char *device_role_to_str(device_role_t role) {
    switch (role) {
        case DEVICE_ROLE_CLIENT:
            return "client";
        case DEVICE_ROLE_ROUTER:
            return "router";
    }
    return "unknown";
}

void device_print(device_t *device) {
    struct in_addr addr = {.s_addr = htonl(device->ip)};
    char ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &addr, ip, sizeof(ip));

    printf("device ip: %s, state: %s, role: %s\n", ip, device_state_to_str(device->state),
           device_role_to_str(device->role));
}
#endif
