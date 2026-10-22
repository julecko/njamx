#include <stdint.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "cli_args.h"
#include "network/device.h"
#include "network/network.h"
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
        device_print(&device);
#endif
    }

    network_delete(&network);
    return 0;
}
