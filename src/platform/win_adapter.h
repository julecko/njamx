#ifndef PLATFORM_WIN_ADAPTER_H
#define PLATFORM_WIN_ADAPTER_H

#ifdef _WIN32

#include <stdint.h>

typedef struct win_adapter_info {
    // GUID of the adapter as it appears in the registry / IP Helper API,
    // e.g. "{4D36E972-...}". Also the suffix of the matching Npcap
    // device name ("\Device\NPF_{4D36E972-...}").
    char guid[64];
    uint8_t mac[6];
} win_adapter_info_t;

/**
 * @brief Look up a network adapter by its friendly name (the name shown
 *        in "Network Connections", e.g. "Ethernet" or "Wi-Fi").
 *
 * @param friendly_name Adapter friendly name, case-insensitive.
 * @param out            Out: adapter GUID and MAC address on success.
 * @return 0 on success, -1 if not found.
 */
int win_adapter_find_by_name(const char *friendly_name, win_adapter_info_t *out);

#endif
#endif
