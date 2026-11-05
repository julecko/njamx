#include "process/cli_commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define PACKET_VIEW_STDIN 0
#define packet_view_read _read
#else
#include <unistd.h>

#include <arpa/inet.h>
#define PACKET_VIEW_STDIN STDIN_FILENO
#define packet_view_read read
#endif

#include "network/arp.h"
#include "network/device.h"
#include "network/forgery.h"
#include "network/packet_log.h"
#include "network/scanner.h"
#include "platform/thread_compat.h"
#include "process/cli.h"
#include "tools/mac_change.h"

#define VERB_MAX 64
#define PACKET_VIEW_CLEAR "\033[H\033[2J"

typedef void (*cli_command_fn)(const char *args, cli_context_t *ctx, char *status_line,
                               size_t status_cap, bool *should_quit);

typedef struct {
    const char *name;
    const char *description;
} cli_command_help_t;

// Independent of the dispatch table below so cmd_help can look entries
// up without needing a forward declaration of it.
static const cli_command_help_t COMMAND_HELP[] = {
    {"help", "Show this list, or 'help <command>' for details on one command."},
    {"scan", "Reset non-router/non-added/non-you devices to dead, then ARP-probe the subnet."},
    {"timeout", "timeout <seconds> - how long 'scan' waits for ARP replies."},
    {"selfjam", "Toggle whether 'all'/'armagedon' also jam your own device."},
    {"randommac", "Toggle jamming's sender MAC between a random decoy and njamx's real/active MAC."},
    {"changemac", "Toggle forging njamx's own interface MAC on/off (needs CAP_NET_ADMIN/root)."},
    {"packets", "Show the last packets njamx sent; [Up/Down] scroll, any key refreshes, q returns."},
    {"jam", "jam <device-number> - toggle ARP-spoofing (jamming) a specific client."},
    {"router", "router <device-number> - toggle a device between client and router role."},
    {"add", "add <ip> - mark a dead device as present without waiting for a scan reply."},
    {"remove", "remove <device-number> - mark a device dead and stop jamming it."},
    {"all", "Jam every visible client (skips your own device unless 'selfjam' is on)."},
    {"none", "Stop jamming every device."},
    {"armagedon", "Jam every known client in the whole subnet (skips your own device unless "
                  "'selfjam' is on)."},
    {"quit", "Quit njamx (same as 'exit'/'q')."},
    {"exit", "Quit njamx (same as 'quit'/'q')."},
    {"q", "Quit njamx (same as 'quit'/'exit')."},
};

#define COMMAND_HELP_COUNT (sizeof(COMMAND_HELP) / sizeof(COMMAND_HELP[0]))

static void cmd_help(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                     bool *should_quit) {
    (void)ctx;
    (void)should_quit;

    if (args[0] == '\0') {
        snprintf(status_line, status_cap,
                 "Commands: help, scan, timeout, selfjam, randommac, changemac, packets, jam, "
                 "router, add, remove, all, none, armagedon, quit/exit/q ('help <command>' for "
                 "details)");
        return;
    }

    for (size_t i = 0; i < COMMAND_HELP_COUNT; i++) {
        if (strcmp(args, COMMAND_HELP[i].name) == 0) {
            snprintf(status_line, status_cap, "%s: %s", COMMAND_HELP[i].name,
                     COMMAND_HELP[i].description);
            return;
        }
    }
    snprintf(status_line, status_cap, "Unknown command: '%s' (try 'help')", args);
}

