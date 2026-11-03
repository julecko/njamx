#include "tools/privileges.h"

#ifdef _WIN32

// Capture there goes through libpcap/Npcap, which handles its own driver
// privilege requirements, so there is nothing extra for njamx to check.
bool privileges_has_raw_capture(void) {
    return true;
}

#else

#include <unistd.h>

#include <linux/capability.h>
#include <sys/syscall.h>

// No glibc wrapper exists for capget(2); it's reached via syscall(2)
// directly, same as libcap does internally. Avoids a libcap-dev
// build dependency for this one check.
bool privileges_has_raw_capture(void) {
    if (geteuid() == 0)
        return true;

    struct __user_cap_header_struct header = {
        .version = _LINUX_CAPABILITY_VERSION_3,
        .pid = 0,
    };
    struct __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3] = {0};

    if (syscall(SYS_capget, &header, data) < 0)
        return false;

    unsigned index = CAP_TO_INDEX(CAP_NET_RAW);
    return (data[index].effective & CAP_TO_MASK(CAP_NET_RAW)) != 0;
}

#endif
