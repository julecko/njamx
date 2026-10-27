#ifndef NETWORK_FORGERY_H
#define NETWORK_FORGERY_H

#include <stdint.h>

#include "device.h"
#include "network.h"

void network_forgery_mac(uint8_t mac[6]);
device_t network_forgery_device(network_t *network);

#endif