static void cmd_packets(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                        bool *should_quit) {
    (void)args;
    (void)ctx;
    (void)should_quit;

    packet_log_entry_t entries[PACKET_LOG_CAPACITY];
    char line[256];
    size_t offset = 0;

    for (;;) {
        size_t n = packet_log_snapshot(entries, PACKET_LOG_CAPACITY);
        size_t max_visible = cli_get_visible_rows();
        if (offset > 0 && offset + max_visible > n) {
            offset = n > max_visible ? n - max_visible : 0;
        }

        printf(PACKET_VIEW_CLEAR);
        printf("njamx - last %zu packet(s) sent ([Up/Down] scroll, any key refreshes, q returns)\n\n",
               n);
        if (n == 0) {
            printf("(nothing sent yet)\n");
        } else {
            size_t shown = 0;
            for (size_t i = offset; i < n && shown < max_visible; i++, shown++) {
                arp_frame_describe(entries[i].data, entries[i].len, line, sizeof(line));
                printf("%3zu. %s\n", i + 1, line);
            }
        }
        fflush(stdout);

        char c;
        if (packet_view_read(PACKET_VIEW_STDIN, &c, 1) <= 0)
            continue;

        if (c == 'q' || c == 'Q' || c == '\x03') {
            break;
        } else if (c == '\x1b') {
            char seq[2];
            if (packet_view_read(PACKET_VIEW_STDIN, &seq[0], 1) == 1 &&
                packet_view_read(PACKET_VIEW_STDIN, &seq[1], 1) == 1) {
                if (seq[0] == '[' && seq[1] == 'B' && offset + max_visible < n) {
                    offset++;
                } else if (seq[0] == '[' && seq[1] == 'A' && offset > 0) {
                    offset--;
                }
            } else {
                break;
            }
        }
    }

    status_line[0] = '\0';
}

// Whether "all"/"armagedon" also jam our own device (flagged
// DEVICE_FLAG_MY_DEVICE). Single-threaded (only touched from the CLI's
// command dispatch), so no lock/atomic needed.
static bool jam_self = false;

static void cmd_selfjam(const char *args, cli_context_t *ctx, char *status_line,
                        size_t status_cap, bool *should_quit) {
    (void)args;
    (void)ctx;
    (void)should_quit;

    jam_self = !jam_self;
    snprintf(status_line, status_cap, "Jamming own device with 'all'/'armagedon' is now %s",
             jam_self ? "enabled" : "disabled");
}

static void cmd_randommac(const char *args, cli_context_t *ctx, char *status_line,
                          size_t status_cap, bool *should_quit) {
    (void)args;
    (void)should_quit;

    bool now_real = !atomic_load(&ctx->opts->use_real_mac);
    atomic_store(&ctx->opts->use_real_mac, now_real);
    snprintf(status_line, status_cap, "Jamming sender MAC is now %s",
             now_real ? "njamx's real/active MAC" : "a random decoy MAC");
}

static void cmd_changemac(const char *args, cli_context_t *ctx, char *status_line,
                          size_t status_cap, bool *should_quit) {
    (void)args;
    (void)should_quit;

    runtime_options_t *opts = ctx->opts;

    uint8_t current[6];
    runtime_options_get_program_mac(opts, current);
    bool currently_forged = memcmp(current, opts->original_mac, 6) != 0;

    uint8_t new_mac[6];
    if (currently_forged) {
        memcpy(new_mac, opts->original_mac, 6);
    } else {
        network_forgery_mac(new_mac);
    }

    if (mac_change_set(opts->interface, new_mac) < 0) {
        snprintf(status_line, status_cap,
                 "Could not change MAC on '%s' (needs CAP_NET_ADMIN or root)", opts->interface);
        return;
    }

    runtime_options_set_program_mac(opts, new_mac);
    network_set_my_mac(ctx->network, new_mac);

    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", new_mac[0], new_mac[1],
             new_mac[2], new_mac[3], new_mac[4], new_mac[5]);
    snprintf(status_line, status_cap, "MAC on '%s' is now %s (%s)", opts->interface, mac_str,
             currently_forged ? "reverted to original" : "forged");
}

static void cmd_timeout(const char *args, cli_context_t *ctx, char *status_line,
                        size_t status_cap, bool *should_quit) {
    (void)should_quit;

    char *end;
    long value = strtol(args, &end, 10);
    if (end == args || *end != '\0' || value <= 0) {
        snprintf(status_line, status_cap, "Usage: timeout <seconds>");
        return;
    }

    atomic_store(ctx->timeout_sec, (int)value);
    snprintf(status_line, status_cap, "Scan timeout set to %ld second(s)", value);
}

