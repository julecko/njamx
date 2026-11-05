#ifndef PROCESS_GUI_H
#define PROCESS_GUI_H

#include <stdatomic.h>

#include "network/network.h"
#include "network/socket.h"
#include "process/runtime_options.h"

/**
 * @brief Graphical equivalent of cli_run(): a single raylib window with
 *        the device table, jam/router/add/remove controls, and the
 *        randommac/changemac/selfjam toggles, blocking until the window
 *        is closed or "Quit" is clicked.
 *
 * Drives the exact same command dispatch cli_run_command() uses (see
 * cli_commands.c), so behavior matches the terminal UI exactly - this
 * is purely an alternative presentation layer, sharing the same network,
 * socket, jammer thread and runtime_options as cli_run().
 *
 * @param network     Network whose devices are displayed and (re)scanned.
 * @param sock        Already-open ARP socket used by the "scan" command.
 * @param opts        Jamming/MAC runtime options, shared with the jammer
 *                    thread and live-togglable via the GUI's toggles.
 * @param timeout_sec Timeout passed to scanner_scan for "scan".
 * @param quit        Shared with other threads (e.g. the jammer); set to
 *                    true when the window closes so they can stop too.
 */
void gui_run(network_t *network, const network_socket_t *sock, runtime_options_t *opts,
            atomic_int *timeout_sec, atomic_bool *quit);

#endif
