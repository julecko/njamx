#include "process/gui/format.hpp"

#include <cstdio>

#include <FL/fl_draw.H>

namespace njamx_gui {

const char *role_str(device_role_t role) {
    return role == DEVICE_ROLE_ROUTER ? "router" : "client";
}

const char *state_str(device_state_t state) {
    switch (state) {
        case DEVICE_STATE_DEAD:
            return "dead";
        case DEVICE_STATE_ADDED:
            return "added";
        case DEVICE_STATE_ALIVE:
            return "alive";
        case DEVICE_STATE_DISCONNECTING:
            return "disconnecting";
    }
    return "unknown";
}

Fl_Color device_row_color(const device_t &d) {
    if (d.flags & DEVICE_FLAG_MY_DEVICE)
        return fl_rgb_color(184, 134, 11); // gold
    return d.role == DEVICE_ROLE_ROUTER ? fl_rgb_color(230, 126, 34) /* orange */
                                        : FL_FOREGROUND_COLOR;
}

std::string format_ip(uint32_t ip_host_order) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%u.%u.%u.%u", (ip_host_order >> 24) & 0xFF,
                  (ip_host_order >> 16) & 0xFF, (ip_host_order >> 8) & 0xFF,
                  ip_host_order & 0xFF);
    return buf;
}

std::string format_mac(const uint8_t mac[6]) {
    char buf[18];
    std::snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2],
                  mac[3], mac[4], mac[5]);
    return buf;
}

} // namespace njamx_gui
