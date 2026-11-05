#include "process/gui/setup_window.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <FL/Fl_Return_Button.H>

#include "network/iface.h"

namespace njamx_gui {

namespace {
constexpr int kWinW = 520;
constexpr int kWinH = 280;
constexpr int kLabelX = 150;
} // namespace

SetupWindow::SetupWindow(const char *initial_ip, const char *initial_iface, bool mac_change,
                         bool real_mac, int timeout_sec)
    : Fl_Double_Window(kWinW, kWinH, "njamx setup") {
    begin();

    iface_choice_ = new Fl_Input_Choice(kLabelX, 20, 280, 26, "Interface:");
    network_iface_info_t ifaces[NETWORK_IFACE_LIST_MAX];
    int n = network_iface_list(ifaces);
    if (n < 0)
        n = 0;
    int initial_index = -1;
    for (int i = 0; i < n; i++) {
        iface_choice_->add(ifaces[i].name);
        if (initial_iface && initial_iface[0] && std::strcmp(ifaces[i].name, initial_iface) == 0)
            initial_index = i;
    }
    iface_choice_->callback(iface_cb, this);

    ip_input_ = new Fl_Input(kLabelX, 58, 280, 26, "IP / subnet:");
    ip_input_->value(initial_ip ? initial_ip : "");

    mac_change_cb_ =
        new Fl_Check_Button(20, 96, kWinW - 40, 24, "Temporarily change MAC while scanning");
    mac_change_cb_->value(mac_change ? 1 : 0);

    real_mac_cb_ = new Fl_Check_Button(20, 124, kWinW - 40, 24, "Use real MAC in forged replies");
    real_mac_cb_->value(real_mac ? 1 : 0);

    char timeout_buf[16];
    std::snprintf(timeout_buf, sizeof(timeout_buf), "%d", timeout_sec);
    timeout_input_ = new Fl_Input(kLabelX, 160, 80, 26, "Timeout (s):");
    timeout_input_->value(timeout_buf);

    status_box_ = new Fl_Box(20, 196, kWinW - 40, 24, "");
    status_box_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    status_box_->labelcolor(FL_DARK_RED);

    Fl_Return_Button *start = new Fl_Return_Button(kWinW - 130, 230, 110, 34, "Start");
    start->callback(start_cb, this);

    end();
    callback(close_cb, this);

    if (initial_iface && initial_iface[0] && initial_index < 0)
        iface_choice_->value(initial_iface); // not in the detected list; keep it anyway

    if (initial_index >= 0) {
        iface_choice_->value(initial_index);
        on_iface_changed(); // auto-guess the IP for the pre-selected interface too
    }
}

void SetupWindow::iface_cb(Fl_Widget *, void *data) {
    static_cast<SetupWindow *>(data)->on_iface_changed();
}

void SetupWindow::on_iface_changed() {
    const char *name = iface_choice_->value();
    if (!name || !name[0])
        return;

    char guess[64];
    if (network_iface_guess_ip(name, guess, sizeof(guess)) == 0) {
        ip_input_->value(guess);
        status_box_->copy_label("");
    } else {
        status_box_->copy_label(
            "Could not detect an IPv4 address for that interface - enter the IP/subnet manually.");
    }
}

void SetupWindow::start_cb(Fl_Widget *, void *data) {
    static_cast<SetupWindow *>(data)->on_start();
}

void SetupWindow::on_start() {
    const char *ip = ip_input_->value();
    const char *iface = iface_choice_->value();
    const char *timeout_s = timeout_input_->value();

    if (!ip || !ip[0]) {
        status_box_->copy_label("IP/subnet is required.");
        return;
    }
    if (!iface || !iface[0]) {
        status_box_->copy_label("Pick (or type) an interface.");
        return;
    }

    char *end;
    long t = timeout_s ? std::strtol(timeout_s, &end, 10) : 0;
    if (!timeout_s || !timeout_s[0] || end == timeout_s || *end != '\0' || t <= 0) {
        status_box_->copy_label("Invalid timeout.");
        return;
    }

    timeout_sec_ = (int)t;
    started_ = true;
    hide();
}

void SetupWindow::close_cb(Fl_Widget *, void *data) {
    // Default close (X) behavior: just hide, leaving started() false so
    // the caller knows to exit without ever starting the jammer thread.
    static_cast<SetupWindow *>(data)->hide();
}

} // namespace njamx_gui
