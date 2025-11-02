/*
 * demo_take_drop.c
 *
 * Tests and benchmarks for List FP early-exit operations:
 * - fp_take_while_gt_i64: Take elements while predicate is true
 * - fp_drop_while_gt_i64: Drop elements while predicate is true, return rest
 *
 * These operations complete the List FP operation suite.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "../include/fp_core.h"

// ============================================================================
// C Baseline Implementations
// ============================================================================

static size_t c_take_while_gt_i64(const int64_t* input, int64_t* output,
                                   size_t n, int64_t threshold) {
    size_t count = 0;
    for (size_t i = 0; i < n; i++) {
        if (input[i] > threshold) {
            output[count++] = input[i];
        } else {
            break;  // Stop at first failure
        }
    }
    return count;
}

static size_t c_drop_while_gt_i64(const int64_t* input, int64_t* output,
                                   size_t n, int64_t threshold) {
    size_t i = 0;
    // Skip elements while predicate is true
    while (i < n && input[i] > threshold) {
        i++;
    }
    // Copy rest
    size_t count = 0;
    for (; i < n; i++) {
        output[count++] = input[i];
    }
    return count;
}

// ============================================================================
// Correctness Testing
// ============================================================================

static bool test_take_while() {
    printf("Testing fp_take_while_gt_i64...\n");

    // Test 1: All elements pass (take all)
    {
        int64_t input[] = {10, 20, 30, 40, 50};
        int64_t output_c[5], output_asm[5];
        size_t n = 5;
        int64_t threshold = 5;

        size_t count_c = c_take_while_gt_i64(input, output_c, n, threshold);
        size_t count_asm = fp_take_while_gt_i64(input, output_asm, n, threshold);

        if (count_c != count_asm) {
            printf("  FAIL: Count mismatch (C=%zu, ASM=%zu)\n", count_c, count_asm);
            return false;
        }
        if (memcmp(output_c, output_asm, count_c * sizeof(int64_t)) != 0) {
            printf("  FAIL: Output mismatch\n");
            return false;
        }
        printf("  Test 1 (all pass): OK (took %zu elements)\n", count_c);
    }

    // Test 2: Stop at middle
    {
        int64_t input[] = {10, 20, 30, 5, 40};
        int64_t output_c[5], output_asm[5];
        size_t n = 5;
        int64_t threshold = 8;

        size_t count_c = c_take_while_gt_i64(input, output_c, n, threshold);
        size_t count_asm = fp_take_while_gt_i64(input, output_asm, n, threshold);

        if (count_c != count_asm) {
            printf("  FAIL: Count mismatch (C=%zu, ASM=%zu)\n", count_c, count_asm);
            return false;
        }
        if (memcmp(output_c, output_asm, count_c * sizeof(int64_t)) != 0) {
            printf("  FAIL: Output mismatch\n");
            return false;
        }
        printf("  Test 2 (stop at middle): OK (took %zu elements)\n", count_c);
    }

    // Test 3: First element fails (take none)
    {
        int64_t input[] = {5, 20, 30, 40, 50};
        int64_t output_c[5], output_asm[5];
        size_t n = 5;
        int64_t threshold = 10;

        size_t count_c = c_take_while_gt_i64(input, output_c, n, threshold);
        size_t count_asm = fp_take_while_gt_i64(input, output_asm, n, threshold);

        if (count_c != count_asm) {
            printf("  FAIL: Count mismatch (C=%zu, ASM=%zu)\n", count_c, count_asm);
            return false;
        }
        printf("  Test 3 (take none): OK (took %zu elements)\n", count_c);
    }

    // Test 4: Large array early exit
    {
        size_t n = 1000;
        int64_t* input = malloc(n * sizeof(int64_t));
        int64_t* output_c = malloc(n * sizeof(int64_t));
        int64_t* output_asm = malloc(n * sizeof(int64_t));

        // First 100 elements > 50, then all < 50
        for (size_t i = 0; i < 100; i++) input[i] = 100;
        for (size_t i = 100; i < n; i++) input[i] = 10;

        size_t count_c = c_take_while_gt_i64(input, output_c, n, 50);
        size_t count_asm = fp_take_while_gt_i64(input, output_asm, n, 50);

        bool ok = (count_c == count_asm && count_c == 100);
        if (!ok) {
            printf("  FAIL: Expected 100, got C=%zu, ASM=%zu\n", count_c, count_asm);
            free(input); free(output_c); free(output_asm);
            return false;
        }
        if (memcmp(output_c, output_asm, count_c * sizeof(int64_t)) != 0) {
            printf("  FAIL: Output mismatch\n");
            free(input); free(output_c); free(output_asm);
            return false;
        }

        printf("  Test 4 (early exit at 100/%zu): OK\n", n);
        free(input); free(output_c); free(output_asm);
    }

    printf("✅ All takeWhile tests PASSED\n\n");
    return true;
}

static bool test_drop_while() {
    printf("Testing fp_drop_while_gt_i64...\n");

    // Test 1: Drop none (first element fails)
    {
        int64_t input[] = {5, 20, 30, 40, 50};
        int64_t output_c[5], output_asm[5];
        size_t n = 5;
        int64_t threshold = 10;

        size_t count_c = c_drop_while_gt_i64(input, output_c, n, threshold);
        size_t count_asm = fp_drop_while_gt_i64(input, output_asm, n, threshold);

        if (count_c != count_asm) {
            printf("  FAIL: Count mismatch (C=%zu, ASM=%zu)\n", count_c, count_asm);
            return false;
        }
        if (memcmp(output_c, output_asm, count_c * sizeof(int64_t)) != 0) {
            printf("  FAIL: Output mismatch\n");
            return false;
        }
        printf("  Test 1 (drop none): OK (returned %zu elements)\n", count_c);
    }

    // Test 2: Drop some (stop at middle)
    {
        int64_t input[] = {10, 20, 30, 5, 40};
        int64_t output_c[5], output_asm[5];
        size_t n = 5;
        int64_t threshold = 8;

        size_t count_c = c_drop_while_gt_i64(input, output_c, n, threshold);
        size_t count_asm = fp_drop_while_gt_i64(input, output_asm, n, threshold);

        if (count_c != count_asm) {
            printf("  FAIL: Count mismatch (C=%zu, ASM=%zu)\n", count_c, count_asm);
            return false;
        }
        if (memcmp(output_c, output_asm, count_c * sizeof(int64_t)) != 0) {
            printf("  FAIL: Output mismatch\n");
            return false;
        }
        printf("  Test 2 (drop 3, keep 2): OK (returned %zu elements)\n", count_c);
    }

    // Test 3: Drop all
    {
        int64_t input[] = {10, 20, 30, 40, 50};
        int64_t output_c[5], output_asm[5];
        size_t n = 5;
        int64_t threshold = 5;

        size_t count_c = c_drop_while_gt_i64(input, output_c, n, threshold);
        size_t count_asm = fp_drop_while_gt_i64(input, output_asm, n, threshold);

        if (count_c != count_asm) {
            printf("  FAIL: Count mismatch (C=%zu, ASM=%zu)\n", count_c, count_asm);
            return false;
        }
        printf("  Test 3 (drop all): OK (returned %zu elements)\n", count_c);
    }

    // Test 4: Large array early exit
    {
        size_t n = 1000;
        int64_t* input = malloc(n * sizeof(int64_t));
        int64_t* output_c = malloc(n * sizeof(int64_t));
        int64_t* output_asm = malloc(n * sizeof(int64_t));

        // First 100 elements > 50, then all < 50
        for (size_t i = 0; i < 100; i++) input[i] = 100;
        for (size_t i = 100; i < n; i++) input[i] = 10;

        size_t count_c = c_drop_while_gt_i64(input, output_c, n, 50);
        size_t count_asm = fp_drop_while_gt_i64(input, output_asm, n, 50);

        bool ok = (count_c == count_asm && count_c == 900);
        if (!ok) {
            printf("  FAIL: Expected 900, got C=%zu, ASM=%zu\n", count_c, count_asm);
            free(input); free(output_c); free(output_asm);
            return false;
        }
        if (memcmp(output_c, output_asm, count_c * sizeof(int64_t)) != 0) {
            printf("  FAIL: Output mismatch\n");
            free(input); free(output_c); free(output_asm);
            return false;
        }

        printf("  Test 4 (early exit, return 900/%zu): OK\n", n);
        free(input); free(output_c); free(output_asm);
    }

    printf("✅ All dropWhile tests PASSED\n\n");
    return true;
}

// ============================================================================
// Performance Benchmarking
// ============================================================================

static void benchmark_take_while(size_t n, int iterations) {
    printf("Benchmarking fp_take_while_gt_i64 (%zu elements, %d iterations)\n", n, iterations);

    int64_t* input = malloc(n * sizeof(int64_t));
    int64_t* output = malloc(n * sizeof(int64_t));

    // Scenario: First 10% pass, then all fail (early exit)
    size_t pass_count = n / 10;
    for (size_t i = 0; i < pass_count; i++) input[i] = 100;
    for (size_t i = pass_count; i < n; i++) input[i] = 10;
    int64_t threshold = 50;

    // Warm-up
    volatile size_t sink = fp_take_while_gt_i64(input, output, n, threshold);
    (void)sink;

    // Benchmark C version
    clock_t start_c = clock();
    for (int iter = 0; iter < iterations; iter++) {
        sink = c_take_while_gt_i64(input, output, n, threshold);
    }
    clock_t end_c = clock();
    double time_c = 1000.0 * (end_c - start_c) / CLOCKS_PER_SEC;

    // Benchmark ASM version
    clock_t start_asm = clock();
    for (int iter = 0; iter < iterations; iter++) {
        sink = fp_take_while_gt_i64(input, output, n, threshold);
    }
    clock_t end_asm = clock();
    double time_asm = 1000.0 * (end_asm - start_asm) / CLOCKS_PER_SEC;

    double speedup = time_c / time_asm;

    printf("  C baseline:  %.2f ms\n", time_c);
    printf("  FP-ASM:      %.2f ms\n", time_asm);
    printf("  Speedup:     %.2fx\n", speedup);
    printf("  (Early exit after %zu/%zu elements)\n\n", pass_count, n);

    free(input);
    free(output);
}

static void benchmark_drop_while(size_t n, int iterations) {
    printf("Benchmarking fp_drop_while_gt_i64 (%zu elements, %d iterations)\n", n, iterations);

    int64_t* input = malloc(n * sizeof(int64_t));
    int64_t* output = malloc(n * sizeof(int64_t));

    // Scenario: First 10% pass (dropped), then 90% fail (returned)
    size_t drop_count = n / 10;
    for (size_t i = 0; i < drop_count; i++) input[i] = 100;
    for (size_t i = drop_count; i < n; i++) input[i] = 10;
    int64_t threshold = 50;

    // Warm-up
    volatile size_t sink = fp_drop_while_gt_i64(input, output, n, threshold);
    (void)sink;

    // Benchmark C version
    clock_t start_c = clock();
    for (int iter = 0; iter < iterations; iter++) {
        sink = c_drop_while_gt_i64(input, output, n, threshold);
    }
    clock_t end_c = clock();
    double time_c = 1000.0 * (end_c - start_c) / CLOCKS_PER_SEC;

    // Benchmark ASM version
    clock_t start_asm = clock();
    for (int iter = 0; iter < iterations; iter++) {
        sink = fp_drop_while_gt_i64(input, output, n, threshold);
    }
    clock_t end_asm = clock();
    double time_asm = 1000.0 * (end_asm - start_asm) / CLOCKS_PER_SEC;

    double speedup = time_c / time_asm;

    printf("  C baseline:  %.2f ms\n", time_c);
    printf("  FP-ASM:      %.2f ms\n", time_asm);
    printf("  Speedup:     %.2fx\n", speedup);
    printf("  (Drop first %zu, return %zu elements)\n\n", drop_count, n - drop_count);

    free(input);
    free(output);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    printf("=================================================================\n");
    printf("FP-ASM List FP Early-Exit Operations Test Suite\n");
    printf("=================================================================\n\n");

    // Parse command line arguments
    size_t n = (argc > 1) ? atoll(argv[1]) : 10000000;  // Default 10M
    int iterations = (argc > 2) ? atoi(argv[2]) : 10;

    printf("Configuration: %zu elements, %d iterations\n\n", n, iterations);

    // Run correctness tests FIRST
    printf("=================================================================\n");
    printf("CORRECTNESS TESTS\n");
    printf("=================================================================\n\n");

    if (!test_take_while()) {
        printf("❌ takeWhile tests FAILED - halting\n");
        return 1;
    }

    if (!test_drop_while()) {
        printf("❌ dropWhile tests FAILED - halting\n");
        return 1;
    }

    // Run performance benchmarks
    printf("=================================================================\n");
    printf("PERFORMANCE BENCHMARKS\n");
    printf("=================================================================\n\n");

    benchmark_take_while(n, iterations);
    benchmark_drop_while(n, iterations);

    printf("=================================================================\n");
    printf("List FP Early-Exit Operations: Complete!\n");
    printf("=================================================================\n");

    return 0;
}
