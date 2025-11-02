/**
 * demo_bench_rolling_window.c
 *
 * Benchmark for Algorithm #7: Rolling Window Statistics
 * Demonstrates FUNCTIONAL COMPOSITION pattern vs monolithic reimplementation
 *
 * Build:
 *   gcc demo_bench_rolling_window.c \
 *       build/obj/fp_rolling_window.o \
 *       build/obj/fp_core_reductions.o \
 *       build/obj/fp_core_stats.o \
 *       -o build/bin/bench_rolling_window.exe -I include
 *
 * Run:
 *   ./build/bin/bench_rolling_window.exe [n] [iterations]
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "../include/fp_core.h"

#define DEFAULT_N 1000000
#define DEFAULT_ITERATIONS 10
#define WINDOW_SIZE 20
#define TOLERANCE_EXACT 1e-12      // For min/max (exact operations)
#define TOLERANCE_SUM 1e-6         // For sum/mean (accumulation order matters)

/* ============================================================================
 * BASELINE C IMPLEMENTATIONS (Monolithic - Reimplementing from scratch)
 * ============================================================================ */

// WRONG APPROACH: Reimplements min from scratch (no reuse!)
void c_rolling_min_monolithic(const double* data, size_t n, size_t window, double* output) {
    size_t out_size = n - window + 1;

    for (size_t i = 0; i < out_size; i++) {
        // Reimplemented min logic (NOT reusing existing functions!)
        double min = data[i];
        for (size_t j = 1; j < window; j++) {
            if (data[i + j] < min) {
                min = data[i + j];
            }
        }
        output[i] = min;
    }
}

// WRONG APPROACH: Reimplements max from scratch (no reuse!)
void c_rolling_max_monolithic(const double* data, size_t n, size_t window, double* output) {
    size_t out_size = n - window + 1;

    for (size_t i = 0; i < out_size; i++) {
        // Reimplemented max logic (NOT reusing existing functions!)
        double max = data[i];
        for (size_t j = 1; j < window; j++) {
            if (data[i + j] > max) {
                max = data[i + j];
            }
        }
        output[i] = max;
    }
}

// WRONG APPROACH: Reimplements sum from scratch (no reuse!)
void c_rolling_sum_monolithic(const double* data, size_t n, size_t window, double* output) {
    size_t out_size = n - window + 1;

    for (size_t i = 0; i < out_size; i++) {
        // Reimplemented sum logic (NOT reusing existing functions!)
        double sum = 0.0;
        for (size_t j = 0; j < window; j++) {
            sum += data[i + j];
        }
        output[i] = sum;
    }
}

// RIGHT APPROACH: Composition-based (reuses existing functions!)
void c_rolling_min_composition(const double* data, size_t n, size_t window, double* output) {
    size_t out_size = n - window + 1;

    for (size_t i = 0; i < out_size; i++) {
        output[i] = fp_reduce_min_f64(&data[i], window);  // REUSE!
    }
}

void c_rolling_max_composition(const double* data, size_t n, size_t window, double* output) {
    size_t out_size = n - window + 1;

    for (size_t i = 0; i < out_size; i++) {
        output[i] = fp_reduce_max_f64(&data[i], window);  // REUSE!
    }
}

void c_rolling_sum_composition(const double* data, size_t n, size_t window, double* output) {
    size_t out_size = n - window + 1;

    for (size_t i = 0; i < out_size; i++) {
        output[i] = fp_reduce_add_f64(&data[i], window);  // REUSE!
    }
}

/* ============================================================================
 * CORRECTNESS CHECKS
 * ============================================================================ */

int check_correctness(const char* name,
                     const double* fp_result,
                     const double* c_result,
                     size_t n,
                     double tolerance) {
    for (size_t i = 0; i < n; i++) {
        double diff = fabs(fp_result[i] - c_result[i]);
        double magnitude = fabs(c_result[i]) + 1e-20;  // Avoid division by zero
        double rel_error = diff / magnitude;

        if (rel_error > tolerance) {
            printf("  [FAIL] at index %zu: fp=%.15f, c=%.15f, rel_error=%.2e\n",
                   i, fp_result[i], c_result[i], rel_error);
            return 0;
        }
    }
    printf("  [PASS] (tolerance %.2e)\n", tolerance);
    return 1;
}

