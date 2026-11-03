#ifndef NETWORK_ETH_ARP_H
#define NETWORK_ETH_ARP_H

#include <stdint.h>

// Self-contained Ethernet/ARP wire format definitions. Linux's
// net/ethernet.h, netinet/if_ether.h and net/if_arp.h don't exist on
// Windows, so the on-wire structs and constants are defined here instead
// of pulled from OS headers, keeping the layout identical on every
// platform.

#define ETH_ALEN 6

#define ETHERTYPE_IP 0x0800
#define ETHERTYPE_ARP 0x0806

#define ARPHRD_ETHER 1
#define ARPOP_REQUEST 1
#define ARPOP_REPLY 2

#if defined(_MSC_VER)
#pragma pack(push, 1)
#define NJAMX_PACKED
#else
#define NJAMX_PACKED __attribute__((packed))
#endif

typedef struct NJAMX_PACKED eth_header {
    uint8_t ether_dhost[ETH_ALEN];
    uint8_t ether_shost[ETH_ALEN];
    uint16_t ether_type;
} eth_header_t;

typedef struct NJAMX_PACKED arp_header {
    uint16_t ar_hrd;
    uint16_t ar_pro;
    uint8_t ar_hln;
    uint8_t ar_pln;
    uint16_t ar_op;
    uint8_t arp_sha[ETH_ALEN];
    uint8_t arp_spa[4];
    uint8_t arp_tha[ETH_ALEN];
    uint8_t arp_tpa[4];
} arp_header_t;

#if defined(_MSC_VER)
#pragma pack(pop)
#endif
#undef NJAMX_PACKED

#endif
