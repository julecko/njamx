#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>

#include <linux/if_ether.h>

#include "cli_args.h"
#include "network/device.h"
#include "network/forgery.h"
#include "network/iface.h"
#include "network/network.h"
#include "network/scanner.h"
#include "network/socket.h"
#include "tools/mac_change.h"
#include "version.h"

int main(int argc, char *argv[]) {
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

    srand((unsigned int)time(NULL));

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
        mac_change_set(args.interface, program_mac);

        printf("New mac address is: %02x:%02x:%02x:%02x:%02x:%02x\n", program_mac[0],
               program_mac[1], program_mac[2], program_mac[3], program_mac[4], program_mac[5]);
        sleep(2);
    } else {
        memcpy(program_mac, my_mac, 6);
    }

    network_t network = network_create(args.ip);
    if (!network.devices || network.length == 0) {
        fprintf(stderr, "Network initialization went wrong\n");
        return 1;
    }

    network_socket_t sock;
    if (network_socket_open(&sock, args.interface, ETH_P_ARP, 1) < 0) {
        network_delete(&network);
        return 1;
    }

    int found = scanner_scan(&network, &sock, program_mac);
    printf("Found %d device(s) alive out of %zu\n", found < 0 ? 0 : found, network.length);

    network_socket_close(&sock);
    network_delete(&network);
    if (args.flags & CLI_ARGS_FLAGS_CHANGE_MAC) {
        mac_change_set(args.interface, my_mac);
    }
    return 0;
}
