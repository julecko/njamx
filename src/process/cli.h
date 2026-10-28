#ifndef PROCESS_CLI_H
#define PROCESS_CLI_H

#include <stdatomic.h>
#include <stdint.h>

#include "network/network.h"
#include "network/socket.h"

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
 * @param src_mac     Source MAC to use for ARP requests sent by "scan".
 * @param timeout_sec Timeout passed to scanner_scan for "scan". Taken by
 *                    reference and atomic so another thread can share it.
 * @param quit        Shared with other threads (e.g. the jammer); set to
 *                    true when the user quits so they can stop too.
 */
void cli_run(network_t *network, const network_socket_t *sock, const uint8_t src_mac[6],
             atomic_int *timeout_sec, atomic_bool *quit);

#endif
