#ifndef NETWORK_SOCKET_H
#define NETWORK_SOCKET_H

#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>

#include "network/network.h"

typedef struct network_socket {
    int fd;
    int ifindex;
} network_socket_t;

int network_socket_open(network_socket_t *s, const char *ifname, uint16_t ethertype,
                        int timeout_sec);
ssize_t network_socket_send(const network_socket_t *s, const uint8_t dst_mac[6], const void *frame,
                            size_t len);
ssize_t network_socket_recv(const network_socket_t *s, void *buf, size_t len);
void network_socket_close(network_socket_t *s);

#endif
