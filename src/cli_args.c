#include "cli_args.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cli_args_print_usage(const char *prog_name, FILE *stream) {
#ifdef NJAMX_HAVE_GUI
    fprintf(stream, "Usage: %s [<ip> -i <interface>] [-g]\n", prog_name);
#else
    fprintf(stream, "Usage: %s <ip> -i <interface>\n", prog_name);
#endif
    fprintf(stream, "\n");
    fprintf(stream, "  -i, --interface <name>   Network interface to use (required)\n");
    fprintf(stream, "  -t, --timeout <seconds>  Time to wait for ARP replies (default: %d)\n",
            CLI_ARGS_DEFAULT_TIMEOUT_SEC);
    fprintf(stream, "  -m, --mac-change         Temporarily forge the MAC address while scanning\n");
    fprintf(stream,
            "  -R, --real-mac           Use the real/active MAC in forged ARP replies instead of "
            "a random one\n");
#ifdef NJAMX_HAVE_GUI
    fprintf(stream, "  -g, --gui                Launch the graphical interface instead of the "
                    "terminal one\n");
    fprintf(stream,
            "\nWith -g and no <ip>/-i, njamx shows a setup screen to fill in the IP, interface, "
            "etc. instead of requiring them on the command line.\n");
#endif
    fprintf(stream, "  -h, --help               Show this help message\n");
    fprintf(stream, "Examples: %s 10.10.10.1/23 -i eth0\n", prog_name);
}

#ifdef DEBUG_MODE
void cli_args_print_debug(cli_args_t args) {
    static const char *result_names[] = {
        [CLI_ARGS_SUCCESS] = "CLI_ARGS_SUCCESS",
        [CLI_ARGS_ERROR] = "CLI_ARGS_ERROR",
        [CLI_ARGS_HELP] = "CLI_ARGS_HELP",
    };

    fprintf(stderr, "[debug] result = %s\n", result_names[args.result]);
    fprintf(stderr, "[debug] ip = %s\n", args.ip ? args.ip : "(null)");
    fprintf(stderr, "[debug] interface = %s\n", args.interface ? args.interface : "(null)");
    fprintf(stderr, "[debug] timeout_sec = %d\n", args.timeout_sec);
}
#endif

cli_args_t cli_args_parse(int argc, char *argv[]) {
    cli_args_t args = {0};
    args.result = CLI_ARGS_ERROR;
    args.timeout_sec = CLI_ARGS_DEFAULT_TIMEOUT_SEC;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            args.result = CLI_ARGS_HELP;
            return args;
        } else if (strcmp(arg, "-i") == 0 || strcmp(arg, "--interface") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s needs a value\n", arg);
                return args;
            }
            args.interface = argv[++i];
            continue;
        } else if (strcmp(arg, "-m") == 0 || strcmp(arg, "--mac-change") == 0) {
            args.flags |= CLI_ARGS_FLAGS_CHANGE_MAC;
            continue;
        } else if (strcmp(arg, "-R") == 0 || strcmp(arg, "--real-mac") == 0) {
            args.flags |= CLI_ARGS_FLAGS_REAL_MAC;
            continue;
#ifdef NJAMX_HAVE_GUI
        } else if (strcmp(arg, "-g") == 0 || strcmp(arg, "--gui") == 0) {
            args.flags |= CLI_ARGS_FLAGS_GUI;
            continue;
#endif
        } else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--timeout") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s needs a value\n", arg);
                return args;
            }
            const char *value = argv[++i];
            char *end;
            long timeout = strtol(value, &end, 10);
            if (end == value || *end != '\0' || timeout <= 0) {
                fprintf(stderr, "Error: invalid timeout '%s'\n", value);
                return args;
            }
            args.timeout_sec = (int)timeout;
            continue;
        } else {
            if (args.ip == NULL) {
                args.ip = arg;
                continue;
            }
        }

        fprintf(stderr, "Error: unknown argument '%s'\n", arg);
        return args;
    }

#ifdef NJAMX_HAVE_GUI
    bool gui_requested = (args.flags & CLI_ARGS_FLAGS_GUI) != 0;
#else
    bool gui_requested = false;
#endif

    // The GUI prompts for whatever's missing itself, so only the
    // terminal UI requires these up front.
    if (!gui_requested) {
        if (args.interface == NULL) {
            fprintf(stderr, "Error: missing required -i/--interface option\n");
            return args;
        } else if (args.ip == NULL) {
            fprintf(stderr, "Error: missing required IP address\n");
            return args;
        }
    }

    args.result = CLI_ARGS_SUCCESS;
    return args;
}
