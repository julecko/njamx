#ifndef NETWORK_IFACE_H
#define NETWORK_IFACE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Look up the hardware (MAC) address of a network interface.
 *
 * @param ifname Interface name (e.g. "wlan0").
 * @param mac    Out: 6-byte MAC address, filled in on success.
 * @return 0 on success, -1 on error.
 */
int network_iface_get_mac(const char *ifname, uint8_t mac[6]);

#define NETWORK_IFACE_LIST_MAX 32
#define NETWORK_IFACE_NAME_LEN 128

typedef struct network_iface_info {
    // Name accepted by network_iface_get_mac() / network_socket_open():
    // the adapter's friendly name ("Wi-Fi", "Ethernet") on Windows, the
    // kernel interface name ("eth0", "wlan0") elsewhere.
    char name[NETWORK_IFACE_NAME_LEN];
} network_iface_info_t;

/**
 * @brief List locally available network interfaces (loopback excluded).
 *
 * @param out Out: array of up to NETWORK_IFACE_LIST_MAX entries.
 * @return Number of interfaces found (0..NETWORK_IFACE_LIST_MAX), or -1 on
 *         error.
 */
int network_iface_list(network_iface_info_t out[NETWORK_IFACE_LIST_MAX]);

/**
 * @brief Guess the IP/subnet to scan for an interface, in the "a.b.c.d/N"
 *        form network_create() expects - i.e. the interface's own IPv4
 *        address and CIDR prefix length, the same thing "ip a" (Linux) or
 *        "ipconfig" (Windows) show for it. Lets the GUI's setup screen
 *        pre-fill the IP field once an interface is chosen.
 *
 * @param ifname  Interface name, as from network_iface_list().
 * @param out     Out: "a.b.c.d/N" buffer.
 * @param out_cap Capacity of out.
 * @return 0 on success (the interface has an IPv4 address), -1 otherwise.
 */
int network_iface_guess_ip(const char *ifname, char *out, size_t out_cap);

#ifdef __cplusplus
}
#endif

#endif
