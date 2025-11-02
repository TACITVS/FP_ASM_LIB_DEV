/*
 * demo_tier2.c
 *
 * Comprehensive test suite for TIER 2 Operations
 * Tests sorting and set operations (unique, union, intersect)
 *
 * Brings library from 70% to ~85% FP completeness
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include "../include/fp_core.h"

#define TEST_SIZE 1000000
#define ITERATIONS 20

// ============================================================================
// C Baseline Implementations
// ============================================================================

static int cmp_i64(const void* a, const void* b) {
    int64_t diff = *(const int64_t*)a - *(const int64_t*)b;
    return (diff > 0) - (diff < 0);
}

static int cmp_f64(const void* a, const void* b) {
    double diff = *(const double*)a - *(const double*)b;
    return (diff > 0.0) - (diff < 0.0);
}

static void c_sort_i64(int64_t* array, size_t n) {
    qsort(array, n, sizeof(int64_t), cmp_i64);
}

static void c_sort_f64(double* array, size_t n) {
    qsort(array, n, sizeof(double), cmp_f64);
}

static size_t c_unique_i64(const int64_t* input, int64_t* output, size_t n) {
    if (n == 0) return 0;

    output[0] = input[0];
    size_t count = 1;

    for (size_t i = 1; i < n; i++) {
        if (input[i] != input[i-1]) {
            output[count++] = input[i];
        }
    }
    return count;
}

static size_t c_union_i64(const int64_t* a, const int64_t* b, int64_t* output,
                           size_t na, size_t nb) {
    size_t i = 0, j = 0, k = 0;

    while (i < na && j < nb) {
        if (a[i] < b[j]) {
            output[k++] = a[i++];
        } else if (a[i] > b[j]) {
            output[k++] = b[j++];
        } else {
            output[k++] = a[i];
            i++; j++;
        }
    }

    while (i < na) output[k++] = a[i++];
    while (j < nb) output[k++] = b[j++];

    return k;
}

static size_t c_intersect_i64(const int64_t* a, const int64_t* b, int64_t* output,
                                size_t na, size_t nb) {
    size_t i = 0, j = 0, k = 0;

    while (i < na && j < nb) {
        if (a[i] < b[j]) {
            i++;
        } else if (a[i] > b[j]) {
            j++;
        } else {
            output[k++] = a[i];
            i++; j++;
        }
    }

    return k;
}

// ============================================================================
// Helper Functions
// ============================================================================

static bool arrays_equal_i64(const int64_t* a, const int64_t* b, size_t n) {
    return memcmp(a, b, n * sizeof(int64_t)) == 0;
}

static bool arrays_equal_f64(const double* a, const double* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (fabs(a[i] - b[i]) > 1e-9) return false;
    }
    return true;
}

static bool is_sorted_i64(const int64_t* array, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (array[i] < array[i-1]) return false;
    }
    return true;
}

static bool is_sorted_f64(const double* array, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (array[i] < array[i-1]) return false;
    }
    return true;
}

static void print_array(const char* name, const int64_t* arr, size_t n) {
    printf("%s: [", name);
    for (size_t i = 0; i < n && i < 10; i++) {
        printf("%lld", arr[i]);
        if (i < n-1 && i < 9) printf(", ");
    }
    if (n > 10) printf(", ...");
    printf("]\n");
}

// ============================================================================
// Correctness Tests
// ============================================================================

static bool test_sort_i64() {
    printf("Testing fp_sort_i64...\n");

    // Test 1: Small array
    int64_t arr1[] = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    int64_t expected1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    size_t n1 = 9;

    fp_sort_i64(arr1, n1);
    if (!arrays_equal_i64(arr1, expected1, n1)) {
        printf("  FAIL: Small array sort incorrect\n");
        print_array("Got", arr1, n1);
        print_array("Expected", expected1, n1);
        return false;
    }

    // Test 2: Already sorted
    int64_t arr2[] = {1, 2, 3, 4, 5};
    int64_t expected2[] = {1, 2, 3, 4, 5};
    size_t n2 = 5;

    fp_sort_i64(arr2, n2);
    if (!arrays_equal_i64(arr2, expected2, n2)) {
        printf("  FAIL: Already sorted array changed\n");
        return false;
    }

    // Test 3: Reverse sorted
    int64_t arr3[] = {5, 4, 3, 2, 1};
    int64_t expected3[] = {1, 2, 3, 4, 5};
    size_t n3 = 5;

    fp_sort_i64(arr3, n3);
    if (!arrays_equal_i64(arr3, expected3, n3)) {
        printf("  FAIL: Reverse sorted incorrect\n");
        return false;
    }

    // Test 4: Duplicates
    int64_t arr4[] = {3, 1, 4, 1, 5, 9, 2, 6, 5};
    size_t n4 = 9;

    int64_t arr4_c[9];
    memcpy(arr4_c, arr4, sizeof(arr4));

    fp_sort_i64(arr4, n4);
    c_sort_i64(arr4_c, n4);

    if (!arrays_equal_i64(arr4, arr4_c, n4)) {
        printf("  FAIL: Duplicates sort mismatch\n");
        return false;
    }

    // Test 5: Large random array
    size_t n5 = 10000;
    int64_t* arr5 = malloc(n5 * sizeof(int64_t));
    int64_t* arr5_c = malloc(n5 * sizeof(int64_t));

    srand(42);
    for (size_t i = 0; i < n5; i++) {
        arr5[i] = arr5_c[i] = rand() % 1000;
    }

    fp_sort_i64(arr5, n5);
    c_sort_i64(arr5_c, n5);

    bool ok = arrays_equal_i64(arr5, arr5_c, n5) && is_sorted_i64(arr5, n5);

    free(arr5);
    free(arr5_c);

    if (!ok) {
        printf("  FAIL: Large random array sort incorrect\n");
        return false;
    }

    printf("  ✅ PASS (5 tests)\n");
    return true;
}

static bool test_sort_f64() {
    printf("Testing fp_sort_f64...\n");

    // Test 1: Small array
    double arr1[] = {5.5, 2.2, 8.8, 1.1, 9.9, 3.3, 7.7, 4.4, 6.6};
    size_t n1 = 9;

    double arr1_c[9];
    memcpy(arr1_c, arr1, sizeof(arr1));

    fp_sort_f64(arr1, n1);
    c_sort_f64(arr1_c, n1);

    if (!arrays_equal_f64(arr1, arr1_c, n1) || !is_sorted_f64(arr1, n1)) {
        printf("  FAIL: Small array sort incorrect\n");
        return false;
    }

    // Test 2: Large random array
    size_t n2 = 10000;
    double* arr2 = malloc(n2 * sizeof(double));
    double* arr2_c = malloc(n2 * sizeof(double));

    srand(42);
    for (size_t i = 0; i < n2; i++) {
        arr2[i] = arr2_c[i] = (rand() % 10000) / 100.0;
    }

    fp_sort_f64(arr2, n2);
    c_sort_f64(arr2_c, n2);

    bool ok = arrays_equal_f64(arr2, arr2_c, n2) && is_sorted_f64(arr2, n2);

    free(arr2);
    free(arr2_c);

    if (!ok) {
        printf("  FAIL: Large random array sort incorrect\n");
        return false;
    }

    printf("  ✅ PASS (2 tests)\n");
    return true;
}

static bool test_unique() {
    printf("Testing fp_unique_i64...\n");

    // Test 1: Simple case
    int64_t input1[] = {1, 2, 2, 3, 3, 3, 4, 5, 5};
    int64_t output_c[9], output_asm[9];
    size_t n1 = 9;

    size_t count_c = c_unique_i64(input1, output_c, n1);
    size_t count_asm = fp_unique_i64(input1, output_asm, n1);

    if (count_c != count_asm) {
        printf("  FAIL: Count mismatch (C=%zu, ASM=%zu)\n", count_c, count_asm);
        return false;
    }

    if (!arrays_equal_i64(output_c, output_asm, count_c)) {
        printf("  FAIL: Output mismatch\n");
        print_array("C output", output_c, count_c);
        print_array("ASM output", output_asm, count_asm);
        return false;
    }

    // Expected: [1, 2, 3, 4, 5]
    int64_t expected[] = {1, 2, 3, 4, 5};
    if (count_c != 5 || !arrays_equal_i64(output_c, expected, 5)) {
        printf("  FAIL: Incorrect unique result\n");
        return false;
    }

    printf("  ✅ PASS\n");
    return true;
}

static bool test_union() {
    printf("Testing fp_union_i64...\n");

    // Test 1: Disjoint sets
    int64_t a1[] = {1, 3, 5, 7};
    int64_t b1[] = {2, 4, 6, 8};
    int64_t output_c[10], output_asm[10];

    size_t count_c = c_union_i64(a1, b1, output_c, 4, 4);
    size_t count_asm = fp_union_i64(a1, b1, output_asm, 4, 4);

    if (count_c != count_asm || count_c != 8) {
        printf("  FAIL: Disjoint union count wrong (C=%zu, ASM=%zu, expected=8)\n", count_c, count_asm);
        return false;
    }

    if (!arrays_equal_i64(output_c, output_asm, count_c)) {
        printf("  FAIL: Disjoint union output mismatch\n");
        return false;
    }

    // Test 2: Overlapping sets
    int64_t a2[] = {1, 2, 3, 4};
    int64_t b2[] = {3, 4, 5, 6};

    count_c = c_union_i64(a2, b2, output_c, 4, 4);
    count_asm = fp_union_i64(a2, b2, output_asm, 4, 4);

    if (count_c != count_asm || count_c != 6) {
        printf("  FAIL: Overlapping union count wrong (expected=6)\n");
        return false;
    }

    if (!arrays_equal_i64(output_c, output_asm, count_c)) {
        printf("  FAIL: Overlapping union output mismatch\n");
        print_array("C output", output_c, count_c);
        print_array("ASM output", output_asm, count_asm);
        return false;
    }

    printf("  ✅ PASS\n");
    return true;
}

static bool test_intersect() {
    printf("Testing fp_intersect_i64...\n");

    // Test 1: No intersection
    int64_t a1[] = {1, 3, 5, 7};
    int64_t b1[] = {2, 4, 6, 8};
    int64_t output_c[10], output_asm[10];

    size_t count_c = c_intersect_i64(a1, b1, output_c, 4, 4);
    size_t count_asm = fp_intersect_i64(a1, b1, output_asm, 4, 4);

    if (count_c != count_asm || count_c != 0) {
        printf("  FAIL: No intersection should return 0\n");
        return false;
    }

    // Test 2: Partial intersection
    int64_t a2[] = {1, 2, 3, 4, 5};
    int64_t b2[] = {3, 4, 5, 6, 7};

    count_c = c_intersect_i64(a2, b2, output_c, 5, 5);
    count_asm = fp_intersect_i64(a2, b2, output_asm, 5, 5);

    if (count_c != count_asm || count_c != 3) {
        printf("  FAIL: Intersection count wrong (C=%zu, ASM=%zu, expected=3)\n", count_c, count_asm);
        return false;
    }

    if (!arrays_equal_i64(output_c, output_asm, count_c)) {
        printf("  FAIL: Intersection output mismatch\n");
        print_array("C output", output_c, count_c);
        print_array("ASM output", output_asm, count_asm);
        return false;
    }

    // Expected: [3, 4, 5]
    int64_t expected[] = {3, 4, 5};
    if (!arrays_equal_i64(output_c, expected, 3)) {
        printf("  FAIL: Incorrect intersection result\n");
        return false;
    }

    printf("  ✅ PASS\n");
    return true;
}

// ============================================================================
// Performance Benchmarks
// ============================================================================

static void benchmark_sort() {
    printf("\n=== Sorting Benchmark ===\n");
    printf("Array size: %d elements, %d iterations\n\n", TEST_SIZE, ITERATIONS);

    // Allocate arrays
    int64_t* arr_i64 = malloc(TEST_SIZE * sizeof(int64_t));
    int64_t* arr_i64_c = malloc(TEST_SIZE * sizeof(int64_t));
    double* arr_f64 = malloc(TEST_SIZE * sizeof(double));
    double* arr_f64_c = malloc(TEST_SIZE * sizeof(double));

    // Initialize with random data
    srand(42);
    for (size_t i = 0; i < TEST_SIZE; i++) {
        arr_i64[i] = rand() % 100000;
        arr_f64[i] = (rand() % 100000) / 100.0;
    }

    // Benchmark i64 sort
    printf("fp_sort_i64:\n");

    clock_t start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        memcpy(arr_i64_c, arr_i64, TEST_SIZE * sizeof(int64_t));
        c_sort_i64(arr_i64_c, TEST_SIZE);
    }
    clock_t end = clock();
    double time_c = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        memcpy(arr_i64_c, arr_i64, TEST_SIZE * sizeof(int64_t));
        fp_sort_i64(arr_i64_c, TEST_SIZE);
    }
    end = clock();
    double time_asm = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    printf("  C (qsort):  %.2f ms\n", time_c);
    printf("  FP-ASM:     %.2f ms\n", time_asm);
    printf("  Speedup:    %.2fx\n\n", time_c / time_asm);

    // Benchmark f64 sort
    printf("fp_sort_f64:\n");

    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        memcpy(arr_f64_c, arr_f64, TEST_SIZE * sizeof(double));
        c_sort_f64(arr_f64_c, TEST_SIZE);
    }
    end = clock();
    time_c = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        memcpy(arr_f64_c, arr_f64, TEST_SIZE * sizeof(double));
        fp_sort_f64(arr_f64_c, TEST_SIZE);
    }
    end = clock();
    time_asm = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    printf("  C (qsort):  %.2f ms\n", time_c);
    printf("  FP-ASM:     %.2f ms\n", time_asm);
    printf("  Speedup:    %.2fx\n", time_c / time_asm);

    free(arr_i64);
    free(arr_i64_c);
    free(arr_f64);
    free(arr_f64_c);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=================================================================\n");
    printf("TIER 2 Operations Test Suite\n");
    printf("Sorting and Set Operations\n");
    printf("Completeness: 70%% → 85%%\n");
    printf("=================================================================\n\n");

    printf("--- CORRECTNESS TESTS ---\n\n");

    if (!test_sort_i64()) return 1;
    if (!test_sort_f64()) return 1;
    if (!test_unique()) return 1;
    if (!test_union()) return 1;
    if (!test_intersect()) return 1;

    printf("\n✅ ALL CORRECTNESS TESTS PASSED!\n");

    printf("\n=================================================================\n");
    printf("--- PERFORMANCE BENCHMARKS ---\n");
    printf("=================================================================\n");

    benchmark_sort();

    printf("\n=================================================================\n");
    printf("✅ TIER 2 Operations: COMPLETE!\n");
    printf("=================================================================\n");
    printf("\nLibrary completeness: ~85%%\n");
    printf("New operations: 5 functions (sort×2, unique, union, intersect)\n");
    printf("\nNow capable of implementing:\n");
    printf("  ✅ Median calculation\n");
    printf("  ✅ Mode finding\n");
    printf("  ✅ Set operations\n");
    printf("  ✅ Sorted array algorithms\n");
    printf("  ✅ Most real-world FP tasks!\n");

    return 0;
}
