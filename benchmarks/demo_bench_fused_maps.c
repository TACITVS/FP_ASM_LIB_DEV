// FP-ASM-DemoBench-Fused-Maps
// Tests the "Fused Maps" module (Module 3).

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
            printf("[FAIL] %s: Mismatch at index %zu (Expected=%.1f, Actual=%.1f, Differs by > %.1e rel_tol)\n",
                   name, i, expected[i], actual[i], rel_tol);
            return false;
        }
    }
    printf("[PASS] %s\n", name);
    return true;
}

// -------------------- Baseline C (Fused Maps) -----------------
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

// -------------------- Main ---------------------------------
int main(int argc, char** argv) {
    size_t n = (argc > 1) ? (size_t)strtoull(argv[1], NULL, 10) : N_DEFAULT;
    int iters = (argc > 2) ? (int)strtol(argv[2], NULL, 10) : ITERS_DEFAULT;

    if (n == 0) n = N_DEFAULT;
    if (iters == 0) iters = ITERS_DEFAULT;

    printf("Benchmark: Fused Maps (Module 3)\n");
    printf("Array Size: n=%" PRIu64 " elements, Iterations: %d\n\n", (uint64_t)n, iters);

    // Allocate buffers
    int64_t* in_i64_a = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* in_i64_b = (int64_t*)xmalloc(n * sizeof(int64_t));
    double* in_f64_a = (double*)xmalloc(n * sizeof(double));
    double* in_f64_b = (double*)xmalloc(n * sizeof(double));
    int64_t* out_i64_c = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* out_i64_asm = (int64_t*)xmalloc(n * sizeof(int64_t));
    double* out_f64_c = (double*)xmalloc(n * sizeof(double));
    double* out_f64_asm = (double*)xmalloc(n * sizeof(double));

    // Fill input
    for (size_t i = 0; i < n; ++i) {
        in_i64_a[i] = (int64_t)(i % 100) - 50;
        in_i64_b[i] = (int64_t)(i % 50) - 25;
        in_f64_a[i] = (double)(i % 100) - 50.0;
        in_f64_b[i] = (double)(i % 50) - 25.0;
    }
    const int64_t const_i64 = 3;
    const double const_f64 = 3.14;

    // A volatile sink to prevent dead code elimination (sum first element)
    volatile int64_t sink_i64 = 0;
    volatile double  sink_f64 = 0.0;

    // ================== CORRECTNESS CHECKS ==================
    bool ok = true;
    printf("Running Correctness Checks...\n");
    double f64_tol = 1e-12; // Tolerance for floating point checks

    // --- axpy ---
    c_map_axpy_i64(in_i64_a, in_i64_b, out_i64_c, n, const_i64);
    fp_map_axpy_i64(in_i64_a, in_i64_b, out_i64_asm, n, const_i64);
    ok &= check_arrays_i64("axpy_i64", out_i64_c, out_i64_asm, n);

    c_map_axpy_f64(in_f64_a, in_f64_b, out_f64_c, n, const_f64);
    fp_map_axpy_f64(in_f64_a, in_f64_b, out_f64_asm, n, const_f64);
    ok &= check_arrays_f64("axpy_f64", out_f64_c, out_f64_asm, n, f64_tol);

    // --- scale ---
    c_map_scale_i64(in_i64_a, out_i64_c, n, const_i64);
    fp_map_scale_i64(in_i64_a, out_i64_asm, n, const_i64);
    ok &= check_arrays_i64("scale_i64", out_i64_c, out_i64_asm, n);

    c_map_scale_f64(in_f64_a, out_f64_c, n, const_f64);
    fp_map_scale_f64(in_f64_a, out_f64_asm, n, const_f64);
    ok &= check_arrays_f64("scale_f64", out_f64_c, out_f64_asm, n, f64_tol);

    // --- offset ---
    c_map_offset_i64(in_i64_a, out_i64_c, n, const_i64);
    fp_map_offset_i64(in_i64_a, out_i64_asm, n, const_i64);
    ok &= check_arrays_i64("offset_i64", out_i64_c, out_i64_asm, n);

    c_map_offset_f64(in_f64_a, out_f64_c, n, const_f64);
    fp_map_offset_f64(in_f64_a, out_f64_asm, n, const_f64);
    ok &= check_arrays_f64("offset_f64", out_f64_c, out_f64_asm, n, f64_tol);

    // --- zip_add ---
    c_zip_add_i64(in_i64_a, in_i64_b, out_i64_c, n);
    fp_zip_add_i64(in_i64_a, in_i64_b, out_i64_asm, n);
    ok &= check_arrays_i64("zip_add_i64", out_i64_c, out_i64_asm, n);

    c_zip_add_f64(in_f64_a, in_f64_b, out_f64_c, n);
    fp_zip_add_f64(in_f64_a, in_f64_b, out_f64_asm, n);
    ok &= check_arrays_f64("zip_add_f64", out_f64_c, out_f64_asm, n, f64_tol);

    if (!ok) {
        printf("Correctness checks FAILED. Halting.\n");
        // Free memory before exiting
        free(in_i64_a); free(in_i64_b); free(in_f64_a); free(in_f64_b);
        free(out_i64_c); free(out_i64_asm); free(out_f64_c); free(out_f64_asm);
        return EXIT_FAILURE;
    }
    printf("All checks passed.\n\n");

    // ================== BENCHMARKS ==================
    hi_timer_t t;
    double c_ms, asm_ms;

    // --- fp_map_axpy_i64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_map_axpy_i64(in_i64_a, in_i64_b, out_i64_c, n, const_i64);
        c_ms += timer_ms_since(&t);
        sink_i64 += out_i64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_map_axpy_i64(in_i64_a, in_i64_b, out_i64_asm, n, const_i64);
        asm_ms += timer_ms_since(&t);
        sink_i64 += out_i64_asm[0];
    }
    printf("== map_axpy_i64 (c*x + y) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_map_axpy_f64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_map_axpy_f64(in_f64_a, in_f64_b, out_f64_c, n, const_f64);
        c_ms += timer_ms_since(&t);
        sink_f64 += out_f64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_map_axpy_f64(in_f64_a, in_f64_b, out_f64_asm, n, const_f64);
        asm_ms += timer_ms_since(&t);
        sink_f64 += out_f64_asm[0];
    }
    printf("\n== map_axpy_f64 (c*x + y) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_map_scale_i64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_map_scale_i64(in_i64_a, out_i64_c, n, const_i64);
        c_ms += timer_ms_since(&t);
        sink_i64 += out_i64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_map_scale_i64(in_i64_a, out_i64_asm, n, const_i64);
        asm_ms += timer_ms_since(&t);
        sink_i64 += out_i64_asm[0];
    }
    printf("\n== map_scale_i64 (c*x) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_map_scale_f64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_map_scale_f64(in_f64_a, out_f64_c, n, const_f64);
        c_ms += timer_ms_since(&t);
        sink_f64 += out_f64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_map_scale_f64(in_f64_a, out_f64_asm, n, const_f64);
        asm_ms += timer_ms_since(&t);
        sink_f64 += out_f64_asm[0];
    }
    printf("\n== map_scale_f64 (c*x) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_map_offset_i64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_map_offset_i64(in_i64_a, out_i64_c, n, const_i64);
        c_ms += timer_ms_since(&t);
        sink_i64 += out_i64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_map_offset_i64(in_i64_a, out_i64_asm, n, const_i64);
        asm_ms += timer_ms_since(&t);
        sink_i64 += out_i64_asm[0];
    }
    printf("\n== map_offset_i64 (x + c) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_map_offset_f64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_map_offset_f64(in_f64_a, out_f64_c, n, const_f64);
        c_ms += timer_ms_since(&t);
        sink_f64 += out_f64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_map_offset_f64(in_f64_a, out_f64_asm, n, const_f64);
        asm_ms += timer_ms_since(&t);
        sink_f64 += out_f64_asm[0];
    }
    printf("\n== map_offset_f64 (x + c) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_zip_add_i64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_zip_add_i64(in_i64_a, in_i64_b, out_i64_c, n);
        c_ms += timer_ms_since(&t);
        sink_i64 += out_i64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_zip_add_i64(in_i64_a, in_i64_b, out_i64_asm, n);
        asm_ms += timer_ms_since(&t);
        sink_i64 += out_i64_asm[0];
    }
    printf("\n== zip_add_i64 (a + b) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));

    // --- fp_zip_add_f64 ---
    c_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        c_zip_add_f64(in_f64_a, in_f64_b, out_f64_c, n);
        c_ms += timer_ms_since(&t);
        sink_f64 += out_f64_c[0];
    }
    asm_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        t = timer_start();
        fp_zip_add_f64(in_f64_a, in_f64_b, out_f64_asm, n);
        asm_ms += timer_ms_since(&t);
        sink_f64 += out_f64_asm[0];
    }
    printf("\n== zip_add_f64 (a + b) ==\n");
    printf("C   : %8.3f ms   (%.2fx)\n", c_ms/iters, 1.0);
    printf("ASM : %8.3f ms   (%.2fx)\n", asm_ms/iters, (c_ms/iters) / (asm_ms/iters));


    // Free memory
    free(in_i64_a); free(in_i64_b); free(in_f64_a); free(in_f64_b);
    free(out_i64_c); free(out_i64_asm); free(out_f64_c); free(out_f64_asm);
    return EXIT_SUCCESS;
}