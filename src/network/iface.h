#ifndef NETWORK_IFACE_H
#define NETWORK_IFACE_H

#include <stdint.h>

/**
 * @brief Look up the hardware (MAC) address of a network interface.
 *
 * @param ifname Interface name (e.g. "wlan0").
 * @param mac    Out: 6-byte MAC address, filled in on success.
 * @return 0 on success, -1 on error.
 */
int iface_get_mac(const char *ifname, uint8_t mac[6]);

#endif
