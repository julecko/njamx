#include "process/runtime_options.h"

#include <string.h>

void runtime_options_init(runtime_options_t *opts, const char *interface,
                          const uint8_t original_mac[6], const uint8_t active_mac[6],
                          bool use_real_mac) {
    pthread_mutex_init(&opts->mac_lock, NULL);
    memcpy(opts->original_mac, original_mac, 6);
    memcpy(opts->program_mac, active_mac, 6);
    opts->interface = interface;
    atomic_init(&opts->use_real_mac, use_real_mac);
}

void runtime_options_set_program_mac(runtime_options_t *opts, const uint8_t mac[6]) {
    pthread_mutex_lock(&opts->mac_lock);
    memcpy(opts->program_mac, mac, 6);
    pthread_mutex_unlock(&opts->mac_lock);
}

void runtime_options_get_program_mac(runtime_options_t *opts, uint8_t out[6]) {
    pthread_mutex_lock(&opts->mac_lock);
    memcpy(out, opts->program_mac, 6);
    pthread_mutex_unlock(&opts->mac_lock);
}
