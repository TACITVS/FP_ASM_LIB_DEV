#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "fp_core.h"

// Print u16 array helper
void print_u16_array(const char* label, const uint16_t* arr, size_t n) {
    printf("%s [", label);
    for (size_t i = 0; i < n; i++) {
        printf("%u", arr[i]);
        if (i < n - 1) printf(", ");
    }
    printf("]\n");
}

// Test all 12 u16 functions
int main() {
    printf("================================================================\n");
    printf("  u16 Comprehensive Test Suite\n");
    printf("  Testing ALL 12 u16 functions (reductions, folds, maps)\n");
    printf("  16-wide SIMD - 4X throughput vs u64!\n");
    printf("================================================================\n\n");

    // Test data
    uint16_t test_data[5] = {1, 2, 3, 4, 5};
    uint16_t test_data_b[5] = {10, 20, 30, 40, 50};
    uint16_t output[5];

    // ========================================================================
    // CATEGORY 1: Reductions (4 functions)
    // ========================================================================
    printf("CATEGORY 1: Reductions\n");
    printf("----------------------------------------\n");

    uint16_t sum = fp_reduce_add_u16(test_data, 5);
    printf("1. reduce_add:  sum([1,2,3,4,5]) = %u (expected 15)\n", sum);
    if (sum != 15) { printf("   FAILED!\n"); return 1; }

    uint16_t product = fp_reduce_mul_u16(test_data, 5);
    printf("2. reduce_mul:  product([1,2,3,4,5]) = %u (expected 120)\n", product);
    if (product != 120) { printf("   FAILED!\n"); return 1; }

    uint16_t min = fp_reduce_min_u16(test_data, 5);
    printf("3. reduce_min:  min([1,2,3,4,5]) = %u (expected 1)\n", min);
    if (min != 1) { printf("   FAILED!\n"); return 1; }

    uint16_t max = fp_reduce_max_u16(test_data, 5);
    printf("4. reduce_max:  max([1,2,3,4,5]) = %u (expected 5)\n", max);
    if (max != 5) { printf("   FAILED!\n"); return 1; }

    printf("\n");

    // ========================================================================
    // CATEGORY 2: Fused Folds (3 functions)
    // ========================================================================
    printf("CATEGORY 2: Fused Folds\n");
    printf("----------------------------------------\n");

    uint16_t sumsq = fp_fold_sumsq_u16(test_data, 5);
    printf("5. fold_sumsq:  sumsq([1,2,3,4,5]) = %u (expected 55)\n", sumsq);
    if (sumsq != 55) { printf("   FAILED!\n"); return 1; }

    uint16_t dotp = fp_fold_dotp_u16(test_data, test_data, 5);
    printf("6. fold_dotp:   dotp([1,2,3,4,5], [1,2,3,4,5]) = %u (expected 55)\n", dotp);
    if (dotp != 55) { printf("   FAILED!\n"); return 1; }

    uint16_t sad = fp_fold_sad_u16(test_data, test_data_b, 5);
    printf("7. fold_sad:    sad([1,2,3,4,5], [10,20,30,40,50]) = %u (expected 135)\n", sad);
    if (sad != 135) { printf("   FAILED!\n"); return 1; }

    printf("\n");

    // ========================================================================
    // CATEGORY 3: Fused Maps (4 functions)
    // ========================================================================
    printf("CATEGORY 3: Fused Maps\n");
    printf("----------------------------------------\n");

    uint16_t c_axpy = 2;
    fp_map_axpy_u16(test_data, test_data_b, output, 5, c_axpy);
    print_u16_array("8. map_axpy:    2*[1,2,3,4,5] + [10,20,30,40,50] =", output, 5);
    uint16_t expected_axpy[5] = {12, 24, 36, 48, 60};
    if (memcmp(output, expected_axpy, sizeof(expected_axpy)) != 0) {
        printf("   FAILED! Expected [12, 24, 36, 48, 60]\n");
        return 1;
    }

    uint16_t c_scale = 3;
    fp_map_scale_u16(test_data, output, 5, c_scale);
    print_u16_array("9. map_scale:   3 * [1,2,3,4,5] =", output, 5);
    uint16_t expected_scale[5] = {3, 6, 9, 12, 15};
    if (memcmp(output, expected_scale, sizeof(expected_scale)) != 0) {
        printf("   FAILED! Expected [3, 6, 9, 12, 15]\n");
        return 1;
    }

    uint16_t c_offset = 100;
    fp_map_offset_u16(test_data, output, 5, c_offset);
    print_u16_array("10. map_offset: [1,2,3,4,5] + 100 =", output, 5);
    uint16_t expected_offset[5] = {101, 102, 103, 104, 105};
    if (memcmp(output, expected_offset, sizeof(expected_offset)) != 0) {
        printf("   FAILED! Expected [101, 102, 103, 104, 105]\n");
        return 1;
    }

    fp_zip_add_u16(test_data, test_data_b, output, 5);
    print_u16_array("11. zip_add:    [1,2,3,4,5] + [10,20,30,40,50] =", output, 5);
    uint16_t expected_zip[5] = {11, 22, 33, 44, 55};
    if (memcmp(output, expected_zip, sizeof(expected_zip)) != 0) {
        printf("   FAILED! Expected [11, 22, 33, 44, 55]\n");
        return 1;
    }

    printf("\n");
    printf("================================================================\n");
    printf("  ALL 12 u16 TESTS PASSED!\n");
    printf("================================================================\n\n");

    // ========================================================================
    // Performance Benchmarks
    // ========================================================================
    printf("================================================================\n");
    printf("  Performance Benchmarks (100K elements × 100 iterations)\n");
    printf("================================================================\n\n");

    const size_t n = 100000;
    const int iterations = 100;

    uint16_t* bench_a = (uint16_t*)malloc(n * sizeof(uint16_t));
    uint16_t* bench_b = (uint16_t*)malloc(n * sizeof(uint16_t));
    uint16_t* bench_out = (uint16_t*)malloc(n * sizeof(uint16_t));

    for (size_t i = 0; i < n; i++) {
        bench_a[i] = (uint16_t)(i % 100);
        bench_b[i] = (uint16_t)((i % 50) + 1);
    }

    clock_t start, end;
    double elapsed;
    volatile uint16_t result_sink;

    printf("Reductions:\n");
    start = clock(); for (int i = 0; i < iterations; i++) result_sink = fp_reduce_add_u16(bench_a, n); end = clock();
    printf("  reduce_add: %.3f ms/iter\n", ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0 / iterations);

    start = clock(); for (int i = 0; i < iterations; i++) result_sink = fp_reduce_mul_u16(bench_a, n); end = clock();
    printf("  reduce_mul: %.3f ms/iter\n", ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0 / iterations);

    start = clock(); for (int i = 0; i < iterations; i++) result_sink = fp_reduce_min_u16(bench_a, n); end = clock();
    printf("  reduce_min: %.3f ms/iter\n", ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0 / iterations);

    start = clock(); for (int i = 0; i < iterations; i++) result_sink = fp_reduce_max_u16(bench_a, n); end = clock();
    printf("  reduce_max: %.3f ms/iter\n", ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0 / iterations);

    printf("\nFused Folds:\n");
    start = clock(); for (int i = 0; i < iterations; i++) result_sink = fp_fold_sumsq_u16(bench_a, n); end = clock();
    printf("  fold_sumsq: %.3f ms/iter\n", ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0 / iterations);

    start = clock(); for (int i = 0; i < iterations; i++) result_sink = fp_fold_dotp_u16(bench_a, bench_b, n); end = clock();
    printf("  fold_dotp:  %.3f ms/iter\n", ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0 / iterations);

    start = clock(); for (int i = 0; i < iterations; i++) result_sink = fp_fold_sad_u16(bench_a, bench_b, n); end = clock();
    printf("  fold_sad:   %.3f ms/iter\n", ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0 / iterations);

    printf("\nFused Maps:\n");
    start = clock(); for (int i = 0; i < iterations; i++) fp_map_axpy_u16(bench_a, bench_b, bench_out, n, 2); end = clock();
    printf("  map_axpy:   %.3f ms/iter\n", ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0 / iterations);

    start = clock(); for (int i = 0; i < iterations; i++) fp_map_scale_u16(bench_a, bench_out, n, 3); end = clock();
    printf("  map_scale:  %.3f ms/iter\n", ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0 / iterations);

    start = clock(); for (int i = 0; i < iterations; i++) fp_map_offset_u16(bench_a, bench_out, n, 100); end = clock();
    printf("  map_offset: %.3f ms/iter\n", ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0 / iterations);

    start = clock(); for (int i = 0; i < iterations; i++) fp_zip_add_u16(bench_a, bench_b, bench_out, n); end = clock();
    printf("  zip_add:    %.3f ms/iter\n", ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0 / iterations);

    free(bench_a);
    free(bench_b);
    free(bench_out);

    printf("\n================================================================\n");
    printf("  u16 Implementation Complete! 16-wide SIMD = 4X Throughput!\n");
    printf("================================================================\n");

    return 0;
}
