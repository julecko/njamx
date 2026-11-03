#ifdef _WIN32

#include "platform/win_adapter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock2.h>

#include <iphlpapi.h>

int win_adapter_find_by_name(const char *friendly_name, win_adapter_info_t *out) {
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

    int found = -1;
    for (IP_ADAPTER_ADDRESSES *a = addrs; a != NULL; a = a->Next) {
        char name[256];
        int n = WideCharToMultiByte(CP_UTF8, 0, a->FriendlyName, -1, name, sizeof(name), NULL,
                                    NULL);
        if (n <= 0)
            continue;

        if (_stricmp(name, friendly_name) != 0)
            continue;

        snprintf(out->guid, sizeof(out->guid), "%s", a->AdapterName);
        memset(out->mac, 0, sizeof(out->mac));
        if (a->PhysicalAddressLength == 6)
            memcpy(out->mac, a->PhysicalAddress, 6);

        found = 0;
        break;
    }

    free(addrs);
    return found;
}

#endif
