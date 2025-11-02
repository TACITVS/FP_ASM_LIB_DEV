// FP-ASM-DemoBench-Simple-Maps
// Tests the "Simple Maps" module (Module 4).

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
            printf("[FAIL] %s: Mismatch at index %zu (Expected=%.10f, Actual=%.10f, Differs by > %.1e rel_tol)\n",
                   name, i, expected[i], actual[i], rel_tol);
            return false;
        }
    }
    printf("[PASS] %s\n", name);
    return true;
}

// -------------------- Baseline C (Simple Maps) -----------------
static void c_map_abs_i64(const int64_t* in, int64_t* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        int64_t x = in[i];
        out[i] = (x < 0) ? -x : x;
    }
}

static void c_map_abs_f64(const double* in, double* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        out[i] = fabs(in[i]);
    }
}

static void c_map_sqrt_f64(const double* in, double* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        out[i] = sqrt(in[i]);
    }
}

static void c_map_clamp_i64(const int64_t* in, int64_t* out, size_t n, int64_t min_val, int64_t max_val) {
    for (size_t i = 0; i < n; ++i) {
        int64_t x = in[i];
        if (x < min_val) x = min_val;
        if (x > max_val) x = max_val;
        out[i] = x;
    }
}

static void c_map_clamp_f64(const double* in, double* out, size_t n, double min_val, double max_val) {
    for (size_t i = 0; i < n; ++i) {
        double x = in[i];
        if (x < min_val) x = min_val;
        if (x > max_val) x = max_val;
        out[i] = x;
    }
}

#define N_DEFAULT 10000000
#define ITERS_DEFAULT 10