void run_correctness_checks(const double* data, size_t n, size_t window) {
    size_t out_size = n - window + 1;
    double* fp_result = (double*)malloc(out_size * sizeof(double));
    double* c_result = (double*)malloc(out_size * sizeof(double));

    printf("=== CORRECTNESS CHECKS (n=%zu, window=%zu) ===\n\n", n, window);

    // Test rolling_min (exact operation - tight tolerance)
    printf("Testing fp_rolling_min_f64...\n");
    fp_rolling_min_f64(data, n, window, fp_result);
    c_rolling_min_monolithic(data, n, window, c_result);
    if (!check_correctness("rolling_min", fp_result, c_result, out_size, TOLERANCE_EXACT)) {
        printf("\n[ABORT] Correctness check failed!\n");
        exit(1);
    }

    // Test rolling_max (exact operation - tight tolerance)
    printf("Testing fp_rolling_max_f64...\n");
    fp_rolling_max_f64(data, n, window, fp_result);
    c_rolling_max_monolithic(data, n, window, c_result);
    if (!check_correctness("rolling_max", fp_result, c_result, out_size, TOLERANCE_EXACT)) {
        printf("\n[ABORT] Correctness check failed!\n");
        exit(1);
    }

    // Test rolling_sum (accumulation - relaxed tolerance)
    printf("Testing fp_rolling_sum_f64...\n");
    fp_rolling_sum_f64(data, n, window, fp_result);
    c_rolling_sum_monolithic(data, n, window, c_result);
    if (!check_correctness("rolling_sum", fp_result, c_result, out_size, TOLERANCE_SUM)) {
        printf("\n[ABORT] Correctness check failed!\n");
        exit(1);
    }

    // Test rolling_mean (accumulation - relaxed tolerance)
    printf("Testing fp_rolling_mean_f64...\n");
    fp_rolling_mean_f64(data, n, window, fp_result);
    c_rolling_sum_monolithic(data, n, window, c_result);
    for (size_t i = 0; i < out_size; i++) {
        c_result[i] /= window;
    }
    if (!check_correctness("rolling_mean", fp_result, c_result, out_size, TOLERANCE_SUM)) {
        printf("\n[ABORT] Correctness check failed!\n");
        exit(1);
    }

    // Test rolling_range (exact operation - tight tolerance)
    printf("Testing fp_rolling_range_f64...\n");
    fp_rolling_range_f64(data, n, window, fp_result);
    double* temp_max = (double*)malloc(out_size * sizeof(double));
    double* temp_min = (double*)malloc(out_size * sizeof(double));
    c_rolling_max_monolithic(data, n, window, temp_max);
    c_rolling_min_monolithic(data, n, window, temp_min);
    for (size_t i = 0; i < out_size; i++) {
        c_result[i] = temp_max[i] - temp_min[i];
    }
    if (!check_correctness("rolling_range", fp_result, c_result, out_size, TOLERANCE_EXACT)) {
        printf("\n[ABORT] Correctness check failed!\n");
        exit(1);
    }
    free(temp_max);
    free(temp_min);

    // Test optimized sum (accumulation - relaxed tolerance)
    printf("Testing fp_rolling_sum_f64_optimized...\n");
    fp_rolling_sum_f64_optimized(data, n, window, fp_result);
    c_rolling_sum_monolithic(data, n, window, c_result);
    if (!check_correctness("rolling_sum_optimized", fp_result, c_result, out_size, TOLERANCE_SUM)) {
        printf("\n[ABORT] Correctness check failed!\n");
        exit(1);
    }

    printf("\n[SUCCESS] All correctness checks PASSED!\n\n");

    free(fp_result);
    free(c_result);
}

/* ============================================================================
 * PERFORMANCE BENCHMARKS
 * ============================================================================ */

