#include "arp.h"

#include <string.h>

#include <arpa/inet.h>
#include <netinet/if_ether.h>

#include <net/ethernet.h>

#include "socket.h"

static const uint8_t BROADCAST[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct arp_frame {
    struct ether_header eth;
    struct ether_arp arp;
} __attribute__((packed));

static void arp_header_common(struct arp_frame *f, uint16_t op) {
    f->arp.ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
    f->arp.ea_hdr.ar_pro = htons(ETH_P_IP);
    f->arp.ea_hdr.ar_hln = 6;
    f->arp.ea_hdr.ar_pln = 4;
    f->arp.ea_hdr.ar_op = htons(op);
}

int arp_send_request(const network_socket_t *s, const uint8_t src_mac[6], uint32_t src_ip,
                     uint32_t target_ip) {
    struct arp_frame f;

    memcpy(f.eth.ether_dhost, BROADCAST, 6);
    memcpy(f.eth.ether_shost, src_mac, 6);
    f.eth.ether_type = htons(ETH_P_ARP);

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
    f.eth.ether_type = htons(ETH_P_ARP);

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
        if (ntohs(f.arp.ea_hdr.ar_op) != ARPOP_REPLY)
            continue;
        if (memcmp(f.arp.arp_spa, &target_ip, 4) != 0)
            continue;
        memcpy(reply_mac, f.arp.arp_sha, 6);
        return 0;
    }
}
