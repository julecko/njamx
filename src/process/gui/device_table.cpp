#include "process/gui/device_table.hpp"

#include <cstdio>

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include "platform/thread_compat.h"
#include "process/gui/format.hpp"

namespace njamx_gui {

namespace {
constexpr int kCols = 6;
const char *kHeaders[kCols] = {"#", "IP", "MAC", "ROLE", "STATE", "JAMMING"};
const int kColWidths[kCols] = {40, 130, 150, 80, 110, 90};
} // namespace

DeviceTable::DeviceTable(int X, int Y, int W, int H) : Fl_Table_Row(X, Y, W, H) {
    type(Fl_Table_Row::SELECT_SINGLE);
    row_header(0);
    row_height_all(22);
    row_resize(0);
    col_header(1);
    col_header_height(24);
    col_resize(1);
    cols(kCols);
    for (int c = 0; c < kCols; c++)
        col_width(c, kColWidths[c]);
    when(FL_WHEN_RELEASE);
    callback(event_cb, this);
    end();
}

void DeviceTable::set_devices(network_t *network, const size_t *visible, size_t visible_count) {
    network_ = network;
    visible_ = visible;
    visible_count_ = visible_count;
    rows((int)visible_count_);
    redraw();
}

void DeviceTable::draw_header(const char *s, int X, int Y, int W, int H) {
    fl_push_clip(X, Y, W, H);
    fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, FL_BACKGROUND_COLOR);
    fl_font(FL_HELVETICA_BOLD, 13);
    fl_color(FL_FOREGROUND_COLOR);
    fl_draw(s, X, Y, W, H, FL_ALIGN_CENTER);
    fl_pop_clip();
}

void DeviceTable::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) {
    switch (context) {
        case CONTEXT_STARTPAGE:
            fl_font(FL_HELVETICA, 13);
            return;
        case CONTEXT_COL_HEADER:
            draw_header(C >= 0 && C < kCols ? kHeaders[C] : "", X, Y, W, H);
            return;
        case CONTEXT_CELL: {
            if (!network_ || !visible_ || R < 0 || (size_t)R >= visible_count_)
                return;

            char text[64] = "";
            Fl_Color text_color = FL_FOREGROUND_COLOR;

            pthread_mutex_lock(&network_->lock);
            size_t idx = visible_[R];
            if (idx < network_->length) {
                const device_t &d = network_->devices[idx];
                text_color = device_row_color(d);
                switch (C) {
                    case 0:
                        std::snprintf(text, sizeof(text), "%d", R + 1);
                        break;
                    case 1:
                        std::snprintf(text, sizeof(text), "%s", format_ip(d.ip).c_str());
                        break;
                    case 2:
                        std::snprintf(text, sizeof(text), "%s", format_mac(d.mac).c_str());
                        break;
                    case 3:
                        std::snprintf(text, sizeof(text), "%s", role_str(d.role));
                        break;
                    case 4:
                        std::snprintf(text, sizeof(text), "%s", state_str(d.state));
                        break;
                    case 5:
                        std::snprintf(text, sizeof(text), "%s", d.jamming ? "true" : "false");
                        if (d.jamming)
                            text_color = FL_RED;
                        break;
                    default:
                        break;
                }
            }
            pthread_mutex_unlock(&network_->lock);

            fl_push_clip(X, Y, W, H);
            Fl_Color bg = row_selected(R) ? selection_color() : FL_BACKGROUND2_COLOR;
            fl_color(bg);
            fl_rectf(X, Y, W, H);
            fl_color(text_color);
            fl_draw(text, X + 4, Y, W - 8, H, FL_ALIGN_LEFT);
            fl_color(FL_LIGHT2);
            fl_rect(X, Y, W, H);
            fl_pop_clip();
            return;
        }
        default:
            return;
    }
}

void DeviceTable::event_cb(Fl_Widget *, void *data) {
    static_cast<DeviceTable *>(data)->handle_event();
}

void DeviceTable::handle_event() {
    if (callback_context() != CONTEXT_CELL)
        return;
    int row = callback_row();
    if (row >= 0 && (size_t)row < visible_count_ && on_row_selected)
        on_row_selected(row + 1);
}

} // namespace njamx_gui
