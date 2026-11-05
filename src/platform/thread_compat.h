#ifndef PLATFORM_THREAD_COMPAT_H
#define PLATFORM_THREAD_COMPAT_H

// Minimal pthread subset (mutex + create/join) used by this project,
// backed by native Win32 primitives on Windows so the build doesn't need
// a pthreads-for-Windows package. POSIX platforms just get the real
// pthread.h.

#ifdef _WIN32

#include <stdlib.h>

// Exclude GDI/USER declarations (Rectangle, CloseWindow, ShowCursor, ...)
// so they don't collide with raylib's own symbols when both headers end
// up in the same translation unit (e.g. process/gui.c).
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOUSER
#define NOUSER
#endif
#include <windows.h>

typedef CRITICAL_SECTION pthread_mutex_t;
typedef HANDLE pthread_t;

static inline int pthread_mutex_init(pthread_mutex_t *m, void *attr) {
    (void)attr;
    InitializeCriticalSection(m);
    return 0;
}

static inline int pthread_mutex_lock(pthread_mutex_t *m) {
    EnterCriticalSection(m);
    return 0;
}

static inline int pthread_mutex_unlock(pthread_mutex_t *m) {
    LeaveCriticalSection(m);
    return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t *m) {
    DeleteCriticalSection(m);
    return 0;
}

typedef struct {
    void *(*fn)(void *);
    void *arg;
} pthread_compat_trampoline_args_t;

static inline DWORD WINAPI pthread_compat_trampoline(LPVOID param) {
    pthread_compat_trampoline_args_t *t = (pthread_compat_trampoline_args_t *)param;
    void *(*fn)(void *) = t->fn;
    void *arg = t->arg;
    free(t);
    fn(arg);
    return 0;
}

static inline int pthread_create(pthread_t *thread, void *attr, void *(*fn)(void *), void *arg) {
    (void)attr;
    pthread_compat_trampoline_args_t *t =
        (pthread_compat_trampoline_args_t *)malloc(sizeof(*t));
    if (!t)
        return -1;
    t->fn = fn;
    t->arg = arg;
    HANDLE h = CreateThread(NULL, 0, pthread_compat_trampoline, t, 0, NULL);
    if (!h) {
        free(t);
        return -1;
    }
    *thread = h;
    return 0;
}

static inline int pthread_join(pthread_t thread, void **ret) {
    (void)ret;
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return 0;
}

#else

#include <pthread.h>

#endif

#endif
