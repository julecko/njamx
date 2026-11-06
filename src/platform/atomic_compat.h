#ifndef PLATFORM_ATOMIC_COMPAT_H
#define PLATFORM_ATOMIC_COMPAT_H

// GCC's <stdatomic.h> relies on the C11 `_Atomic` keyword, which older
// libstdc++ versions (e.g. GCC 11) don't shim for C++ even when building
// with -std=c++23. Headers shared between the C core and the C++ GUI
// include this instead of <stdatomic.h> directly, so both languages get
// a usable atomic_bool/atomic_int plus atomic_load/atomic_store/atomic_init.

#ifdef __cplusplus

#include <atomic>

using atomic_bool = std::atomic<bool>;
using atomic_int = std::atomic<int>;

#else

#include <stdatomic.h>

#endif

#endif
