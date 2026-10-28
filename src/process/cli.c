#include "process/cli.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "network/device.h"
#include "process/cli_commands.h"

#define ALTERNATE_SCREEN_CODE "\033[?1049h"
#define EXIT_ALTERNATE_SCREEN_CODE "\033[?1049l"
#define TOP_LEFT_CODE "\033[H"
#define CLEAR_SCREEN_CODE "\033[2J"
#define COLOR_RESET "\033[0m"
#define COLOR_ROUTER "\033[36m"
#define COLOR_CLIENT "\033[32m"

#define INPUT_MAX 256
#define STATUS_MAX 256

static struct termios orig_termios;

static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf(EXIT_ALTERNATE_SCREEN_CODE);
    fflush(stdout);
}

static void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    printf(ALTERNATE_SCREEN_CODE);
    fflush(stdout);
}

static size_t get_visible_rows(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 6)
        return ws.ws_row - 6;
    return 20;
}

static const char *role_color(device_role_t role) {
    return role == DEVICE_ROLE_ROUTER ? COLOR_ROUTER : COLOR_CLIENT;
}

static const char *role_str(device_role_t role) {
    return role == DEVICE_ROLE_ROUTER ? "router" : "client";
}

static const char *state_str(device_state_t state) {
    switch (state) {
        case DEVICE_STATE_DEAD:
            return "dead";
        case DEVICE_STATE_ADDED:
            return "added";
        case DEVICE_STATE_ALIVE:
            return "alive";
        case DEVICE_STATE_DISCONNECTING:
            return "disconnecting";
    }
    return "unknown";
}

// Collects indices of every non-dead device into out (caller-sized to
// network->length), returns how many were written.
static size_t collect_visible(network_t *network, size_t *out) {
    size_t count = 0;
    pthread_mutex_lock(&network->lock);
    for (size_t i = 0; i < network->length; i++) {
        if (network->devices[i].state != DEVICE_STATE_DEAD) {
            out[count++] = i;
        }
    }
    pthread_mutex_unlock(&network->lock);
    return count;
}

static void draw(network_t *network, const size_t *visible, size_t visible_count, size_t offset,
                  size_t max_visible, const char *status_line, const char *input) {
    printf(TOP_LEFT_CODE CLEAR_SCREEN_CODE);

    printf("njamx - %zu known device(s)\n", visible_count);
    printf("%-5s %-16s %-18s %-8s %s\n", "#", "IP", "MAC", "ROLE", "STATE");

    pthread_mutex_lock(&network->lock);
    size_t shown = 0;
    for (size_t k = offset; k < visible_count && shown < max_visible; k++, shown++) {
        device_t *d = &network->devices[visible[k]];

        struct in_addr addr = {.s_addr = htonl(d->ip)};
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));

        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", d->mac[0], d->mac[1],
                 d->mac[2], d->mac[3], d->mac[4], d->mac[5]);

        printf("%s%-5zu %-16s %-18s %-8s %s%s\n", role_color(d->role), k + 1, ip_str, mac_str,
               role_str(d->role), state_str(d->state), COLOR_RESET);
    }
    pthread_mutex_unlock(&network->lock);

    printf("\n[Up/Down] scroll  commands: help, scan, quit\n");
    if (status_line[0] != '\0')
        printf("%s\n", status_line);
    printf("njamx> %s", input);
    fflush(stdout);
}

void cli_run(network_t *network, const network_socket_t *sock, const uint8_t src_mac[6],
             int timeout_sec) {
    size_t *visible = malloc(network->length * sizeof(size_t));
    if (!visible)
        return;

    enable_raw_mode();

    size_t max_visible = get_visible_rows();
    size_t offset = 0;
    size_t visible_count = collect_visible(network, visible);

    char input[INPUT_MAX] = {0};
    size_t input_len = 0;
    char status_line[STATUS_MAX] = {0};
    cli_context_t ctx = {
        .network = network, .sock = sock, .src_mac = src_mac, .timeout_sec = timeout_sec};

    draw(network, visible, visible_count, offset, max_visible, status_line, input);

    for (;;) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0)
            continue;

        bool should_quit = false;
        bool redraw = true;

        if (c == '\x03') {
            break;
        } else if (c == '\x1b') {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1) {
                if (seq[0] == '[' && seq[1] == 'B' && offset + max_visible < visible_count) {
                    offset++;
                } else if (seq[0] == '[' && seq[1] == 'A' && offset > 0) {
                    offset--;
                } else {
                    redraw = false;
                }
            } else {
                break;
            }
        } else if (c == '\n' || c == '\r') {
            input[input_len] = '\0';
            cli_run_command(input, &ctx, status_line, STATUS_MAX, &should_quit);
            input_len = 0;
            input[0] = '\0';
        } else if (c == 127 || c == '\b') {
            if (input_len > 0) {
                input_len--;
                input[input_len] = '\0';
            }
        } else if (c >= 32 && c <= 126 && input_len < INPUT_MAX - 1) {
            input[input_len++] = c;
            input[input_len] = '\0';
        } else {
            redraw = false;
        }

        if (should_quit)
            break;
        if (!redraw)
            continue;

        max_visible = get_visible_rows();
        visible_count = collect_visible(network, visible);
        if (offset > 0 && offset + max_visible > visible_count) {
            offset = visible_count > max_visible ? visible_count - max_visible : 0;
        }
        draw(network, visible, visible_count, offset, max_visible, status_line, input);
    }

    free(visible);
    disable_raw_mode();
}
