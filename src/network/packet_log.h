#ifndef NETWORK_PACKET_LOG_H
#define NETWORK_PACKET_LOG_H

#include <stddef.h>
#include <stdint.h>

#define PACKET_LOG_CAPACITY 32
#define PACKET_LOG_MAX_FRAME 128

typedef struct packet_log_entry {
    uint8_t data[PACKET_LOG_MAX_FRAME];
    size_t len;
} packet_log_entry_t;

// Must be called once, before any thread can call packet_log_record (the
// mutex it sets up has no static initializer on the Windows backend).
void packet_log_init(void);

// Records a copy of a just-sent frame (truncated to
// PACKET_LOG_MAX_FRAME bytes), overwriting the oldest entry once full.
// Thread-safe: called from both the scanning and jammer threads via
// network_socket_send().
void packet_log_record(const void *frame, size_t len);

// Copies up to out_cap of the most recently recorded entries into out,
// oldest first. Returns how many were copied.
size_t packet_log_snapshot(packet_log_entry_t *out, size_t out_cap);

#endif
