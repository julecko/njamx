#include <stdint.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <net/if.h>
#include <net/if_arp.h>

int mac_change_set(const char *ifname, const uint8_t mac[6]) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return -1;

    struct ifreq ifr = {0};
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        close(fd);
        return -1;
    }
    short flags = ifr.ifr_flags;
    ifr.ifr_flags &= ~IFF_UP;
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
        close(fd);
        return -1;
    }

    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifr.ifr_hwaddr.sa_data, mac, 6);
    int rc = ioctl(fd, SIOCSIFHWADDR, &ifr);

    ifr.ifr_flags = flags | IFF_UP;
    ioctl(fd, SIOCSIFFLAGS, &ifr);

    close(fd);
    return rc < 0 ? -1 : 0;
}
