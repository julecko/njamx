#ifndef PROCESS_RUNTIME_OPTIONS_H
#define PROCESS_RUNTIME_OPTIONS_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "platform/thread_compat.h"

// Jamming/MAC settings that can be flipped live from the CLI ("randommac",
// "changemac" in cli_commands.c) as well as set once at startup from
// cli_args. Shared between the CLI's command dispatch (main thread), the
// scanning thread and the jammer thread.
typedef struct runtime_options {
    // true: jamming spoofs using program_mac below. false: a fresh
    // random decoy MAC is generated every jam cycle.
    atomic_bool use_real_mac;

    // njamx's current active MAC: the real interface MAC, or a forged
    // one if "changemac"/--mac-change is in effect. Guarded by
    // mac_lock since it's read by the jammer thread and written by the
    // CLI thread.
    uint8_t program_mac[6];

    // The interface's real MAC at startup. Immutable after
    // runtime_options_init, so safe to read without the lock.
    uint8_t original_mac[6];

    // Immutable after runtime_options_init.
    const char *interface;

    pthread_mutex_t mac_lock;
} runtime_options_t;

// Must be called once, single-threaded, before any thread can touch opts.
void runtime_options_init(runtime_options_t *opts, const char *interface,
                          const uint8_t original_mac[6], const uint8_t active_mac[6],
                          bool use_real_mac);

void runtime_options_set_program_mac(runtime_options_t *opts, const uint8_t mac[6]);
void runtime_options_get_program_mac(runtime_options_t *opts, uint8_t out[6]);

#endif
