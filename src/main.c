#include <stdint.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <linux/if_ether.h>

#include "cli_args.h"
#include "network/arp.h"
#include "network/device.h"
#include "network/network.h"
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

    network_socket_t sock;
    if (network_socket_open(&sock, "wlan0", ETH_P_ARP, 1) < 0)
        return 1;

    uint8_t my_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t requester[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}; // who asked
    uint32_t my_ip = inet_addr("192.168.1.50");
    uint32_t requester_ip = inet_addr("192.168.1.10");

    if (arp_send_reply(&sock, requester, my_mac, my_ip, requester, requester_ip) < 0)
        fprintf(stderr, "arp_send_reply failed\n");

    network_socket_close(&sock);

    network_delete(&network);
    return 0;
}
