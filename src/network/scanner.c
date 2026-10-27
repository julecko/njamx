#include "network/scanner.h"

#include <stddef.h>
#include <stdio.h>

#include <arpa/inet.h>

#include "network/arp.h"
#include "network/device.h"

int scanner_scan(network_t *network, const network_socket_t *sock, const uint8_t src_mac[6]) {
    uint32_t src_ip = htonl(network->my_ip);
    size_t sent = 0;

    for (size_t i = 0; i < network->length; i++) {
        device_t *device = &network->devices[i];
        if (arp_send_request(sock, src_mac, src_ip, htonl(device->ip)) < 0) {
            fprintf(stderr, "Failed to send ARP request for device %zu\n", i);
            continue;
        }
        device->state = DEVICE_STATE_ADDED;
        sent++;
    }

    if (sent == 0)
        return -1;

    int found = 0;
    uint32_t sender_ip;
    uint8_t sender_mac[6];
    while ((size_t)found < sent && arp_recv_reply(sock, &sender_ip, sender_mac) == 0) {
        for (size_t i = 0; i < network->length; i++) {
            device_t *device = &network->devices[i];
            if (device->ip == sender_ip && device->state != DEVICE_STATE_ALIVE) {
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
