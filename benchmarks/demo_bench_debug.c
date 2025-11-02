/* =============================================================================
 * File:        demo_bench_debug.c
 * Version:     1.0
 * Project:     FP-ASM Core Library - Module 3 (Fused Maps)
 * Path:        /test/demo_bench_debug.c
 * 
 * Description: Debug version of the Fused Maps benchmark that adds detailed
 *              diagnostic output before each zip_add test. Program exits
 *              after zip_add tests complete (no performance benchmarks).
 *              All output is flushed immediately to catch exact crash point.
 * 
 * Purpose:     Diagnose why the full benchmark stops after offset_f64 test
 *              when n >= 4. This version prints timestamps and intermediate
 *              results to identify if the issue is in C code, ASM code, or
 *              the verification step.
 * 
 * Key Differences from Production:
 *   - Exits after correctness checks (no performance benchmarks)
 *   - DEBUG print statements before/after each zip_add call
 *   - Shows intermediate results (out[0] values)
 *   - All stdout is flushed immediately
 * 
 * Dependencies:
 *   - fp_core.h (function declarations)
 *   - fp_core_fused_maps.o (ASM fused maps)
 *   - fp_core_reductions.o (ASM reductions)
 *   - fp_core_fused_folds.o (ASM fused folds)
 * 
 * Build:       gcc -O3 demo_bench_debug.c fp_core_reductions.o \
 *                  fp_core_fused_folds.o fp_core_fused_maps.o \
 *                  -o bench_debug.exe
 * 
 * Usage:       bench_debug.exe [n] [iters]
 *              bench_debug.exe 100000000 1
 * 
 * Expected Behavior:
 *   If working: All checks pass, program exits cleanly
 *   If broken:  Output stops at specific DEBUG statement
 * 
 * Author:      FP-ASM Debug Tools
 * Date:        2025-01-XX
 * ============================================================================= */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <windows.h>
#include <math.h>
#include "../include/fp_core.h"

/* ========== Timer Functions ========== */
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

/* ========== Helper Functions ========== */
static void* xmalloc(size_t bytes) {
    void* p = malloc(bytes);
    if (!p) {
        fprintf(stderr, "Out of memory requesting %zu bytes\n", bytes);
        exit(EXIT_FAILURE);
    }
    return p;
}

static bool doubles_are_close(double a, double b, double rel_tol) {
    double diff = fabs(a - b);
    double scale = fmax(1.0, fmax(fabs(a), fabs(b)));
    return diff <= rel_tol * scale;
}

static bool check_arrays_i64(const char* name, const int64_t* expected, const int64_t* actual, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (expected[i] != actual[i]) {
            printf("[FAIL] %s: Mismatch at index %zu (Expected=%" PRId64 ", Actual=%" PRId64 ")\n",
                   name, i, expected[i], actual[i]);
            return false;
        }
    }
    printf("[PASS] %s\n", name);
    return true;
}

static bool check_arrays_f64(const char* name, const double* expected, const double* actual, size_t n, double rel_tol) {
    for (size_t i = 0; i < n; ++i) {
        if (!doubles_are_close(expected[i], actual[i], rel_tol)) {
            printf("[FAIL] %s: Mismatch at index %zu (Expected=%.1f, Actual=%.1f, Differs by > %.1e rel_tol)\n",
                   name, i, expected[i], actual[i], rel_tol);
            return false;
        }
    }
    printf("[PASS] %s\n", name);
    return true;
}

