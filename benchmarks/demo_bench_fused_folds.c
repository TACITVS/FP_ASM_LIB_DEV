// FP-ASM-DemoBench-Fused-Folds
// Tests the "Fused Folds" module (Module 2).
// *** Debugging v6: Re-enable sad_i64 check to find crash ***

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <windows.h>
#include <math.h> // Include for fabs, fmax
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

// Function to compare doubles with relative tolerance
static bool doubles_are_close(double a, double b, double rel_tol) {
    double diff = fabs(a - b);
    double scale = fmax(1.0, fmax(fabs(a), fabs(b)));
    return diff <= rel_tol * scale;
}


// -------------------- Baseline C (Fused Folds) -----------------
static int64_t c_fold_sumsq_i64(const int64_t* in, size_t n) {
    int64_t acc = 0;
    for (size_t i = 0; i < n; ++i) acc += in[i] * in[i];
    return acc;
}

static int64_t c_fold_dotp_i64(const int64_t* a, const int64_t* b, size_t n) {
    int64_t acc = 0;
    for (size_t i = 0; i < n; ++i) acc += a[i] * b[i];
    return acc;
}

static double c_fold_dotp_f64(const double* a, const double* b, size_t n) {
    double acc = 0.0;
    for (size_t i = 0; i < n; ++i) acc += a[i] * b[i];
    return acc;
}

static int64_t c_fold_sad_i64(const int64_t* a, const int64_t* b, size_t n) {
    int64_t acc = 0;
    for (size_t i = 0; i < n; ++i) {
        int64_t diff = a[i] - b[i];
        acc += (diff < 0) ? -diff : diff; // Equivalent to abs()
    }
    return acc;
}


#define N_DEFAULT 10000000
#define ITERS_DEFAULT 10

// -------------------- Main ---------------------------------
int main(int argc, char** argv) {
    size_t n = (argc > 1) ? (size_t)strtoull(argv[1], NULL, 10) : N_DEFAULT;
    int iters = (argc > 2) ? (int)strtol(argv[2], NULL, 10) : ITERS_DEFAULT;

    if (n == 0) n = N_DEFAULT;
    if (iters == 0) iters = ITERS_DEFAULT;

    printf("Benchmark: Fused Folds (Module 2)\n");
    printf("Array Size: n=%" PRIu64 " elements, Iterations: %d\n\n", (uint64_t)n, iters);

    int64_t* in_i64_a = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* in_i64_b = (int64_t*)xmalloc(n * sizeof(int64_t));
    double* in_f64_a = (double*)xmalloc(n * sizeof(double));
    double* in_f64_b = (double*)xmalloc(n * sizeof(double));

    for (size_t i = 0; i < n; ++i) {
        in_i64_a[i] = (int64_t)(i % 100) - 50;
        in_i64_b[i] = (int64_t)(i % 50) - 25;
        in_f64_a[i] = (double)(i % 100) - 50.0;
        in_f64_b[i] = (double)(i % 50) - 25.0;
    }

    volatile int64_t sink_i64 = 0;
    volatile double  sink_f64 = 0.0;

    // ================== CORRECTNESS CHECKS ==================
    bool ok = true;
    printf("Running Correctness Checks...\n");

    int64_t c_sumsq_i64 = c_fold_sumsq_i64(in_i64_a, n);
    int64_t asm_sumsq_i64 = fp_fold_sumsq_i64(in_i64_a, n);
    if(c_sumsq_i64 != asm_sumsq_i64) { ok = false; printf("[FAIL] sumsq_i64: C=%" PRId64 " ASM=%" PRId64 "\n", c_sumsq_i64, asm_sumsq_i64); }

    int64_t c_dotp_i64 = c_fold_dotp_i64(in_i64_a, in_i64_b, n);
    int64_t asm_dotp_i64 = fp_fold_dotp_i64(in_i64_a, in_i64_b, n);
    if(c_dotp_i64 != asm_dotp_i64) { ok = false; printf("[FAIL] dotp_i64: C=%" PRId64 " ASM=%" PRId64 "\n", c_dotp_i64, asm_dotp_i64); }

    double c_dotp_f64 = c_fold_dotp_f64(in_f64_a, in_f64_b, n);
    double asm_dotp_f64 = fp_fold_dotp_f64(in_f64_a, in_f64_b, n);
    double dotp_rel_tol = 1e-5; // Tolerance for parallel SIMD reduction (different summation order than sequential)
    if(!doubles_are_close(c_dotp_f64, asm_dotp_f64, dotp_rel_tol)) {
        ok = false; printf("[FAIL] dotp_f64: C=%.1f ASM=%.1f (Differs by more than %.1e rel_tol)\n",
                           c_dotp_f64, asm_dotp_f64, dotp_rel_tol);
    }

    // *** DEBUGGING: Re-enable sad_i64 check ***
    int64_t c_sad_i64 = c_fold_sad_i64(in_i64_a, in_i64_b, n);
    int64_t asm_sad_i64 = fp_fold_sad_i64(in_i64_a, in_i64_b, n);
    if(c_sad_i64 != asm_sad_i64) { ok = false; printf("[FAIL] sad_i64: C=%" PRId64 " ASM=%" PRId64 "\n", c_sad_i64, asm_sad_i64); }


    if (!ok) {
        printf("Correctness checks FAILED. Halting.\n");
        free(in_i64_a); free(in_i64_b); free(in_f64_a); free(in_f64_b);
        return EXIT_FAILURE;
    }
    printf("All checks passed.\n\n");

    // ================== BENCHMARKS ==================
    hi_timer_t t;
    double c_ms, asm_ms;

    // --- fp_fold_sumsq_i64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_i64 += c_fold_sumsq_i64(in_i64_a, n);
        c_ms += timer_ms_since(&t);
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_i64 += fp_fold_sumsq_i64(in_i64_a, n);
        asm_ms += timer_ms_since(&t);
    }
    printf("== fold_sumsq_i64 (Sum of Squares i64) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_fold_dotp_i64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_i64 += c_fold_dotp_i64(in_i64_a, in_i64_b, n);
        c_ms += timer_ms_since(&t);
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_i64 += fp_fold_dotp_i64(in_i64_a, in_i64_b, n);
        asm_ms += timer_ms_since(&t);
    }
    printf("\n== fold_dotp_i64 (Dot Product i64) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_fold_dotp_f64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_f64 += c_fold_dotp_f64(in_f64_a, in_f64_b, n);
        c_ms += timer_ms_since(&t);
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        sink_f64 += fp_fold_dotp_f64(in_f64_a, in_f64_b, n);
        asm_ms += timer_ms_since(&t);
    }
    printf("\n== fold_dotp_f64 (Dot Product f64) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_fold_sad_i64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
         t = timer_start();
         sink_i64 += c_fold_sad_i64(in_i64_a, in_i64_b, n);
         c_ms += timer_ms_since(&t);
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
         t = timer_start();
         sink_i64 += fp_fold_sad_i64(in_i64_a, in_i64_b, n);
         asm_ms += timer_ms_since(&t);
    }
    printf("\n== fold_sad_i64 (Sum of Abs Diffs i64) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));


    free(in_i64_a); free(in_i64_b); free(in_f64_a); free(in_f64_b);
    return EXIT_SUCCESS;
}

