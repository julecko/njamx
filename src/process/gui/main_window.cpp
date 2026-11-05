#include "process/gui/main_window.hpp"

#include <cstdio>

#include <FL/Fl.H>
#include <FL/fl_draw.H>

namespace njamx_gui {

namespace {
constexpr int kWinW = 980;
constexpr int kWinH = 660;
constexpr double kTickInterval = 0.15;
} // namespace

MainWindow::MainWindow(network_t *network, const network_socket_t *sock, runtime_options_t *opts,
                       atomic_int *timeout_sec, atomic_bool *quit)
    : Fl_Double_Window(kWinW, kWinH, "njamx"), quit_(quit) {
    visible_.resize(network->length);
    size_t count = cli_collect_visible(network, visible_.data());

    ctx_.network = network;
    ctx_.sock = sock;
    ctx_.opts = opts;
    ctx_.timeout_sec = timeout_sec;
    ctx_.visible = visible_.data();
    ctx_.visible_count = count;

    begin();

    // Row 1: whole-network actions.
    (new Fl_Button(10, 10, 90, 28, "Scan"))->callback(scan_cb, this);
    (new Fl_Button(105, 10, 90, 28, "Jam all"))->callback(jam_all_cb, this);
    (new Fl_Button(200, 10, 90, 28, "Jam none"))->callback(jam_none_cb, this);
    (new Fl_Button(295, 10, 100, 28, "Armagedon"))->callback(armagedon_cb, this);
    (new Fl_Button(400, 10, 130, 28, "Toggle self-jam"))->callback(selfjam_cb, this);
    (new Fl_Button(kWinW - 100, 10, 90, 28, "Quit"))->callback(quit_cb, this);

    // Row 2: MAC options + view toggle.
    (new Fl_Button(10, 45, 150, 28, "Toggle random MAC"))->callback(random_mac_cb, this);
    (new Fl_Button(165, 45, 150, 28, "Toggle change MAC"))->callback(change_mac_cb, this);
    show_toggle_ = new Fl_Button(320, 45, 150, 28, "Show packets");
    show_toggle_->callback(toggle_show_cb, this);

    // Row 3: by-device-number actions.
    device_num_input_ = new Fl_Input(85, 80, 50, 24, "Device #:");
    (new Fl_Button(145, 80, 70, 24, "Jam"))->callback(jam_by_num_cb, this);
    (new Fl_Button(220, 80, 70, 24, "Router"))->callback(router_by_num_cb, this);
    (new Fl_Button(295, 80, 70, 24, "Remove"))->callback(remove_by_num_cb, this);

    char timeout_buf[16];
    std::snprintf(timeout_buf, sizeof(timeout_buf), "%d", atomic_load(timeout_sec));
    timeout_input_ = new Fl_Input(490, 80, 50, 24, "Timeout (s):");
    timeout_input_->value(timeout_buf);
    (new Fl_Button(545, 80, 60, 24, "Set"))->callback(set_timeout_cb, this);

    // Row 4: add by IP.
    add_ip_input_ = new Fl_Input(70, 114, 160, 24, "Add IP:");
    (new Fl_Button(235, 114, 70, 24, "Add"))->callback(add_ip_cb, this);

    const int table_top = 150, table_bottom = kWinH - 30;
    device_table_ = new DeviceTable(10, table_top, kWinW - 20, table_bottom - table_top);
    device_table_->on_row_selected = [this](int row) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d", row);
        device_num_input_->value(buf);
    };

    packet_view_ = new PacketView(10, table_top, kWinW - 20, table_bottom - table_top);
    packet_view_->hide();

    status_box_ = new Fl_Box(10, kWinH - 24, kWinW - 20, 20, "");
    status_box_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    status_box_->labelcolor(fl_rgb_color(128, 0, 0));
    set_status("Welcome to njamx. Click a device row, or pick a number, then act on it.");

    end();
    resizable(device_table_);
    callback(close_cb, this);

    device_table_->set_devices(ctx_.network, ctx_.visible, ctx_.visible_count);
    Fl::add_timeout(kTickInterval, tick_cb, this);
}

void MainWindow::set_status(const char *s) { status_box_->copy_label(s); }