static void cmd_scan(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                     bool *should_quit) {
    (void)args;
    (void)should_quit;

    printf("\nScanning...\n");
    fflush(stdout);

    pthread_mutex_lock(&ctx->network->lock);
    for (size_t i = 0; i < ctx->network->length; i++) {
        device_t *device = &ctx->network->devices[i];
        if (device->state != DEVICE_STATE_ADDED && device->role != DEVICE_ROLE_ROUTER &&
            !(device->flags & DEVICE_FLAG_MY_DEVICE)) {
            device->state = DEVICE_STATE_DEAD;
        }
    }
    pthread_mutex_unlock(&ctx->network->lock);

    uint8_t src_mac[6];
    runtime_options_get_program_mac(ctx->opts, src_mac);
    int found = scanner_scan(ctx->network, ctx->sock, src_mac, atomic_load(ctx->timeout_sec));
    if (found < 0)
        snprintf(status_line, status_cap, "Scan failed: couldn't send any ARP request");
    else
        snprintf(status_line, status_cap, "Scan complete: %d device(s) alive", found);
}

static void cmd_jam(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                    bool *should_quit) {
    (void)should_quit;

    char *end;
    long idx = strtol(args, &end, 10);
    if (end == args || *end != '\0') {
        snprintf(status_line, status_cap, "Usage: jam <device-number>");
        return;
    }
    if (idx < 1 || (size_t)idx > ctx->visible_count) {
        snprintf(status_line, status_cap, "Invalid device number: %ld", idx);
        return;
    }

    pthread_mutex_lock(&ctx->network->lock);
    device_t *device = &ctx->network->devices[ctx->visible[idx - 1]];

    if (device->role == DEVICE_ROLE_ROUTER) {
        snprintf(status_line, status_cap, "Invalid device number (only clients can be jammed) %ld",
                 idx);
        return;
    }
    device->jamming ^= true;
    pthread_mutex_unlock(&ctx->network->lock);
}

static void cmd_quit(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                     bool *should_quit) {
    (void)args;
    (void)ctx;
    (void)status_line;
    (void)status_cap;
    *should_quit = true;
}

static void cmd_router(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                       bool *should_quit) {
    (void)should_quit;

    char *end;
    long idx = strtol(args, &end, 10);
    if (end == args || *end != '\0') {
        snprintf(status_line, status_cap, "Usage: jam <device-number>");
        return;
    }
    if (idx < 1 || (size_t)idx > ctx->visible_count) {
        snprintf(status_line, status_cap, "Invalid device number: %ld", idx);
        return;
    }
    pthread_mutex_lock(&ctx->network->lock);
    device_t *device = &ctx->network->devices[ctx->visible[idx - 1]];
    if (device->jamming) {
        snprintf(status_line, status_cap,
                 "Invalid device number (cannot convert jammed device to router) %ld", idx);
        return;
    }

    if (device->role == DEVICE_ROLE_ROUTER) {
        device->role = DEVICE_ROLE_CLIENT;
    } else if (device->role == DEVICE_ROLE_CLIENT) {
        device->role = DEVICE_ROLE_ROUTER;
    };
    pthread_mutex_unlock(&ctx->network->lock);
}

static void cmd_add(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                    bool *should_quit) {
    (void)should_quit;

    if (args[0] == '\0') {
        snprintf(status_line, status_cap, "Usage: add <ip>");
        return;
    }

    char ip_str[INET_ADDRSTRLEN];
    size_t len = strcspn(args, " ");
    if (len == 0 || len >= sizeof(ip_str)) {
        snprintf(status_line, status_cap, "Invalid IP address");
        return;
    }
    memcpy(ip_str, args, len);
    ip_str[len] = '\0';

    struct in_addr addr;
    if (inet_pton(AF_INET, ip_str, &addr) != 1) {
        snprintf(status_line, status_cap, "Invalid IP address: '%s'", ip_str);
        return;
    }

    uint32_t ip = ntohl(addr.s_addr);

    pthread_mutex_lock(&ctx->network->lock);
    for (size_t i = 0; i < ctx->network->length; i++) {
        device_t *device = &ctx->network->devices[i];
        if (device->ip == ip && device->state == DEVICE_STATE_DEAD) {
            device->state = DEVICE_STATE_ADDED;
            break;
        }
    };
    pthread_mutex_unlock(&ctx->network->lock);
}

static void cmd_remove(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                       bool *should_quit) {
    (void)should_quit;

    char *end;
    long idx = strtol(args, &end, 10);
    if (end == args || *end != '\0') {
        snprintf(status_line, status_cap, "Usage: jam <device-number>");
        return;
    }
    if (idx < 1 || (size_t)idx > ctx->visible_count) {
        snprintf(status_line, status_cap, "Invalid device number: %ld", idx);
        return;
    }
    pthread_mutex_lock(&ctx->network->lock);
    device_t *device = &ctx->network->devices[ctx->visible[idx - 1]];
    device->state = DEVICE_STATE_DEAD;
    device->jamming = false;
    pthread_mutex_unlock(&ctx->network->lock);
}

