#include <stdint.h>

#define ARP_PACKET_SIZE 42
#define MAC_LEN 6
#define IP_LEN 4

struct arp_hdr {
    uint16_t htype;       // Hardware type (Ethernet 0x0001)
    uint16_t ptype;       // Protocol type (IPv4 0x0800)
    uint8_t hlen;         // Hardware length (Mac 6)
    uint8_t plen;         // Protocol length (IPv4 4)
    uint16_t oper;        // Operation (1 Request 2 Response)
    uint8_t sha[MAC_LEN]; // Sender hardware address (AA:BB:CC:DD:FF)
    uint8_t spa[IP_LEN];  // Sender protocol address (192.168.0.1)
    uint8_t tha[MAC_LEN]; // Target hardware address (AA:BB:CC:DD:FF)
    uint8_t tpa[IP_LEN];  // Target protocol address (192.168.0.1)
} __attribute__((packed));
