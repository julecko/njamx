#include "forgery.h"

#include <stdint.h>
#include <stdlib.h>

void network_forgery_mac(uint8_t mac[6]) {
    for (int i = 0; i < 6; i++) {
        mac[i] = rand() % 256;
    }

    // Clear multicast bit (bit 0 of first byte) -> unicast address.
    mac[0] &= 0xFE;
}
