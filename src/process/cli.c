#include "process/cli.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define read _read

#else

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <termios.h>

#endif

#include "network/device.h"
#include "process/cli_commands.h"

#define ALTERNATE_SCREEN_CODE "\033[?1049h"
#define EXIT_ALTERNATE_SCREEN_CODE "\033[?1049l"
#define TOP_LEFT_CODE "\033[H"
#define CLEAR_SCREEN_CODE "\033[2J"
#define COLOR_RESET "\033[0m"
#define COLOR_IP_CLIENT "\033[37m"
#define COLOR_IP_ROUTER "\033[38;5;208m"
#define COLOR_IP_SELF "\033[1;33m"
#define COLOR_STATE_ADDED "\033[34m"
#define COLOR_STATE_ALIVE "\033[32m"
#define COLOR_STATE_DISCONNECTING "\033[33m"
#define COLOR_JAMMING "\033[31m"

#define INPUT_MAX 256
#define STATUS_MAX 256

#ifdef _WIN32

static HANDLE win_stdin;
static HANDLE win_stdout;
static DWORD orig_in_mode;
static DWORD orig_out_mode;
static int orig_stdin_textmode;

static void disable_raw_mode(void) {
    SetConsoleMode(win_stdin, orig_in_mode);
    SetConsoleMode(win_stdout, orig_out_mode);
    _setmode(_fileno(stdin), orig_stdin_textmode);
    printf(EXIT_ALTERNATE_SCREEN_CODE);
    fflush(stdout);
}

static void enable_raw_mode(void) {
    win_stdin = GetStdHandle(STD_INPUT_HANDLE);
    win_stdout = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleMode(win_stdin, &orig_in_mode);
    GetConsoleMode(win_stdout, &orig_out_mode);
    atexit(disable_raw_mode);

    // stdin defaults to text mode, where the CRT buffers a lone '\r'
    // to see whether a '\n' follows before releasing it to _read() -
    // without this, pressing Enter once does nothing and a second
    // press is needed to flush the first.
    orig_stdin_textmode = _setmode(_fileno(stdin), _O_BINARY);

    // ENABLE_VIRTUAL_TERMINAL_INPUT makes the console feed back arrow
    // keys etc. as the same \x1b[... sequences used on POSIX, so the
    // rest of this file's input parsing works unmodified.
    DWORD raw_in = orig_in_mode;
    raw_in &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
    raw_in |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    SetConsoleMode(win_stdin, raw_in);

    SetConsoleMode(win_stdout, orig_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    printf(ALTERNATE_SCREEN_CODE);
    fflush(stdout);
}

size_t cli_get_visible_rows(void) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(win_stdout, &info)) {
        size_t rows = (size_t)(info.srWindow.Bottom - info.srWindow.Top + 1);
        if (rows > 6)
            return rows - 6;
    }
    return 20;
}

#else

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

size_t cli_get_visible_rows(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 6)
        return ws.ws_row - 6;
    return 20;
}

#endif

static const char *ip_color(const device_t *d) {
    if (d->flags & DEVICE_FLAG_MY_DEVICE)
        return COLOR_IP_SELF;
    return d->role == DEVICE_ROLE_ROUTER ? COLOR_IP_ROUTER : COLOR_IP_CLIENT;
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

static const char *state_color(device_state_t state) {
    switch (state) {
        case DEVICE_STATE_ADDED:
            return COLOR_STATE_ADDED;
        case DEVICE_STATE_ALIVE:
            return COLOR_STATE_ALIVE;
        case DEVICE_STATE_DISCONNECTING:
            return COLOR_STATE_DISCONNECTING;
        case DEVICE_STATE_DEAD:
            return COLOR_RESET;
    }
    return COLOR_RESET;
}

// Collects indices of every non-dead device into out (caller-sized to
// network->length), returns how many were written.
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

static void draw(network_t *network, const size_t *visible, size_t visible_count, size_t offset,
                 size_t max_visible, const char *status_line, const char *input) {
    printf(TOP_LEFT_CODE CLEAR_SCREEN_CODE);

    printf("njamx - %zu known device(s)\n", visible_count);
    printf("%-5s %-16s %-18s %-8s %-14s %s\n", "#", "IP", "MAC", "ROLE", "STATE", "JAMMING");

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

        printf("%-5zu %s%-16s%s %-18s %-8s %s%-14s%s %s%s%s\n", k + 1, ip_color(d), ip_str,
               COLOR_RESET, mac_str, role_str(d->role), state_color(d->state), state_str(d->state),
               COLOR_RESET, d->jamming ? COLOR_JAMMING : COLOR_RESET, d->jamming ? "true" : "false",
               COLOR_RESET);
    }
    pthread_mutex_unlock(&network->lock);

    printf("\n[Up/Down] scroll  commands: help, scan, timeout, selfjam, randommac, changemac, "
           "packets, quit\n");
    if (status_line[0] != '\0')
        printf("%s\n", status_line);
    printf("njamx> %s", input);
    fflush(stdout);
}

void cli_run(network_t *network, const network_socket_t *sock, runtime_options_t *opts,
             atomic_int *timeout_sec, atomic_bool *quit) {
    size_t *visible = malloc(network->length * sizeof(size_t));
    if (!visible)
        return;

    enable_raw_mode();

    size_t max_visible = cli_get_visible_rows();
    size_t offset = 0;
    size_t visible_count = cli_collect_visible(network, visible);

    char input[INPUT_MAX] = {0};
    size_t input_len = 0;
    char status_line[STATUS_MAX] = {0};
    cli_context_t ctx = {.network = network,
                         .sock = sock,
                         .opts = opts,
                         .timeout_sec = timeout_sec,
                         .visible = visible,
                         .visible_count = visible_count};

    draw(network, visible, visible_count, offset, max_visible, status_line, input);

    for (;;) {
        if (atomic_load(quit))
            break;

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
            ctx.visible_count = visible_count;
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

        max_visible = cli_get_visible_rows();
        visible_count = cli_collect_visible(network, visible);
        if (offset > 0 && offset + max_visible > visible_count) {
            offset = visible_count > max_visible ? visible_count - max_visible : 0;
        }
        draw(network, visible, visible_count, offset, max_visible, status_line, input);
    }

    atomic_store(quit, true);
    free(visible);
    disable_raw_mode();
}
