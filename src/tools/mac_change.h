#ifndef TOOLS_MAC_CHANGE_H
#define TOOLS_MAC_CHANGE_H

#include <stdint.h>

int mac_change_set(const char *ifname, const uint8_t mac[6]);

#endif
