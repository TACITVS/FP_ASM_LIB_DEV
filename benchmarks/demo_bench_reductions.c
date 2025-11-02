// FP-ASM-DemoBench-Reductions
// Tests the "Simple Folds" module.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <windows.h>
#include <math.h>   // For fabs and -INFINITY
#include "../include/fp_core.h"

// -------------------- Timer --------------------
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

// -------------------- Helpers ------------------------------
static void* xmalloc(size_t bytes) {
    void* p = malloc(bytes);
    if (!p) {
        fprintf(stderr, "Out of memory requesting %zu bytes\n", bytes);
        exit(EXIT_FAILURE);
    }
    return p;
}

// -------------------- Baseline C (Reductions) -----------------
static int64_t c_reduce_add_i64(const int64_t* in, size_t n) {
    int64_t acc = 0;
    for (size_t i = 0; i < n; ++i) acc += in[i];
    return acc;
}

static double c_reduce_add_f64(const double* in, size_t n) {
    double acc = 0.0;
    for (size_t i = 0; i < n; ++i) acc += in[i];
    return acc;
}

static int64_t c_reduce_max_i64(const int64_t* in, size_t n) {
    if (n == 0) return 0;
    int64_t max_val = in[0];
    for (size_t i = 1; i < n; ++i) {
        if (in[i] > max_val) max_val = in[i];
    }
    return max_val;
}

static double c_reduce_max_f64(const double* in, size_t n) {
    if (n == 0) return 0.0;
    double max_val = -INFINITY;
    // Handle case where all values are -Infinity
    if (n > 0) max_val = in[0];
    for (size_t i = 1; i < n; ++i) {
        if (in[i] > max_val) max_val = in[i];
    }
    return max_val;
}

#define N_DEFAULT 10000000
#define ITERS_DEFAULT 10

// -------------------- Main ---------------------------------
int main(int argc, char** argv) {
    size_t n = (argc > 1) ? (size_t)strtoull(argv[1], NULL, 10) : N_DEFAULT;
    int iters = (argc > 2) ? (int)strtol(argv[2], NULL, 10) : ITERS_DEFAULT;

    if (n == 0) n = N_DEFAULT;
    if (iters == 0) iters = ITERS_DEFAULT;

    printf("Benchmark: Simple Folds (Reductions)\n");
    printf("Array Size: n=%" PRIu64 " elements, Iterations: %d\n\n", (uint64_t)n, iters);

    // Allocate buffers
    int64_t* in_i64 = (int64_t*)xmalloc(n * sizeof(int64_t));
    double* in_f64 = (double*)xmalloc(n * sizeof(double));

    // Fill input
    for (size_t i = 0; i < n; ++i) {
        in_i64[i] = (int64_t)(i % 1000) - 500;
        in_f64[i] = (double)(i % 1000) - 500.0;
    }
    // Set a known max value
    if (n > 100) {
        in_i64[n / 2] = 10000;
        in_f64[n / 2] = 10000.0;
    }

    // A volatile sink to prevent dead code elimination
    volatile int64_t sink_i64 = 0;
    volatile double  sink_f64 = 0.0;

    // ================== CORRECTNESS CHECKS ==================
    bool ok = true;
    printf("Running Correctness Checks...\n");
    
    int64_t c_add_i64 = c_reduce_add_i64(in_i64, n);
    int64_t asm_add_i64 = fp_reduce_add_i64(in_i64, n);
    if(c_add_i64 != asm_add_i64) { ok = false; printf("[FAIL] add_i64: C=%" PRId64 " ASM=%" PRId64 "\n", c_add_i64, asm_add_i64); }

    double c_add_f64 = c_reduce_add_f64(in_f64, n);
    double asm_add_f64 = fp_reduce_add_f64(in_f64, n);
    if(fabs(c_add_f64 - asm_add_f64) > 1e-9) { ok = false; printf("[FAIL] add_f64: C=%.1f ASM=%.1f\n", c_add_f64, asm_add_f64); }

    int64_t c_max_i64 = c_reduce_max_i64(in_i64, n);
    int64_t asm_max_i64 = fp_reduce_max_i64(in_i64, n);
    if(c_max_i64 != asm_max_i64) { ok = false; printf("[FAIL] max_i64: C=%" PRId64 " ASM=%" PRId64 "\n", c_max_i64, asm_max_i64); }

    double c_max_f64 = c_reduce_max_f64(in_f64, n);
    double asm_max_f64 = fp_reduce_max_f64(in_f64, n);
    if(c_max_f64 != asm_max_f64) { ok = false; printf("[FAIL] max_f64: C=%.1f ASM=%.1f (Note: C uses -INF, ASM uses in[0])\n", c_max_f64, asm_max_f64); }

    if (!ok) {
        printf("Correctness checks FAILED. Halting.\n");
        free(in_i64); free(in_f64);
        return EXIT_FAILURE;
    }
    printf("All checks passed.\n\n");

    // ================== BENCHMARKS ==================
    hi_timer_t t;
    double c_ms, asm_ms;

    // --- fp_reduce_add_i64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_i64 += c_reduce_add_i64(in_i64, n);
        c_ms += timer_ms_since(&t);
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_i64 += fp_reduce_add_i64(in_i64, n);
        asm_ms += timer_ms_since(&t);
    }
    printf("== reduce_add_i64 (Sum) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_reduce_add_f64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_f64 += c_reduce_add_f64(in_f64, n);
        c_ms += timer_ms_since(&t);
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_f64 += fp_reduce_add_f64(in_f64, n);
        asm_ms += timer_ms_since(&t);
    }
    printf("\n== reduce_add_f64 (Sum) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_reduce_max_i64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_i64 += c_reduce_max_i64(in_i64, n);
        c_ms += timer_ms_since(&t);
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_i64 += fp_reduce_max_i64(in_i64, n);
        asm_ms += timer_ms_since(&t);
    }
    printf("\n== reduce_max_i64 (Max) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_reduce_max_f64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_f64 += c_reduce_max_f64(in_f64, n);
        c_ms += timer_ms_since(&t);
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_f64 += fp_reduce_max_f64(in_f64, n);
        asm_ms += timer_ms_since(&t);
    }
    printf("\n== reduce_max_f64 (Max) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    free(in_i64);
    free(in_f64);
    return EXIT_SUCCESS;
}
