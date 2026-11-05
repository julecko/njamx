#include "process/gui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include "network/arp.h"
#include "network/device.h"
#include "network/packet_log.h"
#include "platform/thread_compat.h"
#include "process/cli.h"
#include "process/cli_commands.h"

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define WIN_W 960
#define WIN_H 640
#define ROW_H 20
#define TABLE_TOP 190
#define TABLE_BOTTOM (WIN_H - 20)

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

static Color ip_color(const device_t *d) {
    if (d->flags & DEVICE_FLAG_MY_DEVICE)
        return GOLD;
    return d->role == DEVICE_ROLE_ROUTER ? ORANGE : DARKGRAY;
}

// Runs one command string through the shared dispatcher, refreshes the
// visible-device snapshot (the command may have changed device states),
// and applies *should_quit to *quit so the GUI closes the same way the
// terminal UI's "quit" command does.
static void run_command(const char *cmd, cli_context_t *ctx, size_t *visible, char *status_line,
                        size_t status_cap, atomic_bool *quit) {
    bool should_quit = false;
    cli_run_command(cmd, ctx, status_line, status_cap, &should_quit);
    ctx->visible_count = cli_collect_visible(ctx->network, visible);
    if (should_quit)
        atomic_store(quit, true);
}

void gui_run(network_t *network, const network_socket_t *sock, runtime_options_t *opts,
            atomic_int *timeout_sec, atomic_bool *quit) {
    size_t *visible = malloc(network->length * sizeof(size_t));
    if (!visible)
        return;
    size_t visible_count = cli_collect_visible(network, visible);

    cli_context_t ctx = {.network = network,
                         .sock = sock,
                         .opts = opts,
                         .timeout_sec = timeout_sec,
                         .visible = visible,
                         .visible_count = visible_count};

    char status_line[256] = "Welcome to njamx. Pick a device number from the table below.";
    char device_num_buf[16] = "";
    char add_ip_buf[64] = "";
    char timeout_buf[16];
    snprintf(timeout_buf, sizeof(timeout_buf), "%d", atomic_load(timeout_sec));
    bool device_num_edit = false, add_ip_edit = false, timeout_edit = false;

    bool show_packets = false;
    int table_offset = 0;
    packet_log_entry_t packets[PACKET_LOG_CAPACITY];

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIN_W, WIN_H, "njamx");
    SetTargetFPS(30);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);

    while (!WindowShouldClose() && !atomic_load(quit)) {
        // Mouse wheel scrolls whichever list is currently shown.
        float wheel = GetMouseWheelMove();
        if (wheel != 0 && CheckCollisionPointRec(
                              GetMousePosition(),
                              (Rectangle){0, TABLE_TOP, WIN_W, TABLE_BOTTOM - TABLE_TOP})) {
            table_offset -= (int)wheel;
            if (table_offset < 0)
                table_offset = 0;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("njamx", 10, 8, 24, DARKGRAY);
        DrawRectangle(0, 36, WIN_W, 1, LIGHTGRAY);

        char cmd[96];

        // Row 1: whole-network actions.
        if (GuiButton((Rectangle){10, 45, 90, 28}, "Scan")) {
            run_command("scan", &ctx, visible, status_line, sizeof(status_line), quit);
        }
        if (GuiButton((Rectangle){105, 45, 90, 28}, "Jam all")) {
            run_command("all", &ctx, visible, status_line, sizeof(status_line), quit);
        }
        if (GuiButton((Rectangle){200, 45, 90, 28}, "Jam none")) {
            run_command("none", &ctx, visible, status_line, sizeof(status_line), quit);
        }
        if (GuiButton((Rectangle){295, 45, 100, 28}, "Armagedon")) {
            run_command("armagedon", &ctx, visible, status_line, sizeof(status_line), quit);
        }
        if (GuiButton((Rectangle){400, 45, 130, 28}, "Toggle self-jam")) {
            run_command("selfjam", &ctx, visible, status_line, sizeof(status_line), quit);
        }
        if (GuiButton((Rectangle){WIN_W - 100, 45, 90, 28}, "Quit")) {
            atomic_store(quit, true);
        }

        // Row 2: MAC options.
        if (GuiButton((Rectangle){10, 80, 150, 28}, "Toggle random MAC")) {
            run_command("randommac", &ctx, visible, status_line, sizeof(status_line), quit);
        }
        if (GuiButton((Rectangle){165, 80, 150, 28}, "Toggle change MAC")) {
            run_command("changemac", &ctx, visible, status_line, sizeof(status_line), quit);
        }
        if (GuiButton((Rectangle){320, 80, 150, 28}, show_packets ? "Show devices" : "Show packets")) {
            show_packets = !show_packets;
            table_offset = 0;
        }

        // Row 3: by-device-number actions.
        DrawText("Device #:", 10, 120, 14, DARKGRAY);
        if (GuiTextBox((Rectangle){85, 115, 50, 24}, device_num_buf, sizeof(device_num_buf),
                       device_num_edit))
            device_num_edit = !device_num_edit;
        if (GuiButton((Rectangle){145, 115, 70, 24}, "Jam")) {
            snprintf(cmd, sizeof(cmd), "jam %s", device_num_buf);
            run_command(cmd, &ctx, visible, status_line, sizeof(status_line), quit);
        }
        if (GuiButton((Rectangle){220, 115, 70, 24}, "Router")) {
            snprintf(cmd, sizeof(cmd), "router %s", device_num_buf);
            run_command(cmd, &ctx, visible, status_line, sizeof(status_line), quit);
        }
        if (GuiButton((Rectangle){295, 115, 70, 24}, "Remove")) {
            snprintf(cmd, sizeof(cmd), "remove %s", device_num_buf);
            run_command(cmd, &ctx, visible, status_line, sizeof(status_line), quit);
        }

        DrawText("Timeout (s):", 400, 120, 14, DARKGRAY);
        if (GuiTextBox((Rectangle){490, 115, 50, 24}, timeout_buf, sizeof(timeout_buf),
                       timeout_edit))
            timeout_edit = !timeout_edit;
        if (GuiButton((Rectangle){545, 115, 60, 24}, "Set")) {
            snprintf(cmd, sizeof(cmd), "timeout %s", timeout_buf);
            run_command(cmd, &ctx, visible, status_line, sizeof(status_line), quit);
        }

        // Row 4: add by IP.
        DrawText("Add IP:", 10, 155, 14, DARKGRAY);
        if (GuiTextBox((Rectangle){70, 150, 160, 24}, add_ip_buf, sizeof(add_ip_buf), add_ip_edit))
            add_ip_edit = !add_ip_edit;
        if (GuiButton((Rectangle){235, 150, 70, 24}, "Add")) {
            snprintf(cmd, sizeof(cmd), "add %s", add_ip_buf);
            run_command(cmd, &ctx, visible, status_line, sizeof(status_line), quit);
        }

        DrawRectangle(0, 180, WIN_W, 1, LIGHTGRAY);

        if (show_packets) {
            packet_log_entry_t snapshot[PACKET_LOG_CAPACITY];
            size_t n = packet_log_snapshot(snapshot, PACKET_LOG_CAPACITY);
            memcpy(packets, snapshot, sizeof(snapshot));

            int max_rows = (TABLE_BOTTOM - TABLE_TOP) / ROW_H;
            if (table_offset > (int)n - 1)
                table_offset = n > 0 ? (int)n - 1 : 0;

            DrawText(TextFormat("Last %zu packet(s) sent (scroll to see more)", n), 10,
                     TABLE_TOP - 24, 14, DARKGRAY);

            char line[256];
            int shown = 0;
            for (int i = table_offset; i < (int)n && shown < max_rows; i++, shown++) {
                arp_frame_describe(packets[i].data, packets[i].len, line, sizeof(line));
                DrawText(TextFormat("%3d. %s", i + 1, line), 10,
                         TABLE_TOP + shown * ROW_H, 14, BLACK);
            }
        } else {
            visible_count = cli_collect_visible(network, visible);
            ctx.visible_count = visible_count;

            DrawText(TextFormat("%zu known device(s)", visible_count), 10, TABLE_TOP - 24, 14,
                     DARKGRAY);
            DrawText("#", 10, TABLE_TOP, 14, GRAY);
            DrawText("IP", 50, TABLE_TOP, 14, GRAY);
            DrawText("MAC", 220, TABLE_TOP, 14, GRAY);
            DrawText("ROLE", 360, TABLE_TOP, 14, GRAY);
            DrawText("STATE", 430, TABLE_TOP, 14, GRAY);
            DrawText("JAMMING", 540, TABLE_TOP, 14, GRAY);

            int max_rows = (TABLE_BOTTOM - (TABLE_TOP + ROW_H)) / ROW_H;
            if (table_offset > (int)visible_count - 1)
                table_offset = visible_count > 0 ? (int)visible_count - 1 : 0;

            pthread_mutex_lock(&network->lock);
            int shown = 0;
            for (int k = table_offset; k < (int)visible_count && shown < max_rows; k++, shown++) {
                device_t *d = &network->devices[visible[k]];
                int y = TABLE_TOP + ROW_H + shown * ROW_H;

                struct in_addr addr = {.s_addr = htonl(d->ip)};
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));

                DrawText(TextFormat("%d", k + 1), 10, y, 14, BLACK);
                DrawText(ip_str, 50, y, 14, ip_color(d));
                DrawText(TextFormat("%02x:%02x:%02x:%02x:%02x:%02x", d->mac[0], d->mac[1],
                                    d->mac[2], d->mac[3], d->mac[4], d->mac[5]),
                         220, y, 14, BLACK);
                DrawText(role_str(d->role), 360, y, 14, BLACK);
                DrawText(state_str(d->state), 430, y, 14, BLACK);
                DrawText(d->jamming ? "true" : "false", 540, y, 14, d->jamming ? RED : BLACK);
            }
            pthread_mutex_unlock(&network->lock);
        }

        DrawRectangle(0, WIN_H - 26, WIN_W, 1, LIGHTGRAY);
        DrawText(status_line, 10, WIN_H - 20, 14, MAROON);

        EndDrawing();
    }

    CloseWindow();
    atomic_store(quit, true);
    free(visible);
}