static void cmd_all(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                    bool *should_quit) {
    (void)should_quit;

    pthread_mutex_lock(&ctx->network->lock);
    for (size_t i = 0; i < ctx->visible_count; i++) {
        device_t *device = &ctx->network->devices[ctx->visible[i]];
        if (device->role == DEVICE_ROLE_ROUTER)
            continue;
        if (!jam_self && (device->flags & DEVICE_FLAG_MY_DEVICE))
            continue;
        device->jamming = true;
    }

    pthread_mutex_unlock(&ctx->network->lock);
}

static void cmd_none(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                     bool *should_quit) {
    (void)should_quit;

    pthread_mutex_lock(&ctx->network->lock);
    for (size_t i = 0; i < ctx->network->length; i++) {
        device_t *device = &ctx->network->devices[i];
        if (device->role != DEVICE_ROLE_ROUTER) {
            device->jamming = false;
        }
    }

    pthread_mutex_unlock(&ctx->network->lock);
}

static void cmd_armagedon(const char *args, cli_context_t *ctx, char *status_line,
                          size_t status_cap, bool *should_quit) {
    (void)should_quit;

    pthread_mutex_lock(&ctx->network->lock);
    for (size_t i = 0; i < ctx->network->length; i++) {
        device_t *device = &ctx->network->devices[i];
        if (device->role == DEVICE_ROLE_ROUTER)
            continue;
        if (!jam_self && (device->flags & DEVICE_FLAG_MY_DEVICE))
            continue;
        device->jamming = true;
    }

    pthread_mutex_unlock(&ctx->network->lock);
}

typedef struct {
    const char *name;
    cli_command_fn fn;
} cli_command_entry_t;

static const cli_command_entry_t COMMANDS[] = {
    {"help", cmd_help},         {"scan", cmd_scan},         {"timeout", cmd_timeout},
    {"selfjam", cmd_selfjam},   {"randommac", cmd_randommac}, {"changemac", cmd_changemac},
    {"packets", cmd_packets},   {"quit", cmd_quit},         {"exit", cmd_quit},
    {"q", cmd_quit},            {"jam", cmd_jam},           {"router", cmd_router},
    {"add", cmd_add},           {"remove", cmd_remove},     {"all", cmd_all},
    {"none", cmd_none},         {"armagedon", cmd_armagedon}};

#define COMMAND_COUNT (sizeof(COMMANDS) / sizeof(COMMANDS[0]))

size_t cli_collect_visible(network_t *network, size_t *out) {
    size_t count = 0;
    pthread_mutex_lock(&network->lock);
    for (size_t i = 0; i < network->length; i++) {
        device_t *d = &network->devices[i];
        if (d->state != DEVICE_STATE_DEAD || d->jamming || (d->flags & DEVICE_FLAG_MY_DEVICE)) {
            out[count++] = i;
        }
    }
    pthread_mutex_unlock(&network->lock);
    return count;
}

void cli_run_command(const char *input, cli_context_t *ctx, char *status_line, size_t status_cap,
                     bool *should_quit) {
    status_line[0] = '\0';

    if (input[0] == '\0') {
        return;
    }

    char verb[VERB_MAX];
    const char *args;
    const char *space = strchr(input, ' ');
    if (space) {
        size_t verb_len = (size_t)(space - input);
        if (verb_len >= sizeof(verb))
            verb_len = sizeof(verb) - 1;
        memcpy(verb, input, verb_len);
        verb[verb_len] = '\0';

        args = space + 1;
        while (*args == ' ')
            args++;
    } else {
        strncpy(verb, input, sizeof(verb) - 1);
        verb[sizeof(verb) - 1] = '\0';
        args = "";
    }

    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if (strcmp(verb, COMMANDS[i].name) == 0) {
            COMMANDS[i].fn(args, ctx, status_line, status_cap, should_quit);
            return;
        }
    }
    snprintf(status_line, status_cap, "Unknown command: '%s' (try 'help')", verb);
}
