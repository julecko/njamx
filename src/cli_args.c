#include "cli_args.h"

#include <stdio.h>
#include <string.h>

void cli_args_print_usage(const char *prog_name, FILE *stream) {
    fprintf(stream, "Usage: %s -i <interface>\n", prog_name);
    fprintf(stream, "\n");
    fprintf(stream, "  -i, --interface <name>   Network interface to use (required)\n");
    fprintf(stream, "  -h, --help               Show this help message\n");
}

#ifdef DEBUG_MODE
void cli_args_print_debug(cli_args args) {
    static const char *result_names[] = {
        [CLI_ARGS_SUCCESS] = "CLI_ARGS_SUCCESS",
        [CLI_ARGS_ERROR] = "CLI_ARGS_ERROR",
        [CLI_ARGS_HELP] = "CLI_ARGS_HELP",
    };

    fprintf(stderr, "[debug] result = %s\n", result_names[args.result]);
    fprintf(stderr, "[debug] interface = %s\n", args.interface ? args.interface : "(null)");
}
#endif

cli_args cli_args_parse(int argc, char *argv[]) {
    cli_args args = {0};
    args.result = CLI_ARGS_ERROR;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            args.result = CLI_ARGS_HELP;
            return args;
        }

        if (strcmp(arg, "-i") == 0 || strcmp(arg, "--interface") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s needs a value\n", arg);
                return args;
            }
            args.interface = argv[++i];
            continue;
        }

        fprintf(stderr, "Error: unknown argument '%s'\n", arg);
        return args;
    }

    if (args.interface == NULL) {
        fprintf(stderr, "Error: missing required -i/--interface option\n");
        return args;
    }

    args.result = CLI_ARGS_SUCCESS;
    return args;
}
