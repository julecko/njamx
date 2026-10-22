#ifndef NETWORK_NETWORK_H
#define NETWORK_NETWORK_H

#include <stddef.h>
#include <stdint.h>

#include "network/device.h"

typedef struct network {
    uint32_t my_ip;
    uint32_t mask;
    device_t *devices;
    size_t length;
} network_t;

/**
 * @brief Create network with all devices (caller must use network_delete)
 *
 * @param ip_with_mask Ip with mask
 * @return Network struct with all devices set to dead and client
 */
network_t network_create(const char *ip_with_mask);

void network_delete(network_t *network);
inline uint32_t network_get_network_ip(network_t *network);
inline uint32_t network_get_broadcast_ip(network_t *network);
#endif