double benchmark(void (*fn)(const double*, size_t, size_t, double*),
                const double* data,
                size_t n,
                size_t window,
                double* output,
                int iterations) {
    clock_t start = clock();

    for (int i = 0; i < iterations; i++) {
        fn(data, n, window, output);
    }

    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

void run_performance_benchmarks(const double* data, size_t n, size_t window, int iterations) {
    size_t out_size = n - window + 1;
    double* output = (double*)malloc(out_size * sizeof(double));
    volatile double sink = 0.0;  // Prevent dead code elimination

    printf("=== PERFORMANCE BENCHMARKS ===\n");
    printf("Array size: %zu elements (%.2f MB)\n", n, n * sizeof(double) / 1e6);
    printf("Window size: %zu\n", window);
    printf("Output size: %zu\n", out_size);
    printf("Iterations: %d\n\n", iterations);

    // Benchmark rolling_min
    printf("1. Rolling Min:\n");
    double time_c_mono = benchmark(c_rolling_min_monolithic, data, n, window, output, iterations);
    printf("   C (monolithic):  %.6f s\n", time_c_mono);

    double time_c_comp = benchmark(c_rolling_min_composition, data, n, window, output, iterations);
    printf("   C (composition): %.6f s\n", time_c_comp);

    double time_fp = benchmark(fp_rolling_min_f64, data, n, window, output, iterations);
    printf("   FP (composition):  %.6f s\n", time_fp);
    printf("   Speedup vs C mono: %.2fx\n", time_c_mono / time_fp);
    printf("   Speedup vs C comp: %.2fx\n\n", time_c_comp / time_fp);
    sink += output[0];

    // Benchmark rolling_max
    printf("2. Rolling Max:\n");
    time_c_mono = benchmark(c_rolling_max_monolithic, data, n, window, output, iterations);
    printf("   C (monolithic):  %.6f s\n", time_c_mono);

    time_c_comp = benchmark(c_rolling_max_composition, data, n, window, output, iterations);
    printf("   C (composition): %.6f s\n", time_c_comp);

    time_fp = benchmark(fp_rolling_max_f64, data, n, window, output, iterations);
    printf("   FP (composition):  %.6f s\n", time_fp);
    printf("   Speedup vs C mono: %.2fx\n", time_c_mono / time_fp);
    printf("   Speedup vs C comp: %.2fx\n\n", time_c_comp / time_fp);
    sink += output[0];

    // Benchmark rolling_sum (generic vs optimized)
    printf("3. Rolling Sum (Generic vs Optimized):\n");
    time_c_mono = benchmark(c_rolling_sum_monolithic, data, n, window, output, iterations);
    printf("   C (monolithic):  %.6f s\n", time_c_mono);

    time_fp = benchmark(fp_rolling_sum_f64, data, n, window, output, iterations);
    printf("   FP (generic):    %.6f s\n", time_fp);
    printf("   Speedup:         %.2fx\n", time_c_mono / time_fp);

    double time_fp_opt = benchmark(fp_rolling_sum_f64_optimized, data, n, window, output, iterations);
    printf("   FP (optimized):  %.6f s\n", time_fp_opt);
    printf("   Speedup:         %.2fx\n", time_c_mono / time_fp_opt);
    printf("   Generic vs Opt:  %.2fx slower\n\n", time_fp / time_fp_opt);
    sink += output[0];

    // Benchmark rolling_mean
    printf("4. Rolling Mean:\n");
    time_fp = benchmark(fp_rolling_mean_f64, data, n, window, output, iterations);
    printf("   FP (generic):    %.6f s\n", time_fp);

    time_fp_opt = benchmark(fp_rolling_mean_f64_optimized, data, n, window, output, iterations);
    printf("   FP (optimized):  %.6f s\n", time_fp_opt);
    printf("   Generic vs Opt:  %.2fx slower\n\n", time_fp / time_fp_opt);
    sink += output[0];

    // Benchmark rolling_range (demonstrates composition of multiple functions)
    printf("5. Rolling Range (max - min composition):\n");
    time_fp = benchmark(fp_rolling_range_f64, data, n, window, output, iterations);
    printf("   FP (composition):  %.6f s\n\n", time_fp);
    sink += output[0];

    printf("Sink: %.15f (prevent dead code elimination)\n", sink);
    free(output);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(int argc, char** argv) {
    size_t n = (argc > 1) ? atoi(argv[1]) : DEFAULT_N;
    int iterations = (argc > 2) ? atoi(argv[2]) : DEFAULT_ITERATIONS;
    size_t window = WINDOW_SIZE;

    printf("===================================================================\n");
    printf("  Algorithm #7: Rolling Window Statistics\n");
    printf("  Functional Composition Pattern Benchmark\n");
    printf("===================================================================\n\n");

    printf("Design Philosophy:\n");
    printf("  * Composition over reimplementation\n");
    printf("  * Reuse existing optimized reduce_* functions\n");
    printf("  * Higher-order functions (function pointers)\n");
    printf("  * DRY principle\n\n");

    // Allocate and initialize data
    double* data = (double*)malloc(n * sizeof(double));
    if (!data) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 1;
    }

    // Initialize with mixed values (positive, negative, zeros)
    srand(42);
    for (size_t i = 0; i < n; i++) {
        data[i] = ((double)rand() / RAND_MAX) * 200.0 - 100.0;  // Range: [-100, 100]
    }

    // Run correctness checks first
    run_correctness_checks(data, n, window);

    // Run performance benchmarks
    run_performance_benchmarks(data, n, window, iterations);

    free(data);

    printf("\n===================================================================\n");
    printf("  Key Takeaway: Composition Pattern\n");
    printf("===================================================================\n");
    printf("  rolling_min = rolling (compose) reduce_min\n");
    printf("  rolling_max = rolling (compose) reduce_max\n");
    printf("  rolling_sum = rolling (compose) reduce_add\n");
    printf("\n");
    printf("  Benefits:\n");
    printf("  * Code reuse (DRY)\n");
    printf("  * Leverages existing SIMD optimizations\n");
    printf("  * Extensible (new reductions work automatically)\n");
    printf("  * True functional programming in C!\n");
    printf("===================================================================\n");

    return 0;
}
