#include "network/scanner.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "network/arp.h"
#include "network/device.h"
#include "platform/compat.h"

int scanner_scan(network_t *network, const network_socket_t *sock, const uint8_t src_mac[6],
                 int timeout_sec) {
    uint32_t src_ip = htonl(network->my_ip);
    size_t sent = 0;

    pthread_mutex_lock(&network->lock);
    for (size_t i = 0; i < network->length; i++) {
        device_t *device = &network->devices[i];
        if (arp_send_request(sock, src_mac, src_ip, htonl(device->ip)) < 0) {
            fprintf(stderr, "Failed to send ARP request for device %zu\n", i);
            continue;
        }
        sent++;
    }
    pthread_mutex_unlock(&network->lock);

    if (sent == 0)
        return -1;

    uint64_t deadline_ms = platform_monotonic_ms() + (uint64_t)timeout_sec * 1000ull;

    int found = 0;
    uint32_t sender_ip;
    uint8_t sender_mac[6];
    while ((size_t)found < sent) {
        uint64_t now_ms = platform_monotonic_ms();
        if (now_ms >= deadline_ms)
            break;
        long remaining_ms = (long)(deadline_ms - now_ms);

        network_socket_set_timeout(sock, remaining_ms / 1000, (remaining_ms % 1000) * 1000);

        if (arp_recv_reply(sock, &sender_ip, sender_mac) != 0)
            break;

        pthread_mutex_lock(&network->lock);
        for (size_t i = 0; i < network->length; i++) {
            device_t *device = &network->devices[i];
            if (device->ip == sender_ip && device->state != DEVICE_STATE_ALIVE) {
                memcpy(device->mac, sender_mac, sizeof(device->mac));
                device->state = DEVICE_STATE_ALIVE;
                found++;
                break;
            }
        }
        pthread_mutex_unlock(&network->lock);
    }

    return found;
}
