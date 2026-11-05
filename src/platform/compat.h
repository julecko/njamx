#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

#include <stdint.h>

// Small cross-platform stand-ins for the handful of POSIX calls used
// outside the capture layer (sleep, monotonic time).

#ifdef _WIN32

// Exclude GDI/USER declarations so they don't collide with raylib's own
// symbols (Rectangle, CloseWindow, ShowCursor, ...) when both headers end
// up in the same translation unit.
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOUSER
#define NOUSER
#endif
#include <windows.h>

static inline void platform_sleep_sec(unsigned int sec) {
    Sleep(sec * 1000);
}

// Monotonic time in milliseconds.
static inline uint64_t platform_monotonic_ms(void) {
    return (uint64_t)GetTickCount64();
}

#else

#include <time.h>
#include <unistd.h>

static inline void platform_sleep_sec(unsigned int sec) {
    sleep(sec);
}

static inline uint64_t platform_monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

#endif

#endif
