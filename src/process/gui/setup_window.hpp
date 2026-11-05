#ifndef PROCESS_GUI_SETUP_WINDOW_HPP
#define PROCESS_GUI_SETUP_WINDOW_HPP

#include <FL/Fl_Box.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Input_Choice.H>

namespace njamx_gui {

// Pre-flight window shown when njamx is launched without the IP/interface
// it needs: lets the user pick (or type) an interface - which
// auto-fills the IP/subnet field via network_iface_guess_ip() - plus the
// mac-change/real-mac/timeout options normally only settable from argv.
class SetupWindow : public Fl_Double_Window {
public:
    SetupWindow(const char *initial_ip, const char *initial_iface, bool mac_change,
               bool real_mac, int timeout_sec);

    bool started() const { return started_; }

    const char *ip() const { return ip_input_->value(); }
    const char *iface() const { return iface_choice_->value(); }
    bool mac_change() const { return mac_change_cb_->value() != 0; }
    bool real_mac() const { return real_mac_cb_->value() != 0; }
    int timeout_sec() const { return timeout_sec_; }

private:
    Fl_Input_Choice *iface_choice_;
    Fl_Input *ip_input_;
    Fl_Check_Button *mac_change_cb_;
    Fl_Check_Button *real_mac_cb_;
    Fl_Input *timeout_input_;
    Fl_Box *status_box_;

    bool started_ = false;
    int timeout_sec_ = 0;

    static void iface_cb(Fl_Widget *w, void *data);
    static void start_cb(Fl_Widget *w, void *data);
    static void close_cb(Fl_Widget *w, void *data);

    void on_iface_changed();
    void on_start();
};

} // namespace njamx_gui

#endif
