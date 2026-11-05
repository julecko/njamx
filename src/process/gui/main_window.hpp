#ifndef PROCESS_GUI_MAIN_WINDOW_HPP
#define PROCESS_GUI_MAIN_WINDOW_HPP

#include <vector>

#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>

#include "process/cli_commands.h"
#include "process/gui/device_table.hpp"
#include "process/gui/packet_view.hpp"
#include "process/runtime_options.h"

namespace njamx_gui {

// Graphical equivalent of cli_run(): drives cli_run_command() exactly
// like the terminal UI, just via buttons/inputs/a table instead of a
// typed command line.
class MainWindow : public Fl_Double_Window {
public:
    MainWindow(network_t *network, const network_socket_t *sock, runtime_options_t *opts,
              atomic_int *timeout_sec, atomic_bool *quit);

private:
    cli_context_t ctx_{};
    std::vector<size_t> visible_;
    atomic_bool *quit_;

    DeviceTable *device_table_;
    PacketView *packet_view_;
    Fl_Button *show_toggle_;
    Fl_Input *device_num_input_;
    Fl_Input *add_ip_input_;
    Fl_Input *timeout_input_;
    Fl_Box *status_box_;
    bool showing_packets_ = false;

    void run_command(const char *cmd);
    void refresh_current_view();
    void set_status(const char *s);

    static void scan_cb(Fl_Widget *w, void *data);
    static void jam_all_cb(Fl_Widget *w, void *data);
    static void jam_none_cb(Fl_Widget *w, void *data);
    static void armagedon_cb(Fl_Widget *w, void *data);
    static void selfjam_cb(Fl_Widget *w, void *data);
    static void random_mac_cb(Fl_Widget *w, void *data);
    static void change_mac_cb(Fl_Widget *w, void *data);
    static void toggle_show_cb(Fl_Widget *w, void *data);
    static void jam_by_num_cb(Fl_Widget *w, void *data);
    static void router_by_num_cb(Fl_Widget *w, void *data);
    static void remove_by_num_cb(Fl_Widget *w, void *data);
    static void set_timeout_cb(Fl_Widget *w, void *data);
    static void add_ip_cb(Fl_Widget *w, void *data);
    static void quit_cb(Fl_Widget *w, void *data);
    static void close_cb(Fl_Widget *w, void *data);
    static void tick_cb(void *data);
    void tick();
};

} // namespace njamx_gui

#endif
