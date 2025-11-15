// FP-ASM-StreamCompaction-Bench
// Benchmarks the SIMD stream compaction primitives that power filter/partition.
//
// Highlights:
//  - Verifies correctness of both the simple and LUT-based SIMD filter kernels.
//  - Compares them against a straightforward scalar baseline.
//  - Uses QueryPerformanceCounter and volatile sinks to avoid DCE, matching
//    the style of the other demo benches.

#if !defined(_WIN32)
#define _POSIX_C_SOURCE 199309L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "../include/fp_core.h"

#if !defined(_WIN32)
#include <immintrin.h>
#endif

// The fully vectorized variant is currently only declared in assembly on
// Windows. On non-Windows platforms used by CI we provide portable
// fallbacks so the benchmark can still be compiled and exercised.
#if defined(_WIN32)
extern size_t fp_filter_gt_i64_simd(const int64_t* input, int64_t* output,
                                    size_t n, int64_t threshold);
#else
static const uint8_t g_popcount_lut[16] = {
    0, 1, 1, 2,
    1, 2, 2, 3,
    1, 2, 2, 3,
    2, 3, 3, 4
};

#if defined(__GNUC__)
#define ALIGN32 __attribute__((aligned(32)))
#else
#define ALIGN32 __declspec(align(32))
#endif

static const ALIGN32 int32_t g_perm_lut[16][8] = {
    {0, 1, 2, 3, 4, 5, 6, 7},
    {0, 1, 0, 1, 0, 1, 0, 1},
    {2, 3, 2, 3, 2, 3, 2, 3},
    {0, 1, 2, 3, 2, 3, 2, 3},
    {4, 5, 4, 5, 4, 5, 4, 5},
    {0, 1, 4, 5, 4, 5, 4, 5},
    {2, 3, 4, 5, 4, 5, 4, 5},
    {0, 1, 2, 3, 4, 5, 4, 5},
    {6, 7, 6, 7, 6, 7, 6, 7},
    {0, 1, 6, 7, 6, 7, 6, 7},
    {2, 3, 6, 7, 6, 7, 6, 7},
    {0, 1, 2, 3, 6, 7, 6, 7},
    {4, 5, 6, 7, 6, 7, 6, 7},
    {0, 1, 4, 5, 6, 7, 6, 7},
    {2, 3, 4, 5, 6, 7, 6, 7},
    {0, 1, 2, 3, 4, 5, 6, 7}
};

size_t fp_filter_gt_i64_simple(const int64_t* input, int64_t* output,
                               size_t n, int64_t threshold) {
    const __m256i thresh = _mm256_set1_epi64x(threshold);
    size_t out = 0;
    size_t i = 0;

    for (; i + 4 <= n; i += 4) {
        const __m256i v = _mm256_loadu_si256((const __m256i*)(input + i));
        const __m256i cmp = _mm256_cmpgt_epi64(v, thresh);
        const int mask = _mm256_movemask_pd(_mm256_castsi256_pd(cmp));

        if (mask & 0x1) {
            output[out++] = _mm256_extract_epi64(v, 0);
        }
        if (mask & 0x2) {
            output[out++] = _mm256_extract_epi64(v, 1);
        }
        if (mask & 0x4) {
            output[out++] = _mm256_extract_epi64(v, 2);
        }
        if (mask & 0x8) {
            output[out++] = _mm256_extract_epi64(v, 3);
        }
    }

    for (; i < n; ++i) {
        if (input[i] > threshold) {
            output[out++] = input[i];
        }
    }

    return out;
}

size_t fp_filter_gt_i64_simd(const int64_t* input, int64_t* output,
                             size_t n, int64_t threshold) {
    const __m256i thresh = _mm256_set1_epi64x(threshold);
    size_t out = 0;
    size_t i = 0;

    for (; i + 4 <= n; i += 4) {
        const __m256i v = _mm256_loadu_si256((const __m256i*)(input + i));
        const __m256i cmp = _mm256_cmpgt_epi64(v, thresh);
        const int mask = _mm256_movemask_pd(_mm256_castsi256_pd(cmp)) & 0xF;
        const int survivors = g_popcount_lut[mask];

        if (!survivors) {
            continue;
        }

        const __m256i perm = _mm256_load_si256((const __m256i*)g_perm_lut[mask]);
        const __m256i packed = _mm256_permutevar8x32_epi32(v, perm);

        if (survivors == 4) {
            _mm256_storeu_si256((__m256i*)(output + out), packed);
        } else if (survivors == 3) {
            _mm_storeu_si128((__m128i*)(output + out),
                             _mm256_castsi256_si128(packed));
            _mm_storel_epi64((__m128i*)(output + out + 2),
                             _mm256_extracti128_si256(packed, 1));
        } else if (survivors == 2) {
            _mm_storeu_si128((__m128i*)(output + out),
                             _mm256_castsi256_si128(packed));
        } else {  // survivors == 1
            _mm_storel_epi64((__m128i*)(output + out),
                             _mm256_castsi256_si128(packed));
        }

        out += (size_t)survivors;
    }

    for (; i < n; ++i) {
        if (input[i] > threshold) {
            output[out++] = input[i];
        }
    }

    return out;
}
#endif

