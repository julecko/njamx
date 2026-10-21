#include <stdint.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "cli_args.h"
#include "network/device.h"
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

    const char *ip = "132.10.122.5";
    struct in_addr addr;
    if (inet_pton(AF_INET, ip, &addr) != 1) {
        fprintf(stderr, "Error happened in converting ip");
        return 1;
    }
    device_t device = device_create(ntohl(addr.s_addr));

#ifdef DEBUG_MODE
    device_print(&device);
#endif

    return 0;
}
