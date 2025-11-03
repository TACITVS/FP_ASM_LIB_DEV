/**
 * Benchmark: Generic vs. Specialized Performance
 *
 * Compares performance of:
 * 1. Specialized i64/f64 assembly functions
 * 2. Generic type system (pure C)
 * 3. Standard C library functions
 */

#include "include/fp_core.h"
#include "include/fp_generic.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define N 1000000
#define ITERATIONS 100

/* ============================================================================
 * BENCHMARK 1: SUM (Fold/Reduce)
 * ============================================================================ */

/* Generic callback for sum */
void generic_sum_i64(void* acc, const void* elem, void* ctx) {
    (void)ctx;
    *(int64_t*)acc += *(const int64_t*)elem;
}

void benchmark_sum() {
    printf("=== BENCHMARK 1: SUM (1M elements, 100 iterations) ===\n");

    int64_t* data = (int64_t*)malloc(N * sizeof(int64_t));
    for (size_t i = 0; i < N; i++) {
        data[i] = (int64_t)(i % 1000);
    }

    clock_t start, end;
    volatile int64_t sink;

    /* Test 1: Specialized assembly (fp_reduce_add_i64) */
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        sink = fp_reduce_add_i64(data, N);
    }
    end = clock();
    double time_specialized = ((double)(end - start)) / CLOCKS_PER_SEC;

    /* Test 2: Generic fold (pure C with function pointer) */
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        int64_t result = 0;
        fp_foldl_generic(data, N, sizeof(int64_t), &result, generic_sum_i64, NULL);
        sink = result;
    }
    end = clock();
    double time_generic = ((double)(end - start)) / CLOCKS_PER_SEC;

    /* Test 3: Naive C loop (baseline) */
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        int64_t result = 0;
        for (size_t i = 0; i < N; i++) {
            result += data[i];
        }
        sink = result;
    }
    end = clock();
    double time_naive = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("  Specialized assembly: %.3f sec (baseline)\n", time_specialized);
    printf("  Generic C:            %.3f sec (%.2fx slower)\n",
           time_generic, time_generic / time_specialized);
    printf("  Naive C loop:         %.3f sec (%.2fx slower)\n\n",
           time_naive, time_naive / time_specialized);

    free(data);
}

/* ============================================================================
 * BENCHMARK 2: MAP (Transform)
 * ============================================================================ */

/* Generic callback for square */
void generic_square_i64(void* out, const void* in, void* ctx) {
    (void)ctx;
    int64_t val = *(const int64_t*)in;
    *(int64_t*)out = val * val;
}

void benchmark_map() {
    printf("=== BENCHMARK 2: MAP SQUARE (1M elements, 100 iterations) ===\n");

    int64_t* input = (int64_t*)malloc(N * sizeof(int64_t));
    int64_t* output = (int64_t*)malloc(N * sizeof(int64_t));

    for (size_t i = 0; i < N; i++) {
        input[i] = (int64_t)(i % 1000);
    }

    clock_t start, end;

    /* Test 1: Specialized i64 version (if it exists) - use fold for now */
    /* Note: We don't have fp_map_square_i64 yet, so this is a placeholder */
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (size_t i = 0; i < N; i++) {
            output[i] = input[i] * input[i];
        }
    }
    end = clock();
    double time_specialized = ((double)(end - start)) / CLOCKS_PER_SEC;

    /* Test 2: Generic map (pure C with function pointer) */
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        fp_map_generic(input, output, N, sizeof(int64_t), sizeof(int64_t),
                       generic_square_i64, NULL);
    }
    end = clock();
    double time_generic = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("  Naive C loop:         %.3f sec (baseline)\n", time_specialized);
    printf("  Generic C:            %.3f sec (%.2fx slower)\n\n",
           time_generic, time_generic / time_specialized);

    free(input);
    free(output);
}

/* ============================================================================
 * BENCHMARK 3: FILTER
 * ============================================================================ */

bool generic_filter_gt_i64(const void* elem, void* ctx) {
    int64_t threshold = *(int64_t*)ctx;
    return *(const int64_t*)elem > threshold;
}

