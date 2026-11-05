#ifndef PROCESS_GUI_DEVICE_TABLE_HPP
#define PROCESS_GUI_DEVICE_TABLE_HPP

#include <cstddef>
#include <functional>

#include <FL/Fl_Table_Row.H>

#include "network/network.h"

namespace njamx_gui {

// Scrollable, color-coded table of devices. Doesn't own any device state
// itself - set_devices() just rebinds it to the latest network_t +
// "visible" index snapshot (the same one cli_collect_visible() produces
// for the terminal UI) before each redraw.
class DeviceTable : public Fl_Table_Row {
public:
    DeviceTable(int X, int Y, int W, int H);

    void set_devices(network_t *network, const size_t *visible, size_t visible_count);

    // Fired with the clicked row's on-screen device number (1-based,
    // matching the "Device #" field and jam/router/remove commands).
    std::function<void(int)> on_row_selected;

protected:
    void draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) override;

private:
    network_t *network_ = nullptr;
    const size_t *visible_ = nullptr;
    size_t visible_count_ = 0;

    void draw_header(const char *s, int X, int Y, int W, int H);
    static void event_cb(Fl_Widget *w, void *data);
    void handle_event();
};

} // namespace njamx_gui

#endif
