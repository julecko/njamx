#ifndef PROCESS_GUI_FORMAT_HPP
#define PROCESS_GUI_FORMAT_HPP

#include <cstdint>
#include <string>

#include <FL/Enumerations.H>

#include "network/device.h"

// Small presentation helpers shared by the device table and (indirectly)
// anything else in the GUI that needs to print a device - kept separate
// from device_table.hpp so they don't drag Fl_Table_Row into code that
// just wants to format a value.
namespace njamx_gui {

const char *role_str(device_role_t role);
const char *state_str(device_state_t state);

// Gold for njamx's own device, orange for routers, dark gray otherwise -
// matches the color coding the old GUI and the terminal UI both used.
Fl_Color device_row_color(const device_t &d);

std::string format_ip(uint32_t ip_host_order);
std::string format_mac(const uint8_t mac[6]);

} // namespace njamx_gui

#endif