/* ========== C Reference Implementations ========== */
static void c_map_axpy_f64(const double* x, const double* y, double* out, size_t n, double c) {
    for (size_t i = 0; i < n; ++i) out[i] = c * x[i] + y[i];
}
static void c_map_axpy_i64(const int64_t* x, const int64_t* y, int64_t* out, size_t n, int64_t c) {
    for (size_t i = 0; i < n; ++i) out[i] = c * x[i] + y[i];
}
static void c_map_scale_i64(const int64_t* in, int64_t* out, size_t n, int64_t c) {
    for (size_t i = 0; i < n; ++i) out[i] = c * in[i];
}
static void c_map_scale_f64(const double* in, double* out, size_t n, double c) {
    for (size_t i = 0; i < n; ++i) out[i] = c * in[i];
}
static void c_map_offset_i64(const int64_t* in, int64_t* out, size_t n, int64_t c) {
    for (size_t i = 0; i < n; ++i) out[i] = in[i] + c;
}
static void c_map_offset_f64(const double* in, double* out, size_t n, double c) {
    for (size_t i = 0; i < n; ++i) out[i] = in[i] + c;
}
static void c_zip_add_i64(const int64_t* a, const int64_t* b, int64_t* out, size_t n) {
    for (size_t i = 0; i < n; ++i) out[i] = a[i] + b[i];
}
static void c_zip_add_f64(const double* a, const double* b, double* out, size_t n) {
    for (size_t i = 0; i < n; ++i) out[i] = a[i] + b[i];
}

#define N_DEFAULT 10000000
#define ITERS_DEFAULT 10

