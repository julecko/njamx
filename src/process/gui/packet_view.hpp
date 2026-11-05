#ifndef PROCESS_GUI_PACKET_VIEW_HPP
#define PROCESS_GUI_PACKET_VIEW_HPP

#include <FL/Fl_Browser.H>

namespace njamx_gui {

// Scrolling list of the most recently sent packets (same source as the
// terminal UI's "packets" view - see network/packet_log.h), newest last.
class PacketView : public Fl_Browser {
public:
    PacketView(int X, int Y, int W, int H);

    // Re-reads packet_log's ring buffer and refreshes the list.
    void refresh();
};

} // namespace njamx_gui

#endif
