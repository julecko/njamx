#include "arp.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#include "network/eth_arp.h"
#include "socket.h"

static const uint8_t BROADCAST[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

#if defined(_MSC_VER)
#pragma pack(push, 1)
struct arp_frame {
    eth_header_t eth;
    arp_header_t arp;
};
#pragma pack(pop)
#else
struct arp_frame {
    eth_header_t eth;
    arp_header_t arp;
} __attribute__((packed));
#endif

static void arp_header_common(struct arp_frame *f, uint16_t op) {
    f->arp.ar_hrd = htons(ARPHRD_ETHER);
    f->arp.ar_pro = htons(ETHERTYPE_IP);
    f->arp.ar_hln = 6;
    f->arp.ar_pln = 4;
    f->arp.ar_op = htons(op);
}

int arp_send_request(const network_socket_t *s, const uint8_t src_mac[6], uint32_t src_ip,
                     uint32_t target_ip) {
    struct arp_frame f;

    memcpy(f.eth.ether_dhost, BROADCAST, 6);
    memcpy(f.eth.ether_shost, src_mac, 6);
    f.eth.ether_type = htons(ETHERTYPE_ARP);

    arp_header_common(&f, ARPOP_REQUEST);
    memcpy(f.arp.arp_sha, src_mac, 6);
    memcpy(f.arp.arp_spa, &src_ip, 4);
    memset(f.arp.arp_tha, 0, 6);
    memcpy(f.arp.arp_tpa, &target_ip, 4);

    return network_socket_send(s, BROADCAST, &f, sizeof(f)) < 0 ? -1 : 0;
}

int arp_send_reply(const network_socket_t *s, const uint8_t dst_mac[6], const uint8_t sender_mac[6],
                   uint32_t sender_ip, const uint8_t target_mac[6], uint32_t target_ip) {
    struct arp_frame f;

    memcpy(f.eth.ether_dhost, dst_mac, 6);
    memcpy(f.eth.ether_shost, sender_mac, 6);
    f.eth.ether_type = htons(ETHERTYPE_ARP);

    arp_header_common(&f, ARPOP_REPLY);
    memcpy(f.arp.arp_sha, sender_mac, 6);
    memcpy(f.arp.arp_spa, &sender_ip, 4);
    memcpy(f.arp.arp_tha, target_mac, 6);
    memcpy(f.arp.arp_tpa, &target_ip, 4);

    return network_socket_send(s, dst_mac, &f, sizeof(f)) < 0 ? -1 : 0;
}

int arp_wait_reply(const network_socket_t *s, uint32_t target_ip, uint8_t reply_mac[6]) {
    struct arp_frame f;
    for (;;) {
        ssize_t n = network_socket_recv(s, &f, sizeof(f));
        if (n < (ssize_t)sizeof(f))
            return -1;
        if (ntohs(f.arp.ar_op) != ARPOP_REPLY)
            continue;
        if (memcmp(f.arp.arp_spa, &target_ip, 4) != 0)
            continue;
        memcpy(reply_mac, f.arp.arp_sha, 6);
        return 0;
    }
}

int arp_recv_reply(const network_socket_t *s, uint32_t *sender_ip, uint8_t sender_mac[6]) {
    struct arp_frame f;
    for (;;) {
        ssize_t n = network_socket_recv(s, &f, sizeof(f));
        if (n < (ssize_t)sizeof(f))
            return -1;
        if (ntohs(f.arp.ar_op) != ARPOP_REPLY)
            continue;

        uint32_t raw_ip;
        memcpy(&raw_ip, f.arp.arp_spa, 4);
        *sender_ip = ntohl(raw_ip);
        memcpy(sender_mac, f.arp.arp_sha, 6);
        return 0;
    }
}

static void format_mac(const uint8_t mac[6], char *out, size_t out_cap) {
    snprintf(out, out_cap, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4],
             mac[5]);
}

void arp_frame_describe(const uint8_t *frame, size_t len, char *out, size_t out_cap) {
    if (len < sizeof(struct arp_frame)) {
        snprintf(out, out_cap, "(truncated frame, %zu byte(s))", len);
        return;
    }
    const struct arp_frame *f = (const struct arp_frame *)frame;

    char sha[18], tha[18];
    format_mac(f->arp.arp_sha, sha, sizeof(sha));
    format_mac(f->arp.arp_tha, tha, sizeof(tha));

    struct in_addr spa_addr, tpa_addr;
    memcpy(&spa_addr, f->arp.arp_spa, 4);
    memcpy(&tpa_addr, f->arp.arp_tpa, 4);
    char spa[INET_ADDRSTRLEN], tpa[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &spa_addr, spa, sizeof(spa));
    inet_ntop(AF_INET, &tpa_addr, tpa, sizeof(tpa));

    uint16_t op = ntohs(f->arp.ar_op);
    const char *op_str = op == ARPOP_REQUEST ? "request" : op == ARPOP_REPLY ? "reply" : "?";

    snprintf(out, out_cap, "ARP %-7s  %s (%s)  ->  %s (%s)", op_str, sha, spa, tha, tpa);
}