/* ========== Main ========== */
int main(int argc, char** argv) {
    size_t n = (argc > 1) ? (size_t)strtoull(argv[1], NULL, 10) : N_DEFAULT;
    int iters = (argc > 2) ? (int)strtol(argv[2], NULL, 10) : ITERS_DEFAULT;

    if (n == 0) n = N_DEFAULT;
    if (iters == 0) iters = ITERS_DEFAULT;

    printf("=============================================================================\n");
    printf(" FP-ASM Debug Benchmark - Fused Maps (Module 3)\n");
    printf(" Array Size: n=%" PRIu64 " elements, Iterations: %d\n", (uint64_t)n, iters);
    printf(" NOTE: Debug version - exits after correctness checks\n");
    printf("=============================================================================\n\n");

    /* Allocate buffers */
    printf("Allocating memory...\n");
    int64_t* in_i64_a = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* in_i64_b = (int64_t*)xmalloc(n * sizeof(int64_t));
    double* in_f64_a = (double*)xmalloc(n * sizeof(double));
    double* in_f64_b = (double*)xmalloc(n * sizeof(double));
    int64_t* out_i64_c = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* out_i64_asm = (int64_t*)xmalloc(n * sizeof(int64_t));
    double* out_f64_c = (double*)xmalloc(n * sizeof(double));
    double* out_f64_asm = (double*)xmalloc(n * sizeof(double));
    printf("  Memory allocated successfully.\n\n");

    /* Fill input */
    printf("Initializing input arrays...\n");
    for (size_t i = 0; i < n; ++i) {
        in_i64_a[i] = (int64_t)(i % 100) - 50;
        in_i64_b[i] = (int64_t)(i % 50) - 25;
        in_f64_a[i] = (double)(i % 100) - 50.0;
        in_f64_b[i] = (double)(i % 50) - 25.0;
    }
    const int64_t const_i64 = 3;
    const double const_f64 = 3.14;
    printf("  Input arrays initialized.\n\n");

    volatile int64_t sink_i64 = 0;
    volatile double  sink_f64 = 0.0;

    /* ========== CORRECTNESS CHECKS ========== */
    bool ok = true;
    printf("Running Correctness Checks...\n");
    double f64_tol = 1e-12;

    /* axpy */
    c_map_axpy_i64(in_i64_a, in_i64_b, out_i64_c, n, const_i64);
    fp_map_axpy_i64(in_i64_a, in_i64_b, out_i64_asm, n, const_i64);
    ok &= check_arrays_i64("axpy_i64", out_i64_c, out_i64_asm, n);

    c_map_axpy_f64(in_f64_a, in_f64_b, out_f64_c, n, const_f64);
    fp_map_axpy_f64(in_f64_a, in_f64_b, out_f64_asm, n, const_f64);
    ok &= check_arrays_f64("axpy_f64", out_f64_c, out_f64_asm, n, f64_tol);

    /* scale */
    c_map_scale_i64(in_i64_a, out_i64_c, n, const_i64);
    fp_map_scale_i64(in_i64_a, out_i64_asm, n, const_i64);
    ok &= check_arrays_i64("scale_i64", out_i64_c, out_i64_asm, n);

    c_map_scale_f64(in_f64_a, out_f64_c, n, const_f64);
    fp_map_scale_f64(in_f64_a, out_f64_asm, n, const_f64);
    ok &= check_arrays_f64("scale_f64", out_f64_c, out_f64_asm, n, f64_tol);

    /* offset */
    c_map_offset_i64(in_i64_a, out_i64_c, n, const_i64);
    fp_map_offset_i64(in_i64_a, out_i64_asm, n, const_i64);
    ok &= check_arrays_i64("offset_i64", out_i64_c, out_i64_asm, n);

    c_map_offset_f64(in_f64_a, out_f64_c, n, const_f64);
    fp_map_offset_f64(in_f64_a, out_f64_asm, n, const_f64);
    ok &= check_arrays_f64("offset_f64", out_f64_c, out_f64_asm, n, f64_tol);

    /* ========== ZIP_ADD WITH DEBUG OUTPUT ========== */
    printf("\n>>> DEBUG: Entering zip_add test section <<<\n");
    printf("  Memory check: in_i64_a[0]=%lld, in_i64_b[0]=%lld\n", 
           (long long)in_i64_a[0], (long long)in_i64_b[0]);
    fflush(stdout);

    printf("DEBUG: Calling c_zip_add_i64 (C reference)...\n"); fflush(stdout);
    c_zip_add_i64(in_i64_a, in_i64_b, out_i64_c, n);
    printf("  C version completed. Result: out_i64_c[0]=%lld\n", (long long)out_i64_c[0]); 
    fflush(stdout);
    
    printf("DEBUG: Calling fp_zip_add_i64 (ASM) with n=%zu...\n", n); fflush(stdout);
    fp_zip_add_i64(in_i64_a, in_i64_b, out_i64_asm, n);
    printf("  ASM version completed. Result: out_i64_asm[0]=%lld\n", (long long)out_i64_asm[0]); 
    fflush(stdout);
    
    printf("DEBUG: Checking zip_add_i64 arrays...\n"); fflush(stdout);
    ok &= check_arrays_i64("zip_add_i64", out_i64_c, out_i64_asm, n);

    printf("DEBUG: Calling c_zip_add_f64...\n"); fflush(stdout);
    c_zip_add_f64(in_f64_a, in_f64_b, out_f64_c, n);
    printf("DEBUG: Calling fp_zip_add_f64...\n"); fflush(stdout);
    fp_zip_add_f64(in_f64_a, in_f64_b, out_f64_asm, n);
    printf("DEBUG: Checking zip_add_f64 arrays...\n"); fflush(stdout);
    ok &= check_arrays_f64("zip_add_f64", out_f64_c, out_f64_asm, n, f64_tol);

    printf(">>> DEBUG: Exited zip_add test section <<<\n\n");

    if (!ok) {
        printf("Correctness checks FAILED. Halting.\n");
        free(in_i64_a); free(in_i64_b); free(in_f64_a); free(in_f64_b);
        free(out_i64_c); free(out_i64_asm); free(out_f64_c); free(out_f64_asm);
        return EXIT_FAILURE;
    }
    
    printf("=============================================================================\n");
    printf(" All checks passed!\n");
    printf(" Debug version complete - exiting without performance benchmarks.\n");
    printf("=============================================================================\n");
    
    free(in_i64_a); free(in_i64_b); free(in_f64_a); free(in_f64_b);
    free(out_i64_c); free(out_i64_asm); free(out_f64_c); free(out_f64_asm);
    return EXIT_SUCCESS;
}