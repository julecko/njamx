#include "tools/mac_change.h"

#include <stdint.h>
#include <string.h>

#ifdef _WIN32

#include <stdio.h>
#include <windows.h>

#include "platform/win_adapter.h"

// Windows has no ioctl equivalent for this: the MAC is overridden via the
// per-adapter "NetworkAddress" registry value (the same mechanism used by
// NIC vendor utilities and tools like the Technitium MAC Changer), which
// only takes effect once the adapter is cycled.
#define NET_CLASS_GUID_KEY \
    "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"

static int set_network_address_value(const char *guid, const char *mac_hex) {
    HKEY class_key;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, NET_CLASS_GUID_KEY, 0, KEY_READ, &class_key) !=
        ERROR_SUCCESS)
        return -1;

    int result = -1;
    for (DWORD i = 0;; i++) {
        char subkey_name[16];
        DWORD name_len = sizeof(subkey_name);
        if (RegEnumKeyExA(class_key, i, subkey_name, &name_len, NULL, NULL, NULL, NULL) !=
            ERROR_SUCCESS)
            break;

        HKEY subkey;
        if (RegOpenKeyExA(class_key, subkey_name, 0, KEY_READ | KEY_WRITE, &subkey) !=
            ERROR_SUCCESS)
            continue;

        char instance_id[128];
        DWORD instance_len = sizeof(instance_id);
        DWORD type;
        if (RegQueryValueExA(subkey, "NetCfgInstanceId", NULL, &type, (BYTE *)instance_id,
                             &instance_len) == ERROR_SUCCESS &&
            _stricmp(instance_id, guid) == 0) {
            if (RegSetValueExA(subkey, "NetworkAddress", 0, REG_SZ, (const BYTE *)mac_hex,
                               (DWORD)strlen(mac_hex) + 1) == ERROR_SUCCESS)
                result = 0;
            RegCloseKey(subkey);
            break;
        }
        RegCloseKey(subkey);
    }

    RegCloseKey(class_key);
    return result;
}

static void restart_adapter(const char *ifname, int enable) {
    char sysdir[MAX_PATH];
    if (GetSystemDirectoryA(sysdir, sizeof(sysdir)) == 0)
        return;

    char app[MAX_PATH];
    snprintf(app, sizeof(app), "%s\\netsh.exe", sysdir);

    char cmdline[512];
    snprintf(cmdline, sizeof(cmdline), "netsh interface set interface \"%s\" admin=%s", ifname,
             enable ? "enabled" : "disabled");

    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(app, cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        return;

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

int mac_change_set(const char *ifname, const uint8_t mac[6]) {
    win_adapter_info_t info;
    if (win_adapter_find_by_name(ifname, &info) < 0)
        return -1;

    char mac_hex[13];
    snprintf(mac_hex, sizeof(mac_hex), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3],
             mac[4], mac[5]);

    if (set_network_address_value(info.guid, mac_hex) < 0)
        return -1;

    // The override is only picked up by the NIC driver on (re)start.
    restart_adapter(ifname, 0);
    restart_adapter(ifname, 1);
    return 0;
}

#else

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

#endif