// -------------------- Main ---------------------------------
int main(int argc, char** argv) {
    size_t n = (argc > 1) ? (size_t)strtoull(argv[1], NULL, 10) : N_DEFAULT;
    int iters = (argc > 2) ? (int)strtol(argv[2], NULL, 10) : ITERS_DEFAULT;

    if (n == 0) n = N_DEFAULT;
    if (iters == 0) iters = ITERS_DEFAULT;

    printf("Benchmark: Simple Maps (Module 4)\n");
    printf("Array Size: n=%" PRIu64 " elements, Iterations: %d\n\n", (uint64_t)n, iters);

    // Allocate buffers
    int64_t* in_i64 = (int64_t*)xmalloc(n * sizeof(int64_t));
    double* in_f64 = (double*)xmalloc(n * sizeof(double));
    int64_t* out_i64_c = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* out_i64_asm = (int64_t*)xmalloc(n * sizeof(int64_t));
    double* out_f64_c = (double*)xmalloc(n * sizeof(double));
    double* out_f64_asm = (double*)xmalloc(n * sizeof(double));

    // Fill input with values that include negatives and positives
    for (size_t i = 0; i < n; ++i) {
        in_i64[i] = (int64_t)(i % 200) - 100;  // Range: -100 to 99
        in_f64[i] = (double)(i % 200) - 100.0; // Range: -100.0 to 99.0
    }

    const int64_t clamp_min_i64 = -50;
    const int64_t clamp_max_i64 = 50;
    const double clamp_min_f64 = -50.0;
    const double clamp_max_f64 = 50.0;

    // A volatile sink to prevent dead code elimination
    volatile int64_t sink_i64 = 0;
    volatile double  sink_f64 = 0.0;

    // ================== CORRECTNESS CHECKS ==================
    bool ok = true;
    printf("Running Correctness Checks...\n");
    double f64_tol = 1e-12; // Tolerance for floating point checks

    // --- abs_i64 ---
    c_map_abs_i64(in_i64, out_i64_c, n);
    fp_map_abs_i64(in_i64, out_i64_asm, n);
    ok &= check_arrays_i64("abs_i64", out_i64_c, out_i64_asm, n);

    // --- abs_f64 ---
    c_map_abs_f64(in_f64, out_f64_c, n);
    fp_map_abs_f64(in_f64, out_f64_asm, n);
    ok &= check_arrays_f64("abs_f64", out_f64_c, out_f64_asm, n, f64_tol);

    // --- sqrt_f64 ---
    // For sqrt, we need non-negative inputs; use absolute values
    for (size_t i = 0; i < n; ++i) {
        in_f64[i] = fabs(in_f64[i]);
    }
    c_map_sqrt_f64(in_f64, out_f64_c, n);
    fp_map_sqrt_f64(in_f64, out_f64_asm, n);
    ok &= check_arrays_f64("sqrt_f64", out_f64_c, out_f64_asm, n, f64_tol);

    // Restore original f64 input for clamp tests
    for (size_t i = 0; i < n; ++i) {
        in_f64[i] = (double)(i % 200) - 100.0;
    }

    // --- clamp_i64 ---
    c_map_clamp_i64(in_i64, out_i64_c, n, clamp_min_i64, clamp_max_i64);
    fp_map_clamp_i64(in_i64, out_i64_asm, n, clamp_min_i64, clamp_max_i64);
    ok &= check_arrays_i64("clamp_i64", out_i64_c, out_i64_asm, n);

    // --- clamp_f64 ---
    c_map_clamp_f64(in_f64, out_f64_c, n, clamp_min_f64, clamp_max_f64);
    fp_map_clamp_f64(in_f64, out_f64_asm, n, clamp_min_f64, clamp_max_f64);
    ok &= check_arrays_f64("clamp_f64", out_f64_c, out_f64_asm, n, f64_tol);

    if (!ok) {
        printf("Correctness checks FAILED. Halting.\n");
        free(in_i64); free(in_f64);
        free(out_i64_c); free(out_i64_asm);
        free(out_f64_c); free(out_f64_asm);
        return EXIT_FAILURE;
    }
    printf("All checks passed.\n\n");

    // ================== BENCHMARKS ==================
    hi_timer_t t;
    double c_ms, asm_ms;

    // --- fp_map_abs_i64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_map_abs_i64(in_i64, out_i64_c, n);
        c_ms += timer_ms_since(&t);
        sink_i64 += out_i64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_map_abs_i64(in_i64, out_i64_asm, n);
        asm_ms += timer_ms_since(&t);
        sink_i64 += out_i64_asm[0];
    }
    printf("== map_abs_i64 ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_map_abs_f64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_map_abs_f64(in_f64, out_f64_c, n);
        c_ms += timer_ms_since(&t);
        sink_f64 += out_f64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_map_abs_f64(in_f64, out_f64_asm, n);
        asm_ms += timer_ms_since(&t);
        sink_f64 += out_f64_asm[0];
    }
    printf("\n== map_abs_f64 ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_map_sqrt_f64 ---
    // Use abs values for sqrt
    for (size_t i = 0; i < n; ++i) {
        in_f64[i] = fabs((double)(i % 200) - 100.0);
    }
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_map_sqrt_f64(in_f64, out_f64_c, n);
        c_ms += timer_ms_since(&t);
        sink_f64 += out_f64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_map_sqrt_f64(in_f64, out_f64_asm, n);
        asm_ms += timer_ms_since(&t);
        sink_f64 += out_f64_asm[0];
    }
    printf("\n== map_sqrt_f64 ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // Restore original f64 input for clamp
    for (size_t i = 0; i < n; ++i) {
        in_f64[i] = (double)(i % 200) - 100.0;
    }

    // --- fp_map_clamp_i64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_map_clamp_i64(in_i64, out_i64_c, n, clamp_min_i64, clamp_max_i64);
        c_ms += timer_ms_since(&t);
        sink_i64 += out_i64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_map_clamp_i64(in_i64, out_i64_asm, n, clamp_min_i64, clamp_max_i64);
        asm_ms += timer_ms_since(&t);
        sink_i64 += out_i64_asm[0];
    }
    printf("\n== map_clamp_i64 ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_map_clamp_f64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_map_clamp_f64(in_f64, out_f64_c, n, clamp_min_f64, clamp_max_f64);
        c_ms += timer_ms_since(&t);
        sink_f64 += out_f64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_map_clamp_f64(in_f64, out_f64_asm, n, clamp_min_f64, clamp_max_f64);
        asm_ms += timer_ms_since(&t);
        sink_f64 += out_f64_asm[0];
    }
    printf("\n== map_clamp_f64 ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // Free memory
    free(in_i64); free(in_f64);
    free(out_i64_c); free(out_i64_asm);
    free(out_f64_c); free(out_f64_asm);
    return EXIT_SUCCESS;
}
