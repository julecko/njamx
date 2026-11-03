#include "network/iface.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32

#include "platform/win_adapter.h"

int network_iface_get_mac(const char *ifname, uint8_t mac[6]) {
    win_adapter_info_t info;
    if (win_adapter_find_by_name(ifname, &info) < 0) {
        fprintf(stderr, "Could not find network interface '%s'\n", ifname);
        return -1;
    }
    memcpy(mac, info.mac, 6);
    return 0;
}

#else

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <net/if.h>

int network_iface_get_mac(const char *ifname, uint8_t mac[6]) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    struct ifreq ifr = {0};
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl(SIOCGIFHWADDR)");
        close(fd);
        return -1;
    }

    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
    close(fd);
    return 0;
}

#endif
