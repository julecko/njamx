#include <stdio.h>

#include "cli_args.h"
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

    printf("Project version: %s\n", NJAMX_VERSION);
    return 0;
}
