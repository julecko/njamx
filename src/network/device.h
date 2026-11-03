#ifndef NETWORK_DEVICE_H
#define NETWORK_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum device_state {
    DEVICE_STATE_DEAD,
    DEVICE_STATE_ADDED,
    DEVICE_STATE_ALIVE,
    DEVICE_STATE_DISCONNECTING,
} device_state_t;

typedef enum device_role {
    DEVICE_ROLE_CLIENT,
    DEVICE_ROLE_ROUTER,
} device_role_t;

typedef enum device_flags {
    DEVICE_FLAG_NONE = 0,
    // This device is us (network->my_ip). Highlighted in the CLI, and
    // excluded from "all"/"armagedon" unless self-jamming is enabled.
    DEVICE_FLAG_MY_DEVICE = 1 << 0,
} device_flags_t;

typedef struct device {
    uint32_t ip;
    uint8_t mac[6];
    device_state_t state;
    device_role_t role;
    device_flags_t flags;
    bool jamming;
} device_t;

/**
 * @brief Make a new device in the dead state with the client role.
 *
 * @param ip  IPv4 address of the device, in host byte order.
 * @return The initialized device, returned by value.
 */
device_t network_device_create(uint32_t ip);

#ifdef DEBUG_MODE
/**
 * @brief Print device (available only in debug)
 *
 * @param device Pointer to device struct to be printed
 * @return Nothing
 */
void network_device_print(device_t *device);
#endif

#endif
