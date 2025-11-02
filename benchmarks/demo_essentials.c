/*
 * demo_essentials.c
 *
 * Test suite for TIER 1 Essential FP Operations
 * Brings library from ~40% to ~70% FP completeness
 *
 * Operations tested:
 * - Index-based: take_n, drop_n, slice
 * - Reductions: product
 * - Search: find_index, contains
 * - Manipulation: reverse, concat, replicate
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include "../include/fp_core.h"

#define TEST_SIZE 10000000
#define ITERATIONS 50

// ============================================================================
// C Baseline Implementations
// ============================================================================

static size_t c_take_n_i64(const int64_t* input, int64_t* output,
                             size_t array_len, size_t take_count) {
    size_t n = (take_count < array_len) ? take_count : array_len;
    for (size_t i = 0; i < n; i++) {
        output[i] = input[i];
    }
    return n;
}

static size_t c_drop_n_i64(const int64_t* input, int64_t* output,
                             size_t array_len, size_t drop_count) {
    if (drop_count >= array_len) return 0;
    size_t n = array_len - drop_count;
    for (size_t i = 0; i < n; i++) {
        output[i] = input[drop_count + i];
    }
    return n;
}

static size_t c_slice_i64(const int64_t* input, int64_t* output,
                           size_t array_len, size_t start, size_t end) {
    if (start >= array_len || start >= end) return 0;
    if (end > array_len) end = array_len;
    size_t n = end - start;
    for (size_t i = 0; i < n; i++) {
        output[i] = input[start + i];
    }
    return n;
}

static int64_t c_reduce_product_i64(const int64_t* input, size_t n) {
    int64_t result = 1;
    for (size_t i = 0; i < n; i++) {
        result *= input[i];
    }
    return result;
}

static double c_reduce_product_f64(const double* input, size_t n) {
    double result = 1.0;
    for (size_t i = 0; i < n; i++) {
        result *= input[i];
    }
    return result;
}

static int64_t c_find_index_i64(const int64_t* input, size_t n, int64_t target) {
    for (size_t i = 0; i < n; i++) {
        if (input[i] == target) return i;
    }
    return -1;
}

static bool c_contains_i64(const int64_t* input, size_t n, int64_t target) {
    for (size_t i = 0; i < n; i++) {
        if (input[i] == target) return true;
    }
    return false;
}

static void c_reverse_i64(const int64_t* input, int64_t* output, size_t n) {
    for (size_t i = 0; i < n; i++) {
        output[i] = input[n - 1 - i];
    }
}

static size_t c_concat_i64(const int64_t* a, const int64_t* b, int64_t* output,
                             size_t na, size_t nb) {
    for (size_t i = 0; i < na; i++) {
        output[i] = a[i];
    }
    for (size_t i = 0; i < nb; i++) {
        output[na + i] = b[i];
    }
    return na + nb;
}

static void c_replicate_i64(int64_t* output, size_t n, int64_t value) {
    for (size_t i = 0; i < n; i++) {
        output[i] = value;
    }
}

// ============================================================================
// Correctness Tests
// ============================================================================

static bool test_take_n() {
    printf("Testing fp_take_n_i64...\n");

    int64_t input[10] = {1,2,3,4,5,6,7,8,9,10};
    int64_t output_c[10], output_asm[10];

    // Test 1: Take 3 from 10
    size_t n_c = c_take_n_i64(input, output_c, 10, 3);
    size_t n_asm = fp_take_n_i64(input, output_asm, 10, 3);

    if (n_c != n_asm || n_c != 3) {
        printf("  FAIL: Count mismatch\n");
        return false;
    }
    if (memcmp(output_c, output_asm, n_c * sizeof(int64_t)) != 0) {
        printf("  FAIL: Output mismatch\n");
        return false;
    }

    // Test 2: Take more than available
    n_c = c_take_n_i64(input, output_c, 10, 20);
    n_asm = fp_take_n_i64(input, output_asm, 10, 20);

    if (n_c != n_asm || n_c != 10) {
        printf("  FAIL: Overflow test failed\n");
        return false;
    }

    printf("  ✅ PASS\n");
    return true;
}

static bool test_drop_n() {
    printf("Testing fp_drop_n_i64...\n");

    int64_t input[10] = {1,2,3,4,5,6,7,8,9,10};
    int64_t output_c[10], output_asm[10];

    // Test 1: Drop 3 from 10
    size_t n_c = c_drop_n_i64(input, output_c, 10, 3);
    size_t n_asm = fp_drop_n_i64(input, output_asm, 10, 3);

    if (n_c != n_asm || n_c != 7) {
        printf("  FAIL: Count mismatch\n");
        return false;
    }
    if (memcmp(output_c, output_asm, n_c * sizeof(int64_t)) != 0) {
        printf("  FAIL: Output mismatch\n");
        return false;
    }

    // Test 2: Drop all
    n_c = c_drop_n_i64(input, output_c, 10, 15);
    n_asm = fp_drop_n_i64(input, output_asm, 10, 15);

    if (n_c != n_asm || n_c != 0) {
        printf("  FAIL: Drop all test failed\n");
        return false;
    }

    printf("  ✅ PASS\n");
    return true;
}

static bool test_slice() {
    printf("Testing fp_slice_i64...\n");

    int64_t input[10] = {1,2,3,4,5,6,7,8,9,10};
    int64_t output_c[10], output_asm[10];

    // Test: Slice [2,5)
    size_t n_c = c_slice_i64(input, output_c, 10, 2, 5);
    size_t n_asm = fp_slice_i64(input, output_asm, 10, 2, 5);

    if (n_c != n_asm || n_c != 3) {
        printf("  FAIL: Count mismatch (got %zu)\n", n_asm);
        return false;
    }
    if (memcmp(output_c, output_asm, n_c * sizeof(int64_t)) != 0) {
        printf("  FAIL: Output mismatch\n");
        return false;
    }

    printf("  ✅ PASS\n");
    return true;
}

static bool test_product() {
    printf("Testing fp_reduce_product_i64...\n");

    int64_t input_i64[5] = {2, 3, 4, 5, 1};
    int64_t result_c = c_reduce_product_i64(input_i64, 5);
    int64_t result_asm = fp_reduce_product_i64(input_i64, 5);

    if (result_c != result_asm || result_c != 120) {
        printf("  FAIL: i64 product mismatch (C=%lld, ASM=%lld)\n", result_c, result_asm);
        return false;
    }

    printf("Testing fp_reduce_product_f64...\n");
    double input_f64[5] = {2.0, 3.0, 4.0, 5.0, 1.0};
    double result_c_f = c_reduce_product_f64(input_f64, 5);
    double result_asm_f = fp_reduce_product_f64(input_f64, 5);

    if (fabs(result_c_f - result_asm_f) > 1e-9 || fabs(result_c_f - 120.0) > 1e-9) {
        printf("  FAIL: f64 product mismatch (C=%.1f, ASM=%.1f)\n", result_c_f, result_asm_f);
        return false;
    }

    printf("  ✅ PASS\n");
    return true;
}

static bool test_find_index() {
    printf("Testing fp_find_index_i64...\n");

    int64_t input[10] = {10,20,30,40,50,60,70,80,90,100};

    // Test 1: Find existing
    int64_t idx_c = c_find_index_i64(input, 10, 50);
    int64_t idx_asm = fp_find_index_i64(input, 10, 50);

    if (idx_c != idx_asm || idx_c != 4) {
        printf("  FAIL: Found at wrong index (C=%lld, ASM=%lld)\n", idx_c, idx_asm);
        return false;
    }

    // Test 2: Not found
    idx_c = c_find_index_i64(input, 10, 999);
    idx_asm = fp_find_index_i64(input, 10, 999);

    if (idx_c != idx_asm || idx_c != -1) {
        printf("  FAIL: Should return -1 for not found\n");
        return false;
    }

    printf("  ✅ PASS\n");
    return true;
}

static bool test_contains() {
    printf("Testing fp_contains_i64...\n");

    int64_t input[10] = {10,20,30,40,50,60,70,80,90,100};

    // Test 1: Contains
    bool c_result = c_contains_i64(input, 10, 50);
    bool asm_result = fp_contains_i64(input, 10, 50);

    if (c_result != asm_result || !c_result) {
        printf("  FAIL: Should find 50\n");
        return false;
    }

    // Test 2: Does not contain
    c_result = c_contains_i64(input, 10, 999);
    asm_result = fp_contains_i64(input, 10, 999);

    if (c_result != asm_result || c_result) {
        printf("  FAIL: Should not find 999\n");
        return false;
    }

    printf("  ✅ PASS\n");
    return true;
}

static bool test_reverse() {
    printf("Testing fp_reverse_i64...\n");

    int64_t input[10] = {1,2,3,4,5,6,7,8,9,10};
    int64_t output_c[10], output_asm[10];

    c_reverse_i64(input, output_c, 10);
    fp_reverse_i64(input, output_asm, 10);

    if (memcmp(output_c, output_asm, 10 * sizeof(int64_t)) != 0) {
        printf("  FAIL: Reverse mismatch\n");
        return false;
    }

    if (output_asm[0] != 10 || output_asm[9] != 1) {
        printf("  FAIL: Reverse incorrect values\n");
        return false;
    }

    printf("  ✅ PASS\n");
    return true;
}

static bool test_concat() {
    printf("Testing fp_concat_i64...\n");

    int64_t a[5] = {1,2,3,4,5};
    int64_t b[3] = {6,7,8};
    int64_t output_c[8], output_asm[8];

    size_t n_c = c_concat_i64(a, b, output_c, 5, 3);
    size_t n_asm = fp_concat_i64(a, b, output_asm, 5, 3);

    if (n_c != n_asm || n_c != 8) {
        printf("  FAIL: Length mismatch\n");
        return false;
    }

    if (memcmp(output_c, output_asm, 8 * sizeof(int64_t)) != 0) {
        printf("  FAIL: Concat mismatch\n");
        return false;
    }

    printf("  ✅ PASS\n");
    return true;
}

static bool test_replicate() {
    printf("Testing fp_replicate_i64...\n");

    int64_t output_c[10], output_asm[10];

    c_replicate_i64(output_c, 10, 42);
    fp_replicate_i64(output_asm, 10, 42);

    if (memcmp(output_c, output_asm, 10 * sizeof(int64_t)) != 0) {
        printf("  FAIL: Replicate mismatch\n");
        return false;
    }

    for (int i = 0; i < 10; i++) {
        if (output_asm[i] != 42) {
            printf("  FAIL: Wrong replicated value\n");
            return false;
        }
    }

    printf("  ✅ PASS\n");
    return true;
}

// ============================================================================
// Performance Benchmarks
// ============================================================================

static void benchmark_index_ops() {
    printf("\n=== Index-Based Operations Benchmark ===\n");
    printf("Array size: %d elements, %d iterations\n\n", TEST_SIZE, ITERATIONS);

    int64_t* input = malloc(TEST_SIZE * sizeof(int64_t));
    int64_t* output = malloc(TEST_SIZE * sizeof(int64_t));

    for (size_t i = 0; i < TEST_SIZE; i++) {
        input[i] = i;
    }

    // take_n benchmark
    clock_t start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        volatile size_t sink = c_take_n_i64(input, output, TEST_SIZE, TEST_SIZE / 2);
    }
    clock_t end = clock();
    double time_c = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        volatile size_t sink = fp_take_n_i64(input, output, TEST_SIZE, TEST_SIZE / 2);
    }
    end = clock();
    double time_asm = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    printf("fp_take_n_i64:  %.2f ms (C)  %.2f ms (ASM)  Speedup: %.2fx\n",
           time_c, time_asm, time_c / time_asm);

    free(input);
    free(output);
}

static void benchmark_product() {
    printf("\n=== Product Reduction Benchmark ===\n");
    printf("Array size: 1000 elements, %d iterations\n\n", ITERATIONS * 100);

    size_t n = 1000;
    int64_t* input_i64 = malloc(n * sizeof(int64_t));
    double* input_f64 = malloc(n * sizeof(double));

    for (size_t i = 0; i < n; i++) {
        input_i64[i] = 1 + (i % 10);
        input_f64[i] = 1.0 + (i % 10) * 0.1;
    }

    // i64 product
    clock_t start = clock();
    for (int iter = 0; iter < ITERATIONS * 100; iter++) {
        volatile int64_t sink = c_reduce_product_i64(input_i64, n);
    }
    clock_t end = clock();
    double time_c = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    start = clock();
    for (int iter = 0; iter < ITERATIONS * 100; iter++) {
        volatile int64_t sink = fp_reduce_product_i64(input_i64, n);
    }
    end = clock();
    double time_asm = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    printf("fp_reduce_product_i64:  %.2f ms (C)  %.2f ms (ASM)  Speedup: %.2fx\n",
           time_c, time_asm, time_c / time_asm);

    // f64 product
    start = clock();
    for (int iter = 0; iter < ITERATIONS * 100; iter++) {
        volatile double sink = c_reduce_product_f64(input_f64, n);
    }
    end = clock();
    time_c = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    start = clock();
    for (int iter = 0; iter < ITERATIONS * 100; iter++) {
        volatile double sink = fp_reduce_product_f64(input_f64, n);
    }
    end = clock();
    time_asm = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    printf("fp_reduce_product_f64:  %.2f ms (C)  %.2f ms (ASM)  Speedup: %.2fx\n",
           time_c, time_asm, time_c / time_asm);

    free(input_i64);
    free(input_f64);
}

static void benchmark_search() {
    printf("\n=== Search Operations Benchmark ===\n");
    printf("Array size: %d elements, %d iterations\n\n", TEST_SIZE, ITERATIONS);

    int64_t* input = malloc(TEST_SIZE * sizeof(int64_t));
    for (size_t i = 0; i < TEST_SIZE; i++) {
        input[i] = i;
    }

    int64_t target = TEST_SIZE / 2;  // Middle element

    // find_index (early exit at middle)
    clock_t start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        volatile int64_t sink = c_find_index_i64(input, TEST_SIZE, target);
    }
    clock_t end = clock();
    double time_c = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        volatile int64_t sink = fp_find_index_i64(input, TEST_SIZE, target);
    }
    end = clock();
    double time_asm = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    printf("fp_find_index_i64 (early exit):  %.2f ms (C)  %.2f ms (ASM)  Speedup: %.2fx\n",
           time_c, time_asm, time_c / time_asm);

    free(input);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=================================================================\n");
    printf("TIER 1 Essential FP Operations Test Suite\n");
    printf("Completeness: 40%% → 70%%\n");
    printf("=================================================================\n\n");

    printf("--- CORRECTNESS TESTS ---\n\n");

    if (!test_take_n()) return 1;
    if (!test_drop_n()) return 1;
    if (!test_slice()) return 1;
    if (!test_product()) return 1;
    if (!test_find_index()) return 1;
    if (!test_contains()) return 1;
    if (!test_reverse()) return 1;
    if (!test_concat()) return 1;
    if (!test_replicate()) return 1;

    printf("\n✅ ALL CORRECTNESS TESTS PASSED!\n");

    printf("\n=================================================================\n");
    printf("--- PERFORMANCE BENCHMARKS ---\n");
    printf("=================================================================\n");

    benchmark_index_ops();
    benchmark_product();
    benchmark_search();

    printf("\n=================================================================\n");
    printf("✅ TIER 1 Essential Operations: COMPLETE!\n");
    printf("=================================================================\n");
    printf("\nLibrary completeness: ~70%%\n");
    printf("New operations: 11 functions (take_n, drop_n, slice, product×2,\n");
    printf("                               find_index, contains, reverse,\n");
    printf("                               concat, replicate)\n");

    return 0;
}
