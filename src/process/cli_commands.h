#ifndef PROCESS_CLI_COMMANDS_H
#define PROCESS_CLI_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "network/network.h"
#include "network/socket.h"

// Everything a command handler might need, bundled so the dispatch table
// in cli_commands.c can stay a flat array of (name, function pointer)
// pairs.
typedef struct {
    network_t *network;
    const network_socket_t *sock;
    const uint8_t *src_mac;
    int timeout_sec;
} cli_context_t;

/**
 * @brief Parse "verb rest-of-line" out of input and run the matching
 *        command, writing a human-readable result into status_line.
 *
 * @param input       Full command line as typed (may be empty).
 * @param ctx         Shared scanning context passed to the handler.
 * @param status_line Out: result message, written as a NUL-terminated
 *                    string up to status_cap bytes.
 * @param status_cap  Size of the status_line buffer.
 * @param should_quit Out: set to true if the command requests exit.
 */
void cli_run_command(const char *input, cli_context_t *ctx, char *status_line, size_t status_cap,
                      bool *should_quit);

#endif
