#include "network/packet_log.h"

#include <string.h>

#include "platform/thread_compat.h"

static packet_log_entry_t entries[PACKET_LOG_CAPACITY];
// Monotonically increasing count of every frame ever recorded; the ring
// slot for record number n is n % PACKET_LOG_CAPACITY.
static size_t total_recorded = 0;
static pthread_mutex_t lock;

void packet_log_init(void) {
    pthread_mutex_init(&lock, NULL);
}

void packet_log_record(const void *frame, size_t len) {
    size_t n = len < PACKET_LOG_MAX_FRAME ? len : PACKET_LOG_MAX_FRAME;

    pthread_mutex_lock(&lock);
    size_t slot = total_recorded % PACKET_LOG_CAPACITY;
    memcpy(entries[slot].data, frame, n);
    entries[slot].len = n;
    total_recorded++;
    pthread_mutex_unlock(&lock);
}

size_t packet_log_snapshot(packet_log_entry_t *out, size_t out_cap) {
    pthread_mutex_lock(&lock);
    size_t available = total_recorded < PACKET_LOG_CAPACITY ? total_recorded : PACKET_LOG_CAPACITY;
    size_t n = available < out_cap ? available : out_cap;
    for (size_t i = 0; i < n; i++) {
        size_t record_index = total_recorded - n + i;
        out[i] = entries[record_index % PACKET_LOG_CAPACITY];
    }
    pthread_mutex_unlock(&lock);
    return n;
}
