#include "network/scanner.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <arpa/inet.h>

#include "network/arp.h"
#include "network/device.h"

int scanner_scan(network_t *network, const network_socket_t *sock, const uint8_t src_mac[6],
                  int timeout_sec) {
    uint32_t src_ip = htonl(network->my_ip);
    size_t sent = 0;

    for (size_t i = 0; i < network->length; i++) {
        device_t *device = &network->devices[i];
        if (arp_send_request(sock, src_mac, src_ip, htonl(device->ip)) < 0) {
            fprintf(stderr, "Failed to send ARP request for device %zu\n", i);
            continue;
        }
        sent++;
    }

    if (sent == 0)
        return -1;

    struct timespec deadline;
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    deadline.tv_sec += timeout_sec;

    int found = 0;
    uint32_t sender_ip;
    uint8_t sender_mac[6];
    while ((size_t)found < sent) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        long remaining_ms = (deadline.tv_sec - now.tv_sec) * 1000 +
                             (deadline.tv_nsec - now.tv_nsec) / 1000000;
        if (remaining_ms <= 0)
            break;

        network_socket_set_timeout(sock, remaining_ms / 1000, (remaining_ms % 1000) * 1000);

        if (arp_recv_reply(sock, &sender_ip, sender_mac) != 0)
            break;

        for (size_t i = 0; i < network->length; i++) {
            device_t *device = &network->devices[i];
            if (device->ip == sender_ip && device->state != DEVICE_STATE_ALIVE) {
                memcpy(device->mac, sender_mac, sizeof(device->mac));
                device->state = DEVICE_STATE_ALIVE;
                found++;
                break;
            }
        }
    }

#ifdef DEBUG_MODE
    for (size_t i = 0; i < network->length; i++) {
        if (network->devices[i].state == DEVICE_STATE_ALIVE)
            network_device_print(&network->devices[i]);
    }
#endif

    return found;
}
