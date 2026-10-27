#include <stdint.h>
#include <stdio.h>

#include <unistd.h>

#include <linux/if_ether.h>

#include "cli_args.h"
#include "network/device.h"
#include "network/forgery.h"
#include "network/iface.h"
#include "network/network.h"
#include "network/scanner.h"
#include "network/socket.h"
#include "version.h"

int main(int argc, char *argv[]) {
    cli_args args = cli_args_parse(argc, argv);

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

    network_t network = network_create(args.ip);
    if (!network.devices || network.length == 0) {
        fprintf(stderr, "Network initialization went wrong\n");
        return 1;
    }

    for (size_t i = 0; i < network.length; i++) {
        device_t device = network.devices[i];
#ifdef DEBUG_MODE
        network_device_print(&device);
#endif
    }
    for (int i = 0; i < 10; i++) {
        sleep(1);
        device_t device = network_forgery_device(&network);
        network_device_print(&device);
    }
    /*uint8_t my_mac[6];
    if (iface_get_mac(args.interface, my_mac) < 0) {
        fprintf(stderr, "Failed to get MAC address of %s\n", args.interface);
        network_delete(&network);
        return 1;
    }

    network_socket_t sock;
    if (network_socket_open(&sock, args.interface, ETH_P_ARP, 1) < 0) {
        network_delete(&network);
        return 1;
    }

    int found = scanner_scan(&network, &sock, my_mac);
    printf("Found %d device(s) alive out of %zu\n", found < 0 ? 0 : found, network.length);

    network_socket_close(&sock);*/

    network_delete(&network);
    return 0;
}