void benchmark_filter() {
    printf("=== BENCHMARK 3: FILTER (1M elements, 100 iterations) ===\n");

    int64_t* input = (int64_t*)malloc(N * sizeof(int64_t));
    int64_t* output = (int64_t*)malloc(N * sizeof(int64_t));

    for (size_t i = 0; i < N; i++) {
        input[i] = (int64_t)(rand() % 1000);
    }

    int64_t threshold = 500;
    clock_t start, end;

    /* Test 1: Naive C loop */
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        size_t write_idx = 0;
        for (size_t i = 0; i < N; i++) {
            if (input[i] > threshold) {
                output[write_idx++] = input[i];
            }
        }
    }
    end = clock();
    double time_naive = ((double)(end - start)) / CLOCKS_PER_SEC;

    /* Test 2: Generic filter */
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        fp_filter_generic(input, output, N, sizeof(int64_t),
                          generic_filter_gt_i64, &threshold);
    }
    end = clock();
    double time_generic = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("  Naive C loop:         %.3f sec (baseline)\n", time_naive);
    printf("  Generic C:            %.3f sec (%.2fx slower)\n\n",
           time_generic, time_generic / time_naive);

    free(input);
    free(output);
}

/* ============================================================================
 * BENCHMARK 4: QUICKSORT
 * ============================================================================ */

int compare_i64_asc(const void* a, const void* b, void* ctx) {
    (void)ctx;
    int64_t diff = *(const int64_t*)a - *(const int64_t*)b;
    return (diff > 0) ? 1 : (diff < 0) ? -1 : 0;
}

int qsort_compare_i64(const void* a, const void* b) {
    int64_t diff = *(const int64_t*)a - *(const int64_t*)b;
    return (diff > 0) ? 1 : (diff < 0) ? -1 : 0;
}

void benchmark_quicksort() {
    printf("=== BENCHMARK 4: QUICKSORT (100K elements, 10 iterations) ===\n");

    const size_t sort_n = 100000;
    const int sort_iters = 10;

    int64_t* data = (int64_t*)malloc(sort_n * sizeof(int64_t));
    int64_t* sorted = (int64_t*)malloc(sort_n * sizeof(int64_t));

    clock_t start, end;

    /* Test 1: Standard C qsort (MUTATES in-place, NOT pure!) */
    start = clock();
    for (int iter = 0; iter < sort_iters; iter++) {
        /* Re-randomize for each iteration */
        for (size_t i = 0; i < sort_n; i++) {
            data[i] = (int64_t)(rand() % 100000);
        }
        qsort(data, sort_n, sizeof(int64_t), qsort_compare_i64);
    }
    end = clock();
    double time_qsort = ((double)(end - start)) / CLOCKS_PER_SEC;

    /* Test 2: Generic quicksort (pure, immutable) */
    start = clock();
    for (int iter = 0; iter < sort_iters; iter++) {
        /* Re-randomize for each iteration */
        for (size_t i = 0; i < sort_n; i++) {
            data[i] = (int64_t)(rand() % 100000);
        }
        fp_quicksort_generic(data, sorted, sort_n, sizeof(int64_t),
                             compare_i64_asc, NULL);
    }
    end = clock();
    double time_generic = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("  Standard qsort:       %.3f sec (baseline, but MUTATES!)\n", time_qsort);
    printf("  Generic quicksort:    %.3f sec (%.2fx slower, but PURE!)\n\n",
           time_generic, time_generic / time_qsort);

    printf("  NOTE: qsort mutates input (breaks immutability)\n");
    printf("        fp_quicksort_generic preserves input (functional purity)\n\n");

    free(data);
    free(sorted);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                ║\n");
    printf("║   PERFORMANCE COMPARISON: Generic vs. Specialized             ║\n");
    printf("║                                                                ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");

    benchmark_sum();
    benchmark_map();
    benchmark_filter();
    benchmark_quicksort();

    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                ║\n");
    printf("║   CONCLUSION:                                                  ║\n");
    printf("║                                                                ║\n");
    printf("║   Generic functions ARE SLOWER due to:                        ║\n");
    printf("║   - Function pointer overhead (prevents inlining)             ║\n");
    printf("║   - No AVX2 SIMD optimizations                                ║\n");
    printf("║   - Extra memcpy for immutability                             ║\n");
    printf("║                                                                ║\n");
    printf("║   USE SPECIALIZED i64/f64 functions for performance-critical  ║\n");
    printf("║   USE GENERIC functions for flexibility and non-numeric types ║\n");
    printf("║                                                                ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    return 0;
}
