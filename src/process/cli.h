#ifndef PROCESS_CLI_H
#define PROCESS_CLI_H

#include "network/network.h"

/**
 * @brief Show a scrollable, color-coded table of every known (non-dead)
 *        device in the network and block until the user quits.
 *
 * Enters the terminal's alternate screen and raw input mode for the
 * duration of the view. Use Up/Down (or j/k) to scroll, q/Esc/Ctrl-C to
 * leave. Read-only: no device field is ever modified here.
 *
 * @param network Network whose devices are displayed.
 */
void cli_run(network_t *network);

#endif
