#ifndef CLI_ARGS_H
#define CLI_ARGS_H

#include <stdio.h>

typedef enum cli_args_result {
    CLI_ARGS_SUCCESS,
    CLI_ARGS_ERROR,
    CLI_ARGS_HELP,
} cli_args_result;

typedef struct cli_args {
    cli_args_result result;
    const char *interface;
    const char *ip;
} cli_args;

cli_args cli_args_parse(int argc, char *argv[]);
void cli_args_print_usage(const char *prog_name, FILE *stream);

#ifdef DEBUG_MODE
void cli_args_print_debug(cli_args args);
#endif

#endif
