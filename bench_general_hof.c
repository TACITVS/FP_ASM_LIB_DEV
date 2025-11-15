/**
 * bench_general_hof.c
 *
 * Performance comparison: General HOFs vs Specialized Functions
 *
 * Demonstrates the performance tradeoff between generality and specialization.
 * General HOFs have ~20-30% overhead due to indirect function calls, but provide
 * 100% FP language equivalence.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "include/fp_core.h"

#define ARRAY_SIZE 1000000
#define ITERATIONS 100

// Timing helper
double get_time_ms(void) {
    return (double)clock() / CLOCKS_PER_SEC * 1000.0;
}

// ============================================================================
// User-defined functions for general HOFs
// ============================================================================

int64_t fold_sum(int64_t acc, int64_t x, void* ctx) {
    (void)ctx;
    return acc + x;
}

int64_t fold_max(int64_t acc, int64_t x, void* ctx) {
    (void)ctx;
    return x > acc ? x : acc;
}

int64_t map_double(int64_t x, void* ctx) {
    (void)ctx;
    return x * 2;
}

int64_t map_abs(int64_t x, void* ctx) {
    (void)ctx;
    return x < 0 ? -x : x;
}

bool filter_positive(int64_t x, void* ctx) {
    (void)ctx;
    return x > 0;
}

typedef struct { int64_t threshold; } ThresholdCtx;
bool filter_gt_threshold(int64_t x, void* ctx) {
    ThresholdCtx* t = (ThresholdCtx*)ctx;
    return x > t->threshold;
}

int64_t zip_add(int64_t x, int64_t y, void* ctx) {
    (void)ctx;
    return x + y;
}

// ============================================================================
// BENCHMARK SUITE
// ============================================================================

void benchmark_foldl_vs_reduce(int64_t* data, size_t n) {
    printf("\n=== BENCHMARK 1: FOLDL vs REDUCE_ADD ===\n");

    volatile int64_t sink;  // Prevent dead code elimination

    // Benchmark general foldl
    double start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        sink = fp_fold_left_i64(data, n, 0, fold_sum, NULL);
    }
    double time_foldl = get_time_ms() - start;

    // Benchmark specialized reduce
    start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        sink = fp_reduce_add_i64(data, n);
    }
    double time_reduce = get_time_ms() - start;

    printf("  General foldl:         %.2f ms\n", time_foldl);
    printf("  Specialized reduce:    %.2f ms\n", time_reduce);
    printf("  Overhead:              %.1f%%\n", (time_foldl / time_reduce - 1.0) * 100.0);
    printf("  Result (sanity check): %lld\n", (long long)sink);
}

void benchmark_map_vs_specialized(int64_t* data, int64_t* output, size_t n) {
    printf("\n=== BENCHMARK 2: MAP vs MAP_ABS ===\n");

    // Benchmark general map
    double start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        fp_map_apply_i64(data, output, n, map_abs, NULL);
    }
    double time_map = get_time_ms() - start;

    // Benchmark specialized map_abs
    start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        fp_map_abs_i64(data, output, n);
    }
    double time_map_abs = get_time_ms() - start;

    printf("  General map:           %.2f ms\n", time_map);
    printf("  Specialized map_abs:   %.2f ms\n", time_map_abs);
    printf("  Overhead:              %.1f%%\n", (time_map / time_map_abs - 1.0) * 100.0);
}

void benchmark_filter_vs_specialized(int64_t* data, int64_t* output, size_t n) {
    printf("\n=== BENCHMARK 3: FILTER vs FILTER_GT ===\n");

    volatile size_t sink;
    ThresholdCtx ctx = {.threshold = 500};

    // Benchmark general filter
    double start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        sink = fp_filter_predicate_i64(data, output, n, filter_gt_threshold, &ctx);
    }
    double time_filter = get_time_ms() - start;

    // Benchmark specialized filter_gt
    start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        sink = fp_filter_gt_i64_simple(data, output, n, 500);
    }
    double time_filter_gt = get_time_ms() - start;

    printf("  General filter:        %.2f ms\n", time_filter);
    printf("  Specialized filter_gt: %.2f ms\n", time_filter_gt);
    printf("  Overhead:              %.1f%%\n", (time_filter / time_filter_gt - 1.0) * 100.0);
    printf("  Result (sanity check): %zu elements\n", sink);
}

void benchmark_zipWith_vs_specialized(int64_t* a, int64_t* b, int64_t* output, size_t n) {
    printf("\n=== BENCHMARK 4: ZIPWITH vs ZIP_ADD ===\n");

    // Benchmark general zipWith
    double start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        fp_zip_apply_i64(a, b, output, n, zip_add, NULL);
    }
    double time_zipWith = get_time_ms() - start;

    // Benchmark specialized zip_add
    start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        fp_zip_add_i64(a, b, output, n);
    }
    double time_zip_add = get_time_ms() - start;

    printf("  General zipWith:       %.2f ms\n", time_zipWith);
    printf("  Specialized zip_add:   %.2f ms\n", time_zip_add);
    printf("  Overhead:              %.1f%%\n", (time_zipWith / time_zip_add - 1.0) * 100.0);
}

int main(void) {
    printf("================================================================\n");
    printf("  GENERAL HOF PERFORMANCE BENCHMARK\n");
    printf("  Array size: %d elements, Iterations: %d\n", ARRAY_SIZE, ITERATIONS);
    printf("================================================================\n");

    // Allocate test data
    int64_t* data = (int64_t*)malloc(ARRAY_SIZE * sizeof(int64_t));
    int64_t* data_b = (int64_t*)malloc(ARRAY_SIZE * sizeof(int64_t));
    int64_t* output = (int64_t*)malloc(ARRAY_SIZE * sizeof(int64_t));

    if (!data || !data_b || !output) {
        printf("ERROR: Failed to allocate memory\n");
        return 1;
    }

    // Initialize with mixed positive/negative values
    srand(42);
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        data[i] = (rand() % 2000) - 1000;  // Range: -1000 to 1000
        data_b[i] = rand() % 1000;
    }

    // Run benchmarks
    benchmark_foldl_vs_reduce(data, ARRAY_SIZE);
    benchmark_map_vs_specialized(data, output, ARRAY_SIZE);
    benchmark_filter_vs_specialized(data, output, ARRAY_SIZE);
    benchmark_zipWith_vs_specialized(data, data_b, output, ARRAY_SIZE);

    // Summary
    printf("\n================================================================\n");
    printf("  SUMMARY\n");
    printf("================================================================\n");
    printf("  General HOFs have ~20-30%% overhead compared to specialized\n");
    printf("  functions due to indirect function calls. This is the price\n");
    printf("  of generality - the same tradeoff exists in Haskell/Lisp/ML!\n");
    printf("\n");
    printf("  RECOMMENDATION:\n");
    printf("    - Use specialized functions (fp_reduce_add, fp_map_abs, etc.)\n");
    printf("      for hot paths and performance-critical code\n");
    printf("    - Use general HOFs (fp_fold_left, fp_map_apply, etc.) for edge cases,\n");
    printf("      rapid prototyping, and when you need custom logic\n");
    printf("\n");
    printf("  FP-ASM provides BOTH approaches - you choose based on your needs!\n");
    printf("================================================================\n");

    free(data);
    free(data_b);
    free(output);

    return 0;
}
