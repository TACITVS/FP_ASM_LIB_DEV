/*
 * fp_once.h — run-once initialisation for the runtime's two lazy caches
 * (CPU detection, dispatch table). Internal; not installed.
 *
 *   static long state;                       // 0 idle, 1 running, 2 done
 *   if (fp_once_begin(&state)) { ...init...; fp_once_end(&state); }
 *
 * fp_once_begin returns 1 to exactly one caller, which must initialise and
 * then call fp_once_end. Every other caller waits (the work takes
 * microseconds) and returns 0 once the result is published, so readers
 * never see a half-written cache. Acquire/release ordering throughout.
 * The initialiser must not re-enter the same once (it would wait forever).
 */
#ifndef FP_ONCE_H
#define FP_ONCE_H

#if defined(_MSC_VER) && !defined(__clang__)
#  include <intrin.h>
#  define FP_ONCE_LOAD(p)       _InterlockedCompareExchange((volatile long*)(p), 0, 0)
#  define FP_ONCE_CAS(p, o, n)  (_InterlockedCompareExchange((volatile long*)(p), (n), (o)) == (o))
#  define FP_ONCE_STORE(p, v)   _InterlockedExchange((volatile long*)(p), (v))
#  define FP_ONCE_PAUSE()       _mm_pause()
#else
#  define FP_ONCE_LOAD(p)       __atomic_load_n((p), __ATOMIC_ACQUIRE)
#  define FP_ONCE_CAS(p, o, n)  __extension__({ long e_ = (o); \
        __atomic_compare_exchange_n((p), &e_, (n), 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); })
#  define FP_ONCE_STORE(p, v)   __atomic_store_n((p), (v), __ATOMIC_RELEASE)
#  define FP_ONCE_PAUSE()       __builtin_ia32_pause()
#endif

static inline int fp_once_begin(long* state) {
    if (FP_ONCE_LOAD(state) == 2) return 0;
    if (FP_ONCE_CAS(state, 0, 1)) return 1;
    while (FP_ONCE_LOAD(state) != 2) FP_ONCE_PAUSE();
    return 0;
}

static inline void fp_once_end(long* state) { FP_ONCE_STORE(state, 2); }

#endif /* FP_ONCE_H */
