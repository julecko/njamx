#ifndef CLI_ARGS_H
#define CLI_ARGS_H

#include <stdio.h>

typedef enum cli_args_result {
    CLI_ARGS_SUCCESS,
    CLI_ARGS_ERROR,
    CLI_ARGS_HELP,
} cli_args_result_t;

typedef enum cli_flags {
    CLI_ARGS_FLAGS_CHANGE_MAC = 1 << 0,
} cli_flags_t;

typedef struct cli_args {
    cli_args_result_t result;
    const char *interface;
    const char *ip;
    cli_flags_t flags;
} cli_args_t;

cli_args_t cli_args_parse(int argc, char *argv[]);
void cli_args_print_usage(const char *prog_name, FILE *stream);

#ifdef DEBUG_MODE
void cli_args_print_debug(cli_args_t args);
#endif

#endif
