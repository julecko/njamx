#include "network/network.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "network/device.h"

network_t network_create(const char *ip_with_mask) {
    network_t network = {0};
    const char *slash = strchr(ip_with_mask, '/');
    if (!slash) {
        fprintf(stderr, "Slash was not found in IP\n");
        return network;
    }

    char ip[INET_ADDRSTRLEN];
    size_t ip_len = (size_t)(slash - ip_with_mask);
    if (ip_len == 0 || ip_len >= sizeof(ip)) {
        fprintf(stderr, "Ip length is bigger than expected\n");
        return network;
    }

    memcpy(ip, ip_with_mask, ip_len);
    ip[ip_len] = '\0';

    struct in_addr addr;
    if (inet_pton(AF_INET, ip, &addr) != 1) {
        fprintf(stderr, "Ip provided is not a valid ip\n");
        return network;
    }

    char *end;
    long prefix = strtol(slash + 1, &end, 10);
    if (end == slash + 1 || *end != '\0' || prefix < 0 || prefix > 32) {
        fprintf(stderr, "Invalid subnet mask\n");
        return network;
    }

    if (prefix > 30) {
        fprintf(stderr, "Subnet too small\n");
        return network;
    }

    uint32_t ip_host = ntohl(addr.s_addr);
    uint32_t mask = prefix == 0 ? 0 : ~0u << (32 - prefix);

    network.my_ip = ip_host;
    network.mask = mask;

    uint32_t ip_network = network_get_network_ip(&network);
    uint32_t ip_broadcast = network_get_broadcast_ip(&network);

    uint64_t network_len = (1ull << (32 - prefix)) - 2;
    network.length = network_len;

    network.devices = (device_t *)malloc(network.length * sizeof(*network.devices));
    if (!network.devices) {
        fprintf(stderr, "Allocation of device list failed\n");
        return network;
    }

    for (uint64_t n = ip_network + 1, i = 0; n < ip_broadcast; n++, i++) {
        network.devices[i] = network_device_create(n);
    }
    printf("Network len: %" PRIu64 "\n", network_len);
    return network;
}

void network_delete(network_t *network) {
    pthread_mutex_lock(&network->lock);
    if (network->devices) {
        free(network->devices);
        network->devices = NULL;
    }
    network->length = 0;
    pthread_mutex_unlock(&network->lock);
}
