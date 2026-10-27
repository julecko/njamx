#ifndef NETWORK_ARP_H
#define NETWORK_ARP_H

#include <stdint.h>

#include "socket.h"

int arp_send_request(const network_socket_t *s, const uint8_t src_mac[6], uint32_t src_ip,
                     uint32_t target_ip);

// Generic reply sender: every field is explicit, nothing is inferred.
// dst_mac    - Ethernet destination (who receives the frame on the wire)
// sender_mac - ARP "sender hardware address" (claimed MAC owning sender_ip)
// sender_ip  - ARP "sender protocol address" (claimed IP)
// target_mac - ARP "target hardware address" (goes to the requester)
// target_ip  - ARP "target protocol address" (the requester's IP)
int arp_send_reply(const network_socket_t *s, const uint8_t dst_mac[6], const uint8_t sender_mac[6],
                   uint32_t sender_ip, const uint8_t target_mac[6], uint32_t target_ip);

int arp_wait_reply(const network_socket_t *s, uint32_t target_ip, uint8_t reply_mac[6]);

// Receive the next ARP reply, whoever it is from (unlike arp_wait_reply,
// this does not filter by target IP, so no replies are discarded while
// probing many hosts at once). Blocks subject to the socket's receive
// timeout.
//
// sender_ip  - Out: replying host's IP, in host byte order.
// sender_mac - Out: replying host's MAC address.
// @return 0 on success, -1 on timeout/error.
int arp_recv_reply(const network_socket_t *s, uint32_t *sender_ip, uint8_t sender_mac[6]);

#endif
