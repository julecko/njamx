#ifndef NETWORK_NETWORK_H
#define NETWORK_NETWORK_H

#include <stddef.h>
#include <stdint.h>

#include "network/device.h"
#include "platform/thread_compat.h"

typedef struct network {
    uint32_t my_ip;
    uint32_t mask;
    device_t *devices;
    size_t length;

    pthread_mutex_t lock;
} network_t;

/**
 * @brief Create network with all devices (caller must use network_delete)
 *
 * @param ip_with_mask Ip with mask
 * @return Network struct with all devices set to dead and client
 */
network_t network_create(const char *ip_with_mask);

void network_delete(network_t *network);

/**
 * @brief Set the MAC address on the device flagged DEVICE_FLAG_MY_DEVICE
 *        (a no-op if, somehow, none is flagged).
 *
 * @param network Network to update.
 * @param mac     MAC currently in use on the interface (the forged one
 *                if --change-mac is active, the real one otherwise).
 */
void network_set_my_mac(network_t *network, const uint8_t mac[6]);

static inline uint32_t network_get_network_ip(network_t *network) {
    return network->my_ip & network->mask;
}

static inline uint32_t network_get_broadcast_ip(network_t *network) {
    return network->my_ip | ~network->mask;
}
#endif
