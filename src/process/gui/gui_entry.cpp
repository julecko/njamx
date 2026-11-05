#include "process/gui.h"

#include <cstdio>

#include <FL/Fl.H>

#include "process/gui/main_window.hpp"
#include "process/gui/setup_window.hpp"

// Definitions for the extern "C" declarations in process/gui.h - the only
// seam between njamx's C core and this FLTK-based (C++) GUI.

void gui_run(network_t *network, const network_socket_t *sock, runtime_options_t *opts,
            atomic_int *timeout_sec, atomic_bool *quit) {
    Fl::scheme("gtk+");
    njamx_gui::MainWindow win(network, sock, opts, timeout_sec, quit);
    win.show();
    Fl::run();
    atomic_store(quit, true);
}

bool gui_run_setup(char *ip_buf, size_t ip_cap, char *iface_buf, size_t iface_cap,
                   bool *mac_change, bool *real_mac, int *timeout_sec) {
    Fl::scheme("gtk+");
    njamx_gui::SetupWindow win(ip_buf, iface_buf, *mac_change, *real_mac, *timeout_sec);
    win.show();
    while (win.shown())
        Fl::wait();

    if (!win.started())
        return false;

    std::snprintf(ip_buf, ip_cap, "%s", win.ip());
    std::snprintf(iface_buf, iface_cap, "%s", win.iface());
    *mac_change = win.mac_change();
    *real_mac = win.real_mac();
    *timeout_sec = win.timeout_sec();
    return true;
}
