#ifndef TOOLS_PRIVILEGES_H
#define TOOLS_PRIVILEGES_H

#include <stdbool.h>

/**
 * @brief Check whether the process has enough privilege to send/receive
 *        raw packets (ARP spoofing, raw sockets, capture).
 *
 * On Linux: true if running as root (euid 0) or if CAP_NET_RAW is
 * present in the effective capability set.
 * On Windows: always true. Capture there goes through libpcap/Npcap,
 * which enforces its own driver privilege requirements.
 *
 * @return true if privileged enough, false otherwise.
 */
bool privileges_has_raw_capture(void);

#endif
