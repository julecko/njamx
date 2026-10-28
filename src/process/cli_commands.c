#include "process/cli_commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <pthread.h>

#include "network/device.h"
#include "network/scanner.h"

#define VERB_MAX 64

typedef void (*cli_command_fn)(const char *args, cli_context_t *ctx, char *status_line,
                               size_t status_cap, bool *should_quit);

static void cmd_help(const char *args, cli_context_t *ctx, char *status_line, size_t status_cap,
                     bool *should_quit) {
    (void)args;
    (void)ctx;
    (void)should_quit;
    snprintf(status_line, status_cap, "Commands: help, scan, timeout, quit/exit/q");
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
        if (device->state != DEVICE_STATE_ADDED && device->role != DEVICE_ROLE_ROUTER) {
            device->state = DEVICE_STATE_DEAD;
        }
    }
    pthread_mutex_unlock(&ctx->network->lock);

    int found = scanner_scan(ctx->network, ctx->sock, ctx->src_mac, atomic_load(ctx->timeout_sec));
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
        if (device->role != DEVICE_ROLE_ROUTER) {
            device->jamming = true;
        }
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
        if (device->role != DEVICE_ROLE_ROUTER) {
            device->jamming = true;
        }
    }

    pthread_mutex_unlock(&ctx->network->lock);
}

typedef struct {
    const char *name;
    cli_command_fn fn;
} cli_command_entry_t;

static const cli_command_entry_t COMMANDS[] = {
    {"help", cmd_help},       {"scan", cmd_scan},     {"timeout", cmd_timeout},
    {"quit", cmd_quit},       {"exit", cmd_quit},     {"q", cmd_quit},
    {"jam", cmd_jam},         {"router", cmd_router}, {"add", cmd_add},
    {"remove", cmd_remove},   {"all", cmd_all},       {"none", cmd_none},
    {"armagedon", cmd_armagedon}};

#define COMMAND_COUNT (sizeof(COMMANDS) / sizeof(COMMANDS[0]))

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
