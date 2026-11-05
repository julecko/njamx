#ifndef PROCESS_GUI_H
#define PROCESS_GUI_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

#include "network/network.h"
#include "network/socket.h"
#include "process/runtime_options.h"

// Implemented in C++ (src/process/gui/, FLTK-based) and exposed here with
// C linkage so main.c can call it like any other C function.
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Graphical equivalent of cli_run(): an FLTK window with the device
 *        table, jam/router/add/remove controls, and the
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

/**
 * @brief Pre-flight window shown when njamx is launched without the
 *        IP/interface it needs (e.g. no arguments at all): lets the user
 *        pick an interface (IP/subnet is guessed from it automatically,
 *        see network_iface_guess_ip()) - plus the mac-change/real-mac/
 *        timeout options normally only settable from the command line -
 *        instead of requiring them on argv.
 *
 * @param ip_buf        IP/subnet buffer (e.g. "192.168.1.5/24"), pre-filled
 *                      with whatever was already given on argv; edited in
 *                      place.
 * @param ip_cap        Capacity of ip_buf.
 * @param iface_buf     Interface name buffer, pre-filled/edited the same
 *                      way; also fillable by picking a detected interface.
 * @param iface_cap     Capacity of iface_buf.
 * @param mac_change    In/out: --mac-change toggle.
 * @param real_mac      In/out: --real-mac toggle.
 * @param timeout_sec   In/out: ARP reply timeout in seconds.
 * @return true if the user clicked "Start" with valid values, false if
 *         they closed the window (caller should exit without starting).
 */
bool gui_run_setup(char *ip_buf, size_t ip_cap, char *iface_buf, size_t iface_cap,
                   bool *mac_change, bool *real_mac, int *timeout_sec);

#ifdef __cplusplus
}
#endif

#endif
