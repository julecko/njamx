#include "process/cli_commands.h"

#include <stdio.h>
#include <string.h>

#include "network/scanner.h"

#define VERB_MAX 64

typedef void (*cli_command_fn)(const char *args, cli_context_t *ctx, char *status_line,
                                size_t status_cap, bool *should_quit);

static void cmd_help(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                      bool *should_quit) {
    (void)args;
    (void)ctx;
    (void)should_quit;
    snprintf(status_line, status_cap, "Commands: help, scan, quit/exit/q");
}

static void cmd_scan(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                      bool *should_quit) {
    (void)args;
    (void)should_quit;
    int found = scanner_scan(ctx->network, ctx->sock, ctx->src_mac, ctx->timeout_sec);
    if (found < 0)
        snprintf(status_line, status_cap, "Scan failed: couldn't send any ARP request");
    else
        snprintf(status_line, status_cap, "Scan complete: %d device(s) alive", found);
}

static void cmd_quit(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                      bool *should_quit) {
    (void)args;
    (void)ctx;
    (void)status_line;
    (void)status_cap;
    *should_quit = true;
}

typedef struct {
    const char *name;
    cli_command_fn fn;
} cli_command_entry_t;

static const cli_command_entry_t COMMANDS[] = {
    {"help", cmd_help}, {"scan", cmd_scan}, {"quit", cmd_quit}, {"exit", cmd_quit},
    {"q", cmd_quit},
};
#define COMMAND_COUNT (sizeof(COMMANDS) / sizeof(COMMANDS[0]))

void cli_run_command(const char *input, cli_context_t *ctx, char *status_line, size_t status_cap,
                      bool *should_quit) {
    if (input[0] == '\0') {
        status_line[0] = '\0';
        return;
    }

    char verb[VERB_MAX];
    const char *args;
    const char *space = strchr(input, ' ');
    if (space) {
        size_t verb_len = (size_t)(space - input);
        if (verb_len >= sizeof(verb))
            verb_len = sizeof(verb) - 1;
        memcpy(verb, input, verb_len);
        verb[verb_len] = '\0';

        args = space + 1;
        while (*args == ' ')
            args++;
    } else {
        strncpy(verb, input, sizeof(verb) - 1);
        verb[sizeof(verb) - 1] = '\0';
        args = "";
    }

    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if (strcmp(verb, COMMANDS[i].name) == 0) {
            COMMANDS[i].fn(args, ctx, status_line, status_cap, should_quit);
            return;
        }
    }
    snprintf(status_line, status_cap, "Unknown command: '%s' (try 'help')", verb);
}
