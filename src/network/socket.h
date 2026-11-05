#ifndef NETWORK_SOCKET_H
#define NETWORK_SOCKET_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h>
#endif

#include <pcap.h>

#include "network/network.h"

#ifdef __cplusplus
extern "C" {
#endif

// Backed by libpcap on Linux and Npcap (the Windows port of libpcap's
// API) on Windows, so the same capture/injection code runs unmodified
// on both platforms.
typedef struct network_socket {
    pcap_t *handle;
    long timeout_sec;
    long timeout_usec;
} network_socket_t;

int network_socket_open(network_socket_t *s, const char *ifname, uint16_t ethertype,
                        int timeout_sec);
int network_socket_set_timeout(const network_socket_t *s, long sec, long usec);
ssize_t network_socket_send(const network_socket_t *s, const uint8_t dst_mac[6], const void *frame,
                            size_t len);
ssize_t network_socket_recv(const network_socket_t *s, void *buf, size_t len);
void network_socket_close(network_socket_t *s);

#ifdef __cplusplus
}
#endif

#endif