void MainWindow::run_command(const char *cmd) {
    char status_line[256];
    bool should_quit = false;
    cli_run_command(cmd, &ctx_, status_line, sizeof(status_line), &should_quit);
    set_status(status_line);
    refresh_current_view();
    if (should_quit) {
        atomic_store(quit_, true);
        hide();
    }
}

void MainWindow::refresh_current_view() {
    ctx_.visible_count = cli_collect_visible(ctx_.network, visible_.data());
    if (showing_packets_)
        packet_view_->refresh();
    else
        device_table_->set_devices(ctx_.network, ctx_.visible, ctx_.visible_count);
}

void MainWindow::scan_cb(Fl_Widget *, void *data) {
    static_cast<MainWindow *>(data)->run_command("scan");
}
void MainWindow::jam_all_cb(Fl_Widget *, void *data) {
    static_cast<MainWindow *>(data)->run_command("all");
}
void MainWindow::jam_none_cb(Fl_Widget *, void *data) {
    static_cast<MainWindow *>(data)->run_command("none");
}
void MainWindow::armagedon_cb(Fl_Widget *, void *data) {
    static_cast<MainWindow *>(data)->run_command("armagedon");
}
void MainWindow::selfjam_cb(Fl_Widget *, void *data) {
    static_cast<MainWindow *>(data)->run_command("selfjam");
}
void MainWindow::random_mac_cb(Fl_Widget *, void *data) {
    static_cast<MainWindow *>(data)->run_command("randommac");
}
void MainWindow::change_mac_cb(Fl_Widget *, void *data) {
    static_cast<MainWindow *>(data)->run_command("changemac");
}

void MainWindow::toggle_show_cb(Fl_Widget *, void *data) {
    MainWindow *self = static_cast<MainWindow *>(data);
    self->showing_packets_ = !self->showing_packets_;
    if (self->showing_packets_) {
        self->device_table_->hide();
        self->packet_view_->show();
        self->show_toggle_->label("Show devices");
    } else {
        self->packet_view_->hide();
        self->device_table_->show();
        self->show_toggle_->label("Show packets");
    }
    self->refresh_current_view();
}

void MainWindow::jam_by_num_cb(Fl_Widget *, void *data) {
    MainWindow *self = static_cast<MainWindow *>(data);
    char cmd[96];
    std::snprintf(cmd, sizeof(cmd), "jam %s", self->device_num_input_->value());
    self->run_command(cmd);
}
void MainWindow::router_by_num_cb(Fl_Widget *, void *data) {
    MainWindow *self = static_cast<MainWindow *>(data);
    char cmd[96];
    std::snprintf(cmd, sizeof(cmd), "router %s", self->device_num_input_->value());
    self->run_command(cmd);
}
void MainWindow::remove_by_num_cb(Fl_Widget *, void *data) {
    MainWindow *self = static_cast<MainWindow *>(data);
    char cmd[96];
    std::snprintf(cmd, sizeof(cmd), "remove %s", self->device_num_input_->value());
    self->run_command(cmd);
}
void MainWindow::set_timeout_cb(Fl_Widget *, void *data) {
    MainWindow *self = static_cast<MainWindow *>(data);
    char cmd[96];
    std::snprintf(cmd, sizeof(cmd), "timeout %s", self->timeout_input_->value());
    self->run_command(cmd);
}
void MainWindow::add_ip_cb(Fl_Widget *, void *data) {
    MainWindow *self = static_cast<MainWindow *>(data);
    char cmd[96];
    std::snprintf(cmd, sizeof(cmd), "add %s", self->add_ip_input_->value());
    self->run_command(cmd);
}

void MainWindow::quit_cb(Fl_Widget *, void *data) {
    MainWindow *self = static_cast<MainWindow *>(data);
    atomic_store(self->quit_, true);
    self->hide();
}

void MainWindow::close_cb(Fl_Widget *, void *data) { quit_cb(nullptr, data); }

void MainWindow::tick_cb(void *data) { static_cast<MainWindow *>(data)->tick(); }

void MainWindow::tick() {
    refresh_current_view();
    if (atomic_load(quit_)) {
        hide();
        return;
    }
    Fl::repeat_timeout(kTickInterval, tick_cb, this);
}

} // namespace njamx_gui
