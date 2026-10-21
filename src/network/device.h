#ifndef NETWORK_DEVICE_H
#define NETWORK_DEVICE_H

#include <stdint.h>

typedef enum device_state {
    DEVICE_STATE_DEAD,
    DEVICE_STATE_ADDED,
    DEVICE_STATE_ALIVE,
} device_state_t;

typedef enum device_role {
    DEVICE_ROLE_CLIENT,
    DEVICE_ROLE_ROUTER,
} device_role_t;

typedef struct device {
    uint32_t ip;
    device_state_t state;
    device_role_t role;
} device_t;


/**
 * @brief Make a new device in the dead state with the client role.
 *
 * @param ip  IPv4 address of the device, in host byte order.
 * @return The initialized device, returned by value.
 */
device_t device_create(uint32_t ip);

/**
 * @brief Print device (available only in debug)
 *
 * @param device Pointer to device struct to be printed
 * @return Nothing
 */
void device_print(device_t *device);

#endif
