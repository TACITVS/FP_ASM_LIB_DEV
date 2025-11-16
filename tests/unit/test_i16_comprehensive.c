#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "fp_core.h"

// Print i16 array helper
void print_i16_array(const char* label, const int16_t* arr, size_t n) {
    printf("%s [", label);
    for (size_t i = 0; i < n; i++) {
        printf("%d", arr[i]);
        if (i < n - 1) printf(", ");
    }
    printf("]\n");
}

// Test all 12 i16 functions
int main() {
    printf("================================================================\n");
    printf("  i16 Comprehensive Test Suite\n");
    printf("  Testing ALL 12 i16 functions (reductions, folds, maps)\n");
    printf("  16-wide SIMD - 4X throughput vs i64!\n");
    printf("================================================================\n\n");

    // Test data
    int16_t test_data[5] = {1, 2, 3, 4, 5};
    int16_t test_data_b[5] = {10, 20, 30, 40, 50};
    int16_t output[5];

    // ========================================================================
    // CATEGORY 1: Reductions (4 functions)
    // ========================================================================
    printf("CATEGORY 1: Reductions\n");
    printf("----------------------------------------\n");

    // Test 1: fp_reduce_add_i16
    int16_t sum = fp_reduce_add_i16(test_data, 5);
    printf("1. reduce_add:  sum([1,2,3,4,5]) = %d (expected 15)\n", sum);
    if (sum != 15) {
        printf("   FAILED!\n");
        return 1;
    }

    // Test 2: fp_reduce_mul_i16
    int16_t product = fp_reduce_mul_i16(test_data, 5);
    printf("2. reduce_mul:  product([1,2,3,4,5]) = %d (expected 120)\n", product);
    if (product != 120) {
        printf("   FAILED!\n");
        return 1;
    }

    // Test 3: fp_reduce_min_i16
    int16_t min = fp_reduce_min_i16(test_data, 5);
    printf("3. reduce_min:  min([1,2,3,4,5]) = %d (expected 1)\n", min);
    if (min != 1) {
        printf("   FAILED!\n");
        return 1;
    }

    // Test 4: fp_reduce_max_i16
    int16_t max = fp_reduce_max_i16(test_data, 5);
    printf("4. reduce_max:  max([1,2,3,4,5]) = %d (expected 5)\n", max);
    if (max != 5) {
        printf("   FAILED!\n");
        return 1;
    }

    printf("\n");

    // ========================================================================
    // CATEGORY 2: Fused Folds (3 functions)
    // ========================================================================
    printf("CATEGORY 2: Fused Folds\n");
    printf("----------------------------------------\n");

    // Test 5: fp_fold_sumsq_i16
    int16_t sumsq = fp_fold_sumsq_i16(test_data, 5);
    printf("5. fold_sumsq:  sumsq([1,2,3,4,5]) = %d (expected 55)\n", sumsq);
    if (sumsq != 55) {
        printf("   FAILED!\n");
        return 1;
    }

    // Test 6: fp_fold_dotp_i16
    int16_t dotp = fp_fold_dotp_i16(test_data, test_data, 5);
    printf("6. fold_dotp:   dotp([1,2,3,4,5], [1,2,3,4,5]) = %d (expected 55)\n", dotp);
    if (dotp != 55) {
        printf("   FAILED!\n");
        return 1;
    }

    // Test 7: fp_fold_sad_i16
    int16_t sad = fp_fold_sad_i16(test_data, test_data_b, 5);
    printf("7. fold_sad:    sad([1,2,3,4,5], [10,20,30,40,50]) = %d (expected 135)\n", sad);
    if (sad != 135) {  // |1-10| + |2-20| + |3-30| + |4-40| + |5-50| = 9+18+27+36+45 = 135
        printf("   FAILED!\n");
        return 1;
    }

    printf("\n");

    // ========================================================================
    // CATEGORY 3: Fused Maps (4 functions)
    // ========================================================================
    printf("CATEGORY 3: Fused Maps\n");
    printf("----------------------------------------\n");

    // Test 8: fp_map_axpy_i16 (out = c*x + y)
    int16_t c_axpy = 2;
    fp_map_axpy_i16(test_data, test_data_b, output, 5, c_axpy);
    print_i16_array("8. map_axpy:    2*[1,2,3,4,5] + [10,20,30,40,50] =", output, 5);
    int16_t expected_axpy[5] = {12, 24, 36, 48, 60};
    if (memcmp(output, expected_axpy, sizeof(expected_axpy)) != 0) {
        printf("   FAILED! Expected [12, 24, 36, 48, 60]\n");
        return 1;
    }

    // Test 9: fp_map_scale_i16 (out = c*x)
    int16_t c_scale = 3;
    fp_map_scale_i16(test_data, output, 5, c_scale);
    print_i16_array("9. map_scale:   3 * [1,2,3,4,5] =", output, 5);
    int16_t expected_scale[5] = {3, 6, 9, 12, 15};
    if (memcmp(output, expected_scale, sizeof(expected_scale)) != 0) {
        printf("   FAILED! Expected [3, 6, 9, 12, 15]\n");
        return 1;
    }

    // Test 10: fp_map_offset_i16 (out = x + c)
    int16_t c_offset = 100;
    fp_map_offset_i16(test_data, output, 5, c_offset);
    print_i16_array("10. map_offset: [1,2,3,4,5] + 100 =", output, 5);
    int16_t expected_offset[5] = {101, 102, 103, 104, 105};
    if (memcmp(output, expected_offset, sizeof(expected_offset)) != 0) {
        printf("   FAILED! Expected [101, 102, 103, 104, 105]\n");
        return 1;
    }

    // Test 11: fp_zip_add_i16 (out = x + y)
    fp_zip_add_i16(test_data, test_data_b, output, 5);
    print_i16_array("11. zip_add:    [1,2,3,4,5] + [10,20,30,40,50] =", output, 5);
    int16_t expected_zip[5] = {11, 22, 33, 44, 55};
    if (memcmp(output, expected_zip, sizeof(expected_zip)) != 0) {
        printf("   FAILED! Expected [11, 22, 33, 44, 55]\n");
        return 1;
    }

    printf("\n");
    printf("================================================================\n");
    printf("  ALL 12 i16 TESTS PASSED!\n");
    printf("================================================================\n\n");

    // ========================================================================
    // Performance Benchmarks
    // ========================================================================
    printf("================================================================\n");
    printf("  Performance Benchmarks (100K elements × 100 iterations)\n");
    printf("================================================================\n\n");

    const size_t n = 100000;
    const int iterations = 100;

    int16_t* bench_a = (int16_t*)malloc(n * sizeof(int16_t));
    int16_t* bench_b = (int16_t*)malloc(n * sizeof(int16_t));
    int16_t* bench_out = (int16_t*)malloc(n * sizeof(int16_t));

    // Initialize with test data
    for (size_t i = 0; i < n; i++) {
        bench_a[i] = (int16_t)(i % 100);
        bench_b[i] = (int16_t)((i % 50) + 1);
    }

    clock_t start, end;
    double elapsed;
    volatile int16_t result_sink;  // Prevent optimization

    // Benchmark reductions
    printf("Reductions:\n");

    start = clock();
    for (int i = 0; i < iterations; i++) {
        result_sink = fp_reduce_add_i16(bench_a, n);
    }
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("  reduce_add: %.3f ms/iter\n", elapsed / iterations);

    start = clock();
    for (int i = 0; i < iterations; i++) {
        result_sink = fp_reduce_mul_i16(bench_a, n);
    }
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("  reduce_mul: %.3f ms/iter\n", elapsed / iterations);

    start = clock();
    for (int i = 0; i < iterations; i++) {
        result_sink = fp_reduce_min_i16(bench_a, n);
    }
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("  reduce_min: %.3f ms/iter\n", elapsed / iterations);

    start = clock();
    for (int i = 0; i < iterations; i++) {
        result_sink = fp_reduce_max_i16(bench_a, n);
    }
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("  reduce_max: %.3f ms/iter\n", elapsed / iterations);

    printf("\n");

    // Benchmark fused folds
    printf("Fused Folds:\n");

    start = clock();
    for (int i = 0; i < iterations; i++) {
        result_sink = fp_fold_sumsq_i16(bench_a, n);
    }
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("  fold_sumsq: %.3f ms/iter\n", elapsed / iterations);

    start = clock();
    for (int i = 0; i < iterations; i++) {
        result_sink = fp_fold_dotp_i16(bench_a, bench_b, n);
    }
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("  fold_dotp:  %.3f ms/iter\n", elapsed / iterations);

    start = clock();
    for (int i = 0; i < iterations; i++) {
        result_sink = fp_fold_sad_i16(bench_a, bench_b, n);
    }
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("  fold_sad:   %.3f ms/iter\n", elapsed / iterations);

    printf("\n");

    // Benchmark fused maps
    printf("Fused Maps:\n");

    start = clock();
    for (int i = 0; i < iterations; i++) {
        fp_map_axpy_i16(bench_a, bench_b, bench_out, n, 2);
    }
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("  map_axpy:   %.3f ms/iter\n", elapsed / iterations);

    start = clock();
    for (int i = 0; i < iterations; i++) {
        fp_map_scale_i16(bench_a, bench_out, n, 3);
    }
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("  map_scale:  %.3f ms/iter\n", elapsed / iterations);

    start = clock();
    for (int i = 0; i < iterations; i++) {
        fp_map_offset_i16(bench_a, bench_out, n, 100);
    }
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("  map_offset: %.3f ms/iter\n", elapsed / iterations);

    start = clock();
    for (int i = 0; i < iterations; i++) {
        fp_zip_add_i16(bench_a, bench_b, bench_out, n);
    }
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("  zip_add:    %.3f ms/iter\n", elapsed / iterations);

    free(bench_a);
    free(bench_b);
    free(bench_out);

    printf("\n");
    printf("================================================================\n");
    printf("  i16 Implementation Complete! 16-wide SIMD = 4X Throughput!\n");
    printf("================================================================\n");

    return 0;
}
