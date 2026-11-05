#include "socket.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "network/eth_arp.h"
#include "network/packet_log.h"
#include "platform/compat.h"

#ifdef _WIN32
#include "platform/win_adapter.h"
#endif

// Resolve the caller-facing interface name into the device name libpcap
// expects. On Linux this is a no-op (pcap takes the same "eth0"/"wlan0"
// names the kernel does). On Windows, Npcap devices are named
// "\Device\NPF_{<adapter GUID>}", so the user-friendly adapter name
// (e.g. "Ethernet", "Wi-Fi") is first resolved to its GUID via the IP
// Helper API, then matched against the Npcap device list.
static int resolve_device(const char *ifname, char *out, size_t out_len) {
#ifdef _WIN32
    win_adapter_info_t info;
    if (win_adapter_find_by_name(ifname, &info) < 0)
        return -1;

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs;
    if (pcap_findalldevs(&alldevs, errbuf) < 0)
        return -1;

    int found = -1;
    for (pcap_if_t *d = alldevs; d != NULL; d = d->next) {
        if (d->name && strstr(d->name, info.guid) != NULL) {
            snprintf(out, out_len, "%s", d->name);
            found = 0;
            break;
        }
    }
    pcap_freealldevs(alldevs);
    return found;
#else
    snprintf(out, out_len, "%s", ifname);
    return 0;
#endif
}

int network_socket_open(network_socket_t *s, const char *ifname, uint16_t ethertype,
                        int timeout_sec) {
    char device[256];
    if (resolve_device(ifname, device, sizeof(device)) < 0) {
        fprintf(stderr, "Could not find network interface '%s'\n", ifname);
        return -1;
    }

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_live(device, 65536, 1, 50, errbuf);
    if (!handle) {
        fprintf(stderr, "pcap_open_live(%s): %s\n", device, errbuf);
        return -1;
    }

    if (pcap_datalink(handle) != DLT_EN10MB) {
        fprintf(stderr, "Interface '%s' is not an Ethernet interface\n", ifname);
        pcap_close(handle);
        return -1;
    }

    char filter_exp[64];
    if (ethertype == ETHERTYPE_ARP)
        snprintf(filter_exp, sizeof(filter_exp), "arp");
    else
        snprintf(filter_exp, sizeof(filter_exp), "ether proto 0x%04x", ethertype);

    struct bpf_program prog;
    if (pcap_compile(handle, &prog, filter_exp, 1, PCAP_NETMASK_UNKNOWN) < 0) {
        fprintf(stderr, "pcap_compile: %s\n", pcap_geterr(handle));
        pcap_close(handle);
        return -1;
    }
    if (pcap_setfilter(handle, &prog) < 0) {
        fprintf(stderr, "pcap_setfilter: %s\n", pcap_geterr(handle));
        pcap_freecode(&prog);
        pcap_close(handle);
        return -1;
    }
    pcap_freecode(&prog);

    s->handle = handle;
    s->timeout_sec = timeout_sec;
    s->timeout_usec = 0;
    return 0;
}

int network_socket_set_timeout(const network_socket_t *s, long sec, long usec) {
    // The struct is logically mutable configuration (mirrors the
    // original SO_RCVTIMEO-on-an-fd design); constness here only
    // promises callers won't touch the pcap handle itself.
    network_socket_t *mutable_s = (network_socket_t *)s;
    mutable_s->timeout_sec = sec;
    mutable_s->timeout_usec = usec;
    return 0;
}

ssize_t network_socket_send(const network_socket_t *s, const uint8_t dst_mac[6], const void *frame,
                            size_t len) {
    (void)dst_mac; // destination is already encoded in the frame's Ethernet header
    packet_log_record(frame, len);
    if (pcap_sendpacket(s->handle, (const u_char *)frame, (int)len) != 0) {
        fprintf(stderr, "pcap_sendpacket: %s\n", pcap_geterr(s->handle));
        return -1;
    }
    return (ssize_t)len;
}

ssize_t network_socket_recv(const network_socket_t *s, void *buf, size_t len) {
    bool infinite = s->timeout_sec == 0 && s->timeout_usec == 0;
    uint64_t deadline = infinite ? 0
                                 : platform_monotonic_ms() + (uint64_t)s->timeout_sec * 1000ull +
                                       (uint64_t)s->timeout_usec / 1000ull;

    for (;;) {
        struct pcap_pkthdr *hdr;
        const u_char *data;
        int rc = pcap_next_ex(s->handle, &hdr, &data);
        if (rc == 1) {
            size_t n = hdr->caplen < len ? hdr->caplen : len;
            memcpy(buf, data, n);
            return (ssize_t)n;
        }
        if (rc < 0)
            return -1;

        // rc == 0: the handle's internal read timeout elapsed with no
        // packet available; keep polling until our own deadline passes.
        if (!infinite && platform_monotonic_ms() >= deadline)
            return -1;
    }
}

void network_socket_close(network_socket_t *s) {
    if (s->handle)
        pcap_close(s->handle);
    s->handle = NULL;
}
