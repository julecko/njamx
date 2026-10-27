#include "socket.h"

#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>

#include "network/network.h"

int network_socket_open(network_socket_t *s, const char *ifname, uint16_t ethertype,
                        int timeout_sec) {
    s->ifindex = if_nametoindex(ifname);
    if (s->ifindex == 0) {
        perror("if_nametoindex");
        return -1;
    }

    s->fd = socket(AF_PACKET, SOCK_RAW, htons(ethertype));
    if (s->fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_ll addr = {
        .sll_family = AF_PACKET,
        .sll_protocol = htons(ethertype),
        .sll_ifindex = s->ifindex,
    };
    if (bind(s->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(s->fd);
        return -1;
    }

    struct timeval tv = {.tv_sec = timeout_sec};
    if (setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        close(s->fd);
        return -1;
    }
    return 0;
}

ssize_t network_socket_send(const network_socket_t *s, const uint8_t dst_mac[6], const void *frame,
                            size_t len) {
    struct sockaddr_ll addr = {
        .sll_family = AF_PACKET,
        .sll_ifindex = s->ifindex,
        .sll_halen = ETH_ALEN,
    };
    memcpy(addr.sll_addr, dst_mac, ETH_ALEN);
    ssize_t n = sendto(s->fd, frame, len, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (n < 0)
        perror("sendto");
    return n;
}

ssize_t network_socket_recv(const network_socket_t *s, void *buf, size_t len) {
    return recvfrom(s->fd, buf, len, 0, NULL, NULL);
}

void network_socket_close(network_socket_t *s) {
    if (s->fd >= 0)
        close(s->fd);
    s->fd = -1;
}