// -------------------- High-resolution timer --------------------
#if defined(_WIN32)
typedef struct {
    LARGE_INTEGER freq;
    LARGE_INTEGER t0;
} hi_timer_t;

static hi_timer_t timer_start(void) {
    hi_timer_t t;
    QueryPerformanceFrequency(&t.freq);
    QueryPerformanceCounter(&t.t0);
    return t;
}

static double timer_ms_since(const hi_timer_t* t) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    const double dt = (double)(now.QuadPart - t->t0.QuadPart);
    return (1000.0 * dt) / (double)t->freq.QuadPart;
}
#else
typedef struct {
    struct timespec t0;
} hi_timer_t;

static hi_timer_t timer_start(void) {
    hi_timer_t t;
    clock_gettime(CLOCK_MONOTONIC, &t.t0);
    return t;
}

static double timer_ms_since(const hi_timer_t* t) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    const double sec = (double)(now.tv_sec - t->t0.tv_sec);
    const double nsec = (double)(now.tv_nsec - t->t0.tv_nsec);
    return (sec * 1000.0) + (nsec / 1.0e6);
}
#endif

// -------------------- Utilities --------------------
static void* xmalloc(size_t bytes) {
    void* p = malloc(bytes);
    if (!p) {
        fprintf(stderr, "Out of memory requesting %zu bytes\n", bytes);
        exit(EXIT_FAILURE);
    }
    return p;
}

static int64_t rand_range_i64(int64_t min, int64_t max) {
    const uint64_t span = (uint64_t)(max - min + 1);
    const uint64_t r0 = (uint64_t)(rand() & 0xFFFF);
    const uint64_t r1 = (uint64_t)(rand() & 0xFFFF);
    const uint64_t r2 = (uint64_t)(rand() & 0xFFFF);
    const uint64_t r = (r0 << 32) ^ (r1 << 16) ^ r2;
    return min + (int64_t)(r % span);
}

static void fill_input(int64_t* data, size_t n, unsigned seed,
                       int64_t min_v, int64_t max_v) {
    srand(seed);
    for (size_t i = 0; i < n; ++i) {
        data[i] = rand_range_i64(min_v, max_v);
    }
}

static void consume_results(volatile size_t* sink_count,
                            volatile int64_t* sink_value,
                            const int64_t* buf, size_t count) {
    *sink_count += count;
    if (count) {
        *sink_value += buf[count - 1];
    }
}

// -------------------- Scalar baseline --------------------
static size_t c_filter_gt_i64(const int64_t* input, int64_t* output,
                              size_t n, int64_t threshold) {
    size_t out_idx = 0;
    for (size_t i = 0; i < n; ++i) {
        if (input[i] > threshold) {
            output[out_idx++] = input[i];
        }
    }
    return out_idx;
}

static int64_t compute_checksum(const int64_t* data, size_t count) {
    int64_t acc = 0;
    for (size_t i = 0; i < count; ++i) {
        acc ^= data[i];
    }
    return acc;
}

static int compare_outputs(const int64_t* lhs, size_t lhs_count,
                           const int64_t* rhs, size_t rhs_count) {
    if (lhs_count != rhs_count) {
        return 0;
    }
    for (size_t i = 0; i < lhs_count; ++i) {
        if (lhs[i] != rhs[i]) {
            return 0;
        }
    }
    return 1;
}

// -------------------- Main harness --------------------
#define DEFAULT_N        1000000ULL
#define DEFAULT_THRESHOLD     12345
#define DEFAULT_ITERS          10
#define DEFAULT_SEED       20250217u

