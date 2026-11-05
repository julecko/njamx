#ifndef PROCESS_CLI_H
#define PROCESS_CLI_H

#include <stdatomic.h>
#include <stdint.h>

#include "network/network.h"
#include "network/socket.h"
#include "process/runtime_options.h"

/**
 * @brief Show a scrollable, color-coded table of every known (non-dead)
 *        device in the network, with an msfconsole-style command prompt
 *        at the bottom, and block until the user quits.
 *
 * Enters the terminal's alternate screen and raw input mode for the
 * duration of the view. Up/Down arrows scroll the table; typed commands
 * are submitted with Enter. Supported commands: help, scan, quit/exit/q.
 * Ctrl-C also quits immediately.
 *
 * @param network     Network whose devices are displayed and (re)scanned.
 * @param sock        Already-open ARP socket used by the "scan" command.
 * @param opts        Jamming/MAC runtime options, shared with the jammer
 *                    thread and live-togglable via "randommac"/"changemac".
 * @param timeout_sec Timeout passed to scanner_scan for "scan". Taken by
 *                    reference and atomic so another thread can share it.
 * @param quit        Shared with other threads (e.g. the jammer); set to
 *                    true when the user quits so they can stop too.
 */
void cli_run(network_t *network, const network_socket_t *sock, runtime_options_t *opts,
             atomic_int *timeout_sec, atomic_bool *quit);

// Rows of terminal height available for listing content below the
// header/above the status+input lines. Shared with cli_commands.c's
// "packets" view so both scroll consistently with the terminal size.
size_t cli_get_visible_rows(void);

#endif
