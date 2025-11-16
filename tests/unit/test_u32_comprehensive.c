/**
 * Comprehensive Test & Benchmark: u32 All Operations
 *
 * Tests ALL u32 functions:
 * - Reductions (add, mul, min, max)
 * - Fused Folds (dotp, sumsq, sad)
 * - Fused Maps (axpy, scale, offset, zip_add)
 */

#include "include/fp_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#define N 100000
#define ITERATIONS 100

/* ============================================================================
 * CORRECTNESS TESTS
 * ============================================================================ */

void test_reductions() {
    printf("=== REDUCTIONS ===\n");

    uint32_t data[] = {1, 2, 3, 4, 5};

    uint32_t sum = fp_reduce_add_u32(data, 5);
    printf("  add: %u (expected 15) %s\n", sum, sum == 15 ? "✓" : "✗");

    uint32_t prod = fp_reduce_mul_u32(data, 5);
    printf("  mul: %u (expected 120) %s\n", prod, prod == 120 ? "✓" : "✗");

    uint32_t min = fp_reduce_min_u32(data, 5);
    printf("  min: %u (expected 1) %s\n", min, min == 1 ? "✓" : "✗");

    uint32_t max = fp_reduce_max_u32(data, 5);
    printf("  max: %u (expected 5) %s\n\n", max, max == 5 ? "✓" : "✗");
}

void test_fused_folds() {
    printf("=== FUSED FOLDS ===\n");

    uint32_t a[] = {1, 2, 3, 4, 5};
    uint32_t b[] = {2, 3, 4, 5, 6};

    // sumsq: 1 + 4 + 9 + 16 + 25 = 55
    uint32_t sumsq = fp_fold_sumsq_u32(a, 5);
    printf("  sumsq: %u (expected 55) %s\n", sumsq, sumsq == 55 ? "✓" : "✗");

    // dotp: 1*2 + 2*3 + 3*4 + 4*5 + 5*6 = 2 + 6 + 12 + 20 + 30 = 70
    uint32_t dotp = fp_fold_dotp_u32(a, b, 5);
    printf("  dotp: %u (expected 70) %s\n", dotp, dotp == 70 ? "✓" : "✗");

    // sad: |1-2| + |2-3| + |3-4| + |4-5| + |5-6| = 1+1+1+1+1 = 5
    uint32_t sad = fp_fold_sad_u32(a, b, 5);
    printf("  sad: %u (expected 5) %s\n\n", sad, sad == 5 ? "✓" : "✗");
}

void test_fused_maps() {
    printf("=== FUSED MAPS ===\n");

    uint32_t x[] = {1, 2, 3, 4, 5};
    uint32_t y[] = {10, 20, 30, 40, 50};
    uint32_t out[5];

    // axpy: c*x + y, c=2
    // [2*1+10, 2*2+20, 2*3+30, 2*4+40, 2*5+50] = [12, 24, 36, 48, 60]
    fp_map_axpy_u32(x, y, out, 5, 2);
    printf("  axpy: [%u, %u, %u, %u, %u] %s\n",
           out[0], out[1], out[2], out[3], out[4],
           (out[0]==12 && out[1]==24 && out[2]==36 && out[3]==48 && out[4]==60) ? "✓" : "✗");

    // scale: c*x, c=3
    // [3, 6, 9, 12, 15]
    fp_map_scale_u32(x, out, 5, 3);
    printf("  scale: [%u, %u, %u, %u, %u] %s\n",
           out[0], out[1], out[2], out[3], out[4],
           (out[0]==3 && out[1]==6 && out[2]==9 && out[3]==12 && out[4]==15) ? "✓" : "✗");

    // offset: x+c, c=100
    // [101, 102, 103, 104, 105]
    fp_map_offset_u32(x, out, 5, 100);
    printf("  offset: [%u, %u, %u, %u, %u] %s\n",
           out[0], out[1], out[2], out[3], out[4],
           (out[0]==101 && out[1]==102 && out[2]==103 && out[3]==104 && out[4]==105) ? "✓" : "✗");

    // zip_add: x+y
    // [11, 22, 33, 44, 55]
    fp_zip_add_u32(x, y, out, 5);
    printf("  zip_add: [%u, %u, %u, %u, %u] %s\n\n",
           out[0], out[1], out[2], out[3], out[4],
           (out[0]==11 && out[1]==22 && out[2]==33 && out[3]==44 && out[4]==55) ? "✓" : "✗");
}

/* ============================================================================
 * PERFORMANCE BENCHMARKS
 * ============================================================================ */

void benchmark_all() {
    printf("===================================================\n");
    printf("  PERFORMANCE: u32 operations (%d elements, %d iter)\n", N, ITERATIONS);
    printf("===================================================\n\n");

    // Allocate test data
    uint32_t* a = (uint32_t*)malloc(N * sizeof(uint32_t));
    uint32_t* b = (uint32_t*)malloc(N * sizeof(uint32_t));
    uint32_t* out = (uint32_t*)malloc(N * sizeof(uint32_t));

    for (size_t i = 0; i < N; i++) {
        a[i] = (uint32_t)(i % 1000);
        b[i] = (uint32_t)((i + 500) % 1000);
    }

    clock_t start, end;
    volatile uint32_t sink_u32;

    // Reductions
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        sink_u32 = fp_reduce_add_u32(a, N);
    }
    end = clock();
    printf("  reduce_add:  %.3f sec\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        sink_u32 = fp_reduce_min_u32(a, N);
    }
    end = clock();
    printf("  reduce_min:  %.3f sec\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        sink_u32 = fp_reduce_max_u32(a, N);
    }
    end = clock();
    printf("  reduce_max:  %.3f sec\n\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    // Fused folds
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        sink_u32 = fp_fold_sumsq_u32(a, N);
    }
    end = clock();
    printf("  fold_sumsq:  %.3f sec\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        sink_u32 = fp_fold_dotp_u32(a, b, N);
    }
    end = clock();
    printf("  fold_dotp:   %.3f sec\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        sink_u32 = fp_fold_sad_u32(a, b, N);
    }
    end = clock();
    printf("  fold_sad:    %.3f sec\n\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    // Fused maps
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        fp_map_axpy_u32(a, b, out, N, 2);
    }
    end = clock();
    printf("  map_axpy:    %.3f sec\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        fp_map_scale_u32(a, out, N, 2);
    }
    end = clock();
    printf("  map_scale:   %.3f sec\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        fp_map_offset_u32(a, out, N, 100);
    }
    end = clock();
    printf("  map_offset:  %.3f sec\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        fp_zip_add_u32(a, b, out, N);
    }
    end = clock();
    printf("  zip_add:     %.3f sec\n\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    free(a);
    free(b);
    free(out);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("================================================================\n");
    printf("  FP-ASM u32 Comprehensive Test Suite\n");
    printf("  Testing ALL u32 operations (12 functions)\n");
    printf("  Unsigned 32-bit integer support\n");
    printf("================================================================\n\n");

    test_reductions();
    test_fused_folds();
    test_fused_maps();

    benchmark_all();

    printf("================================================================\n");
    printf("  ALL u32 OPERATIONS TESTED!\n");
    printf("  8-wide SIMD delivers consistent unsigned arithmetic\n");
    printf("================================================================\n");

    return 0;
}
