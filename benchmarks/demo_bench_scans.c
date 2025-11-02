// FP-ASM-DemoBench-Scans
// Tests the "Scans" module (Module 5) - Prefix Sum operations.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <windows.h>
#include <math.h>
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

// Function to compare arrays
static bool arrays_equal_i64(const int64_t* a, const int64_t* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

static bool arrays_close_f64(const double* a, const double* b, size_t n, double rel_tol) {
    for (size_t i = 0; i < n; i++) {
        if (!doubles_are_close(a[i], b[i], rel_tol)) {
            printf("  Mismatch at [%zu]: expected %.15f, got %.15f\n", i, a[i], b[i]);
            return false;
        }
    }
    return true;
}

// -------------------- C Reference Implementations --------------------
static void c_scan_add_i64(const int64_t* in, int64_t* out, size_t n) {
    int64_t acc = 0;
    for (size_t i = 0; i < n; i++) {
        acc += in[i];
        out[i] = acc;
    }
}

static void c_scan_add_f64(const double* in, double* out, size_t n) {
    double acc = 0.0;
    for (size_t i = 0; i < n; i++) {
        acc += in[i];
        out[i] = acc;
    }
}

// -------------------- Main --------------------
int main(int argc, char** argv) {
    size_t n = 10000000;  // 10M elements default
    int iterations = 10;

    if (argc >= 2) n = (size_t)atoll(argv[1]);
    if (argc >= 3) iterations = atoi(argv[2]);

    printf("Benchmark: Scans (Module 5)\n");
    printf("Array Size: n=%zu elements, Iterations: %d\n\n", n, iterations);

    // Allocate arrays
    int64_t* in_i64 = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* out_i64_c = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* out_i64_asm = (int64_t*)xmalloc(n * sizeof(int64_t));

    double* in_f64 = (double*)xmalloc(n * sizeof(double));
    double* out_f64_c = (double*)xmalloc(n * sizeof(double));
    double* out_f64_asm = (double*)xmalloc(n * sizeof(double));

    // Initialize input arrays with simple pattern
    for (size_t i = 0; i < n; i++) {
        in_i64[i] = (int64_t)(i % 100) - 50;  // Values from -50 to 49
        in_f64[i] = (double)(i % 100) - 50.0;
    }

    // ========================================
    // Correctness Checks
    // ========================================
    printf("Running Correctness Checks...\n");

    // --- fp_scan_add_i64 ---
    c_scan_add_i64(in_i64, out_i64_c, n);
    fp_scan_add_i64(in_i64, out_i64_asm, n);
    if (arrays_equal_i64(out_i64_c, out_i64_asm, n)) {
        printf("[PASS] scan_add_i64\n");
    } else {
        printf("[FAIL] scan_add_i64: Results differ!\n");
        return 1;
    }

    // --- fp_scan_add_f64 ---
    c_scan_add_f64(in_f64, out_f64_c, n);
    fp_scan_add_f64(in_f64, out_f64_asm, n);
    if (arrays_close_f64(out_f64_c, out_f64_asm, n, 1e-9)) {
        printf("[PASS] scan_add_f64\n");
    } else {
        printf("[FAIL] scan_add_f64: Results differ!\n");
        return 1;
    }

    printf("\nAll checks passed. Proceeding to benchmarks...\n\n");

    // ========================================
    // Benchmarks
    // ========================================

    volatile int64_t sink_i64 = 0;
    volatile double sink_f64 = 0.0;

    // --- Benchmark: fp_scan_add_i64 ---
    printf("== scan_add_i64 ==\n");

    hi_timer_t t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        c_scan_add_i64(in_i64, out_i64_c, n);
        sink_i64 += out_i64_c[n/2];  // Prevent dead code elimination
    }
    double elapsed_c_i64 = timer_ms_since(&t0);

    t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        fp_scan_add_i64(in_i64, out_i64_asm, n);
        sink_i64 += out_i64_asm[n/2];
    }
    double elapsed_asm_i64 = timer_ms_since(&t0);

    double speedup_i64 = elapsed_c_i64 / elapsed_asm_i64;
    printf("C   : %8.3f ms   (1.00x)\n", elapsed_c_i64);
    printf("ASM : %8.3f ms   (%.2fx)\n", elapsed_asm_i64, speedup_i64);
    printf("\n");

    // --- Benchmark: fp_scan_add_f64 ---
    printf("== scan_add_f64 ==\n");

    t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        c_scan_add_f64(in_f64, out_f64_c, n);
        sink_f64 += out_f64_c[n/2];
    }
    double elapsed_c_f64 = timer_ms_since(&t0);

    t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        fp_scan_add_f64(in_f64, out_f64_asm, n);
        sink_f64 += out_f64_asm[n/2];
    }
    double elapsed_asm_f64 = timer_ms_since(&t0);

    double speedup_f64 = elapsed_c_f64 / elapsed_asm_f64;
    printf("C   : %8.3f ms   (1.00x)\n", elapsed_c_f64);
    printf("ASM : %8.3f ms   (%.2fx)\n", elapsed_asm_f64, speedup_f64);
    printf("\n");

    // Cleanup
    free(in_i64);
    free(out_i64_c);
    free(out_i64_asm);
    free(in_f64);
    free(out_f64_c);
    free(out_f64_asm);

    printf("Done. (sinks: i64=%lld, f64=%.3f)\n", (long long)sink_i64, sink_f64);
    return 0;
}
