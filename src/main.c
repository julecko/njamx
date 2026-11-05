#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

#include "cli_args.h"
#include "network/device.h"
#include "network/eth_arp.h"
#include "network/forgery.h"
#include "network/iface.h"
#include "network/network.h"
#include "network/packet_log.h"
#include "network/scanner.h"
#include "network/socket.h"
#include "platform/compat.h"
#include "platform/thread_compat.h"
#include "process/cli.h"
#include "process/jammer.h"
#include "process/runtime_options.h"
#include "tools/mac_change.h"
#include "tools/privileges.h"
#include "version.h"

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif

    cli_args_t args = cli_args_parse(argc, argv);

#ifdef DEBUG_MODE
    cli_args_print_debug(args);
#endif

    switch (args.result) {
        case CLI_ARGS_HELP:
            cli_args_print_usage(argv[0], stdout);
            return 0;
        case CLI_ARGS_ERROR:
            cli_args_print_usage(argv[0], stderr);
            return 1;
        case CLI_ARGS_SUCCESS:
            break;
    }

    if (!privileges_has_raw_capture()) {
        fprintf(stderr,
                "njamx needs root or the CAP_NET_RAW capability (raw packet capture/injection).\n"
                "Run with sudo, or: sudo setcap cap_net_raw+ep %s\n",
                argv[0]);
        return 1;
    }

    srand((unsigned int)time(NULL));
    packet_log_init();

    uint8_t my_mac[6];
    if (network_iface_get_mac(args.interface, my_mac) < 0) {
        fprintf(stderr, "Couldnt get current interface\n");
        return 1;
    }

    uint8_t program_mac[6];
    if (args.flags & CLI_ARGS_FLAGS_CHANGE_MAC) {
        printf("Original mac address is: %02x:%02x:%02x:%02x:%02x:%02x\n", my_mac[0], my_mac[1],
               my_mac[2], my_mac[3], my_mac[4], my_mac[5]);

        network_forgery_mac(program_mac);
        if (mac_change_set(args.interface, program_mac) < 0) {
#ifdef _WIN32
            fprintf(stderr,
                    "Could not change MAC address on '%s' (run from an elevated/Administrator "
                    "terminal, and make sure the adapter's driver supports the NetworkAddress "
                    "override)\n",
                    args.interface);
#else
            fprintf(stderr,
                    "Could not change MAC address on '%s' (needs CAP_NET_ADMIN or root)\n",
                    args.interface);
#endif
            return 1;
        }

        printf("New mac address is: %02x:%02x:%02x:%02x:%02x:%02x\n", program_mac[0],
               program_mac[1], program_mac[2], program_mac[3], program_mac[4], program_mac[5]);
        platform_sleep_sec(2);
    } else {
        memcpy(program_mac, my_mac, 6);
    }

    network_t network = network_create(args.ip);
    pthread_mutex_lock(&network.lock);
    bool network_empty = !network.devices || network.length == 0;
    pthread_mutex_unlock(&network.lock);
    if (network_empty) {
        fprintf(stderr, "Network initialization went wrong\n");
        return 1;
    }
    network_set_my_mac(&network, program_mac);

    network_socket_t sock;
    if (network_socket_open(&sock, args.interface, ETHERTYPE_ARP, 1) < 0) {
        network_delete(&network);
        return 1;
    }

    atomic_int timeout_sec = args.timeout_sec;
    atomic_bool quit = false;

    runtime_options_t opts;
    runtime_options_init(&opts, args.interface, my_mac, program_mac,
                         (args.flags & CLI_ARGS_FLAGS_REAL_MAC) != 0);

    jammer_args_t jammer_args = {
        .network = &network, .stop_flag = &quit, .network_socket = sock, .opts = &opts};
    pthread_t jammer_thread;
    if (pthread_create(&jammer_thread, NULL, jammer_process, &jammer_args) != 0) {
        fprintf(stderr, "Couldnt start jammer thread\n");
        network_socket_close(&sock);
        network_delete(&network);
        return 1;
    }

    cli_run(&network, &sock, &opts, &timeout_sec, &quit);

    printf("Cleaning up, can take up few seconds...\n");
    pthread_join(jammer_thread, NULL);

    network_socket_close(&sock);
    network_delete(&network);
    if (args.flags & CLI_ARGS_FLAGS_CHANGE_MAC) {
        mac_change_set(args.interface, my_mac);
    }
    return 0;
}