int main(int argc, char** argv) {
    size_t n = (argc > 1) ? (size_t)strtoull(argv[1], NULL, 10) : DEFAULT_N;
    int64_t threshold = (argc > 2) ? (int64_t)strtoll(argv[2], NULL, 10) : DEFAULT_THRESHOLD;
    int iters = (argc > 3) ? (int)strtol(argv[3], NULL, 10) : DEFAULT_ITERS;
    unsigned seed = (argc > 4) ? (unsigned)strtoul(argv[4], NULL, 10) : DEFAULT_SEED;

    if (n == 0) {
        fprintf(stderr, "Input size must be > 0\n");
        return EXIT_FAILURE;
    }
    if (iters <= 0) {
        iters = DEFAULT_ITERS;
    }

    printf("+============================================================+\n");
    printf("|   STREAM COMPACTION: Filter + Pack In One Pass             |\n");
    printf("|   SIMD vs Scalar (LUT gather + compact)                    |\n");
    printf("+============================================================+\n\n");

    printf("Compaction takes a predicate mask and packs the surviving\n");
    printf("elements into a dense list. This powers filter, partition,\n");
    printf("and database-style WHERE clauses.\n\n");

    printf("Configuration:\n");
    printf("  Elements:    %" PRIu64 "\n", (uint64_t)n);
    printf("  Threshold:   %" PRId64 " (keep > threshold)\n", threshold);
    printf("  Iterations:  %d timed runs per implementation\n", iters);
    printf("  RNG Seed:    %u\n\n", seed);

    int64_t* input = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* out_c = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* out_simple = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* out_simd = (int64_t*)xmalloc(n * sizeof(int64_t));

    fill_input(input, n, seed, threshold - 100000, threshold + 100000);

    // Warm-up calls to get code in cache and trigger JIT-like effects if any.
    (void)c_filter_gt_i64(input, out_c, n, threshold);
    (void)fp_filter_gt_i64_simple(input, out_simple, n, threshold);
    (void)fp_filter_gt_i64_simd(input, out_simd, n, threshold);

    printf("--- Correctness Check ---------------------------------------\n");
    size_t c_count = c_filter_gt_i64(input, out_c, n, threshold);
    size_t simple_count = fp_filter_gt_i64_simple(input, out_simple, n, threshold);
    size_t simd_count = fp_filter_gt_i64_simd(input, out_simd, n, threshold);

    int ok = 1;
    if (!compare_outputs(out_c, c_count, out_simple, simple_count)) {
        ok = 0;
        fprintf(stderr, "[FAIL] fp_filter_gt_i64_simple mismatch\n");
    }
    if (!compare_outputs(out_c, c_count, out_simd, simd_count)) {
        ok = 0;
        fprintf(stderr, "[FAIL] fp_filter_gt_i64_simd mismatch\n");
    }

    if (!ok) {
        fprintf(stderr, "Halting benchmark due to incorrect results.\n");
        free(out_simd); free(out_simple); free(out_c); free(input);
        return EXIT_FAILURE;
    }

    printf("PASS: All implementations match the scalar baseline.\n");
    printf("  Survivors:      %zu of %" PRIu64 " (%.2f%%)\n",
           c_count, (uint64_t)n, (100.0 * (double)c_count) / (double)n);
    printf("  Checksums:      C=%" PRId64 ", Simple=%" PRId64 ", SIMD=%" PRId64 "\n\n",
           compute_checksum(out_c, c_count),
           compute_checksum(out_simple, simple_count),
           compute_checksum(out_simd, simd_count));

    volatile size_t sink_count = 0;
    volatile int64_t sink_value = 0;

    hi_timer_t t;
    double c_total = 0.0;
    double simple_total = 0.0;
    double simd_total = 0.0;

    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        size_t count = c_filter_gt_i64(input, out_c, n, threshold);
        c_total += timer_ms_since(&t);
        consume_results(&sink_count, &sink_value, out_c, count);
    }

    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        size_t count = fp_filter_gt_i64_simple(input, out_simple, n, threshold);
        simple_total += timer_ms_since(&t);
        consume_results(&sink_count, &sink_value, out_simple, count);
    }

    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        size_t count = fp_filter_gt_i64_simd(input, out_simd, n, threshold);
        simd_total += timer_ms_since(&t);
        consume_results(&sink_count, &sink_value, out_simd, count);
    }

    const double avg_c = c_total / iters;
    const double avg_simple = simple_total / iters;
    const double avg_simd = simd_total / iters;

    printf("--- Performance --------------------------------------------\n");
    printf("Averaged over %d iterations:\n", iters);
    printf("+------------------------------------------------------------+\n");
    printf("| Implementation   |   Avg ms   |  Speedup vs C |\n");
    printf("+------------------------------------------------------------+\n");
    printf("| Scalar baseline  |  %8.3f |       1.00x   |\n", avg_c);
    printf("| fp_filter_simple |  %8.3f |       %.2fx   |\n",
           avg_simple, avg_c / avg_simple);
    printf("| fp_filter_simd   |  %8.3f |       %.2fx   |\n",
           avg_simd, avg_c / avg_simd);
    printf("+------------------------------------------------------------+\n\n");

    printf("Speedup summary: simple=%.2fx, simd=%.2fx vs scalar.\n",
           avg_c / avg_simple, avg_c / avg_simd);
    printf("Sink totals (ignore): count=%" PRIu64 " value=%" PRId64 "\n",
           (uint64_t)sink_count, sink_value);

    free(out_simd);
    free(out_simple);
    free(out_c);
    free(input);
    return EXIT_SUCCESS;
}
