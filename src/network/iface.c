#include "network/iface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iphlpapi.h>

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

int network_iface_list(network_iface_info_t out[NETWORK_IFACE_LIST_MAX]) {
    ULONG size = 0;
    ULONG rc = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, NULL, &size);
    if (rc != ERROR_BUFFER_OVERFLOW)
        return -1;

    IP_ADAPTER_ADDRESSES *addrs = (IP_ADAPTER_ADDRESSES *)malloc(size);
    if (!addrs)
        return -1;

    rc = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, addrs, &size);
    if (rc != NO_ERROR) {
        free(addrs);
        return -1;
    }

    int count = 0;
    for (IP_ADAPTER_ADDRESSES *a = addrs; a != NULL && count < NETWORK_IFACE_LIST_MAX;
         a = a->Next) {
        if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
            continue;

        int n = WideCharToMultiByte(CP_UTF8, 0, a->FriendlyName, -1, out[count].name,
                                    sizeof(out[count].name), NULL, NULL);
        if (n <= 0)
            continue;
        count++;
    }

    free(addrs);
    return count;
}

int network_iface_guess_ip(const char *ifname, char *out, size_t out_cap) {
    ULONG size = 0;
    ULONG rc = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, NULL, &size);
    if (rc != ERROR_BUFFER_OVERFLOW)
        return -1;

    IP_ADAPTER_ADDRESSES *addrs = (IP_ADAPTER_ADDRESSES *)malloc(size);
    if (!addrs)
        return -1;

    rc = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, addrs, &size);
    if (rc != NO_ERROR) {
        free(addrs);
        return -1;
    }

    int found = -1;
    for (IP_ADAPTER_ADDRESSES *a = addrs; a != NULL; a = a->Next) {
        char name[256];
        int n = WideCharToMultiByte(CP_UTF8, 0, a->FriendlyName, -1, name, sizeof(name), NULL,
                                    NULL);
        if (n <= 0 || _stricmp(name, ifname) != 0)
            continue;

        for (IP_ADAPTER_UNICAST_ADDRESS *ua = a->FirstUnicastAddress; ua != NULL;
             ua = ua->Next) {
            if (ua->Address.lpSockaddr->sa_family != AF_INET)
                continue;

            struct sockaddr_in *sin = (struct sockaddr_in *)ua->Address.lpSockaddr;
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sin->sin_addr, ip_str, sizeof(ip_str));
            snprintf(out, out_cap, "%s/%u", ip_str, (unsigned int)ua->OnLinkPrefixLength);
            found = 0;
            break;
        }
        break;
    }

    free(addrs);
    return found;
}

#else

#include <stdbool.h>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ifaddrs.h>
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

int network_iface_list(network_iface_info_t out[NETWORK_IFACE_LIST_MAX]) {
    struct ifaddrs *addrs;
    if (getifaddrs(&addrs) < 0) {
        perror("getifaddrs");
        return -1;
    }

    int count = 0;
    for (struct ifaddrs *a = addrs; a != NULL && count < NETWORK_IFACE_LIST_MAX;
         a = a->ifa_next) {
        if (!a->ifa_name || (a->ifa_flags & IFF_LOOPBACK))
            continue;

        bool seen = false;
        for (int i = 0; i < count; i++) {
            if (strcmp(out[i].name, a->ifa_name) == 0) {
                seen = true;
                break;
            }
        }
        if (seen)
            continue;

        snprintf(out[count].name, sizeof(out[count].name), "%s", a->ifa_name);
        count++;
    }

    freeifaddrs(addrs);
    return count;
}

int network_iface_guess_ip(const char *ifname, char *out, size_t out_cap) {
    struct ifaddrs *addrs;
    if (getifaddrs(&addrs) < 0) {
        perror("getifaddrs");
        return -1;
    }

    int found = -1;
    for (struct ifaddrs *a = addrs; a != NULL; a = a->ifa_next) {
        if (!a->ifa_name || strcmp(a->ifa_name, ifname) != 0)
            continue;
        if (!a->ifa_addr || a->ifa_addr->sa_family != AF_INET)
            continue;

        struct sockaddr_in *sin = (struct sockaddr_in *)a->ifa_addr;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sin->sin_addr, ip_str, sizeof(ip_str));

        int prefix = 0;
        if (a->ifa_netmask) {
            uint32_t m = ntohl(((struct sockaddr_in *)a->ifa_netmask)->sin_addr.s_addr);
            while (m) {
                prefix += (int)(m & 1);
                m >>= 1;
            }
        }

        snprintf(out, out_cap, "%s/%d", ip_str, prefix);
        found = 0;
        break;
    }

    freeifaddrs(addrs);
    return found;
}

#endif
