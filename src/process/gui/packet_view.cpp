#include "process/gui/packet_view.hpp"

#include <cstdio>

#include "network/arp.h"
#include "network/packet_log.h"

namespace njamx_gui {

PacketView::PacketView(int X, int Y, int W, int H) : Fl_Browser(X, Y, W, H) {
    format_char(0); // '@'-prefixed format codes would otherwise apply to MAC/IP text
    textfont(FL_COURIER);
    textsize(13);
}

void PacketView::refresh() {
    packet_log_entry_t entries[PACKET_LOG_CAPACITY];
    size_t n = packet_log_snapshot(entries, PACKET_LOG_CAPACITY);

    int top = topline();
    clear();

    char line[256];
    char numbered[288];
    for (size_t i = 0; i < n; i++) {
        arp_frame_describe(entries[i].data, entries[i].len, line, sizeof(line));
        std::snprintf(numbered, sizeof(numbered), "%3zu. %s", i + 1, line);
        add(numbered);
    }

    if (top > 0)
        topline(top);
}

} // namespace njamx_gui
