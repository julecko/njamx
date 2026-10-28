#ifndef NETWORK_SCANNER_H
#define NETWORK_SCANNER_H

#include <stdint.h>

#include "network/network.h"
#include "network/socket.h"

/**
 * @brief Send an ARP request to every device in the network and mark the
 *        ones that reply as alive.
 *
 * Sends a request to every device first, then listens for replies until
 * either every device has answered or timeout_sec elapses since replies
 * started being collected, whichever comes first. Devices that answered
 * are left in the DEVICE_STATE_ALIVE state; all others keep whatever
 * state they had. Found devices are debug-printed (available only in
 * debug builds).
 *
 * @param network     Network whose devices are probed (device states are
 *                    updated in place).
 * @param sock        Already-open ARP socket (ETH_P_ARP) bound to the
 *                    scanning interface.
 * @param src_mac     Source MAC address to use in the outgoing ARP
 *                    requests.
 * @param timeout_sec Total time to spend waiting for replies, in seconds.
 * @return Number of devices found alive, or -1 if no request could be sent.
 */
int scanner_scan(network_t *network, const network_socket_t *sock, const uint8_t src_mac[6],
                  int timeout_sec);

#endif
