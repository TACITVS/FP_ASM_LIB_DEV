// test_conditional_reduction.c
//
// Comprehensive tests for fp_reduce_add_f64_where() conditional reduction
//
// Tests:
//   1. Edge cases: n=0, all masked, none masked, single element
//   2. Correctness: Known values with partial masks
//   3. Consistency: Results match fp_reduce_add_f64 when all elements masked
//   4. Larger arrays: Verify correctness for arrays > 16 elements
//   5. Non-boolean mask values: Verify any non-zero value is treated as true
//
// Model: claude-sonnet-4-5-20250929
// Timestamp: 2025-11-23

#include "fp_core.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define TEST_PASS(name) printf("[PASS] %s\n", name)
#define TEST_FAIL(name, ...) do { printf("[FAIL] %s: ", name); printf(__VA_ARGS__); printf("\n"); failures++; } while(0)
#define EPSILON 1e-10

int failures = 0;

// ============================================================================
// Test 1: Edge Cases
// ============================================================================

void test_edge_cases() {
    printf("\n=== Test 1: Edge Cases ===\n");

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    int mask_all[] = {1, 1, 1, 1, 1};
    int mask_none[] = {0, 0, 0, 0, 0};

    // Test: n=0 (should return 0.0)
    double result_empty = fp_reduce_add_f64_where(data, mask_all, 0);
    if (fabs(result_empty - 0.0) < EPSILON) {
        TEST_PASS("f64_where n=0 returns 0.0");
    } else {
        TEST_FAIL("f64_where n=0", "Expected 0.0, got %f", result_empty);
    }

    // Test: all elements masked (should equal sum of all = 15.0)
    double result_all = fp_reduce_add_f64_where(data, mask_all, 5);
    if (fabs(result_all - 15.0) < EPSILON) {
        TEST_PASS("f64_where all masked = 15.0");
    } else {
        TEST_FAIL("f64_where all masked", "Expected 15.0, got %f", result_all);
    }

    // Test: no elements masked (should return 0.0)
    double result_none = fp_reduce_add_f64_where(data, mask_none, 5);
    if (fabs(result_none - 0.0) < EPSILON) {
        TEST_PASS("f64_where none masked = 0.0");
    } else {
        TEST_FAIL("f64_where none masked", "Expected 0.0, got %f", result_none);
    }

    // Test: single element, masked
    double single_data[] = {42.5};
    int single_mask_on[] = {1};
    double result_single_on = fp_reduce_add_f64_where(single_data, single_mask_on, 1);
    if (fabs(result_single_on - 42.5) < EPSILON) {
        TEST_PASS("f64_where single element masked = 42.5");
    } else {
        TEST_FAIL("f64_where single element masked", "Expected 42.5, got %f", result_single_on);
    }

    // Test: single element, not masked
    int single_mask_off[] = {0};
    double result_single_off = fp_reduce_add_f64_where(single_data, single_mask_off, 1);
    if (fabs(result_single_off - 0.0) < EPSILON) {
        TEST_PASS("f64_where single element not masked = 0.0");
    } else {
        TEST_FAIL("f64_where single element not masked", "Expected 0.0, got %f", result_single_off);
    }
}

// ============================================================================
// Test 2: Correctness with Known Values
// ============================================================================

void test_correctness() {
    printf("\n=== Test 2: Correctness with Known Values ===\n");

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};

    // Test: mask [1, 0, 1, 0, 1] -> sum of indices 0, 2, 4 = 1+3+5 = 9.0
    int mask_odd_indices[] = {1, 0, 1, 0, 1};
    double result1 = fp_reduce_add_f64_where(data, mask_odd_indices, 5);
    if (fabs(result1 - 9.0) < EPSILON) {
        TEST_PASS("f64_where [1,0,1,0,1] = 9.0 (1+3+5)");
    } else {
        TEST_FAIL("f64_where [1,0,1,0,1]", "Expected 9.0, got %f", result1);
    }

    // Test: mask [0, 1, 0, 1, 0] -> sum of indices 1, 3 = 2+4 = 6.0
    int mask_even_indices[] = {0, 1, 0, 1, 0};
    double result2 = fp_reduce_add_f64_where(data, mask_even_indices, 5);
    if (fabs(result2 - 6.0) < EPSILON) {
        TEST_PASS("f64_where [0,1,0,1,0] = 6.0 (2+4)");
    } else {
        TEST_FAIL("f64_where [0,1,0,1,0]", "Expected 6.0, got %f", result2);
    }

    // Test: first element only
    int mask_first[] = {1, 0, 0, 0, 0};
    double result3 = fp_reduce_add_f64_where(data, mask_first, 5);
    if (fabs(result3 - 1.0) < EPSILON) {
        TEST_PASS("f64_where first only = 1.0");
    } else {
        TEST_FAIL("f64_where first only", "Expected 1.0, got %f", result3);
    }

    // Test: last element only
    int mask_last[] = {0, 0, 0, 0, 1};
    double result4 = fp_reduce_add_f64_where(data, mask_last, 5);
    if (fabs(result4 - 5.0) < EPSILON) {
        TEST_PASS("f64_where last only = 5.0");
    } else {
        TEST_FAIL("f64_where last only", "Expected 5.0, got %f", result4);
    }

    // Test with floating point values
    double data_fp[] = {1.5, 2.5, 3.5, 4.5};
    int mask_fp[] = {1, 0, 1, 0};
    double result_fp = fp_reduce_add_f64_where(data_fp, mask_fp, 4);
    if (fabs(result_fp - 5.0) < EPSILON) {  // 1.5 + 3.5 = 5.0
        TEST_PASS("f64_where floating point = 5.0 (1.5+3.5)");
    } else {
        TEST_FAIL("f64_where floating point", "Expected 5.0, got %f", result_fp);
    }
}

// ============================================================================
// Test 3: Consistency with fp_reduce_add_f64
// ============================================================================

void test_consistency() {
    printf("\n=== Test 3: Consistency with fp_reduce_add_f64 ===\n");

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    int mask_all[] = {1, 1, 1, 1, 1, 1, 1, 1};

    double result_where = fp_reduce_add_f64_where(data, mask_all, 8);
    double result_full = fp_reduce_add_f64(data, 8);

    if (fabs(result_where - result_full) < EPSILON) {
        TEST_PASS("f64_where (all ones) == fp_reduce_add_f64");
    } else {
        TEST_FAIL("Consistency", "f64_where=%f, fp_reduce_add_f64=%f", result_where, result_full);
    }

    // Test with negative values
    double data_neg[] = {-1.0, 2.0, -3.0, 4.0, -5.0};
    int mask_neg[] = {1, 1, 1, 1, 1};

    double result_neg_where = fp_reduce_add_f64_where(data_neg, mask_neg, 5);
    double result_neg_full = fp_reduce_add_f64(data_neg, 5);

    if (fabs(result_neg_where - result_neg_full) < EPSILON) {
        TEST_PASS("f64_where with negatives == fp_reduce_add_f64");
    } else {
        TEST_FAIL("Consistency negatives", "f64_where=%f, fp_reduce_add_f64=%f",
                  result_neg_where, result_neg_full);
    }
}

// ============================================================================
// Test 4: Larger Arrays (>16 elements to exercise potential SIMD paths)
// ============================================================================

void test_larger_arrays() {
    printf("\n=== Test 4: Larger Arrays ===\n");

    // Create array of 32 elements: [1, 2, 3, ..., 32]
    double data[32];
    int mask_all[32];
    int mask_even[32];  // Mask even indices (0, 2, 4, ...)

    for (int i = 0; i < 32; i++) {
        data[i] = (double)(i + 1);
        mask_all[i] = 1;
        mask_even[i] = (i % 2 == 0) ? 1 : 0;
    }

    // Sum of 1+2+...+32 = 32*33/2 = 528
    double result_all = fp_reduce_add_f64_where(data, mask_all, 32);
    if (fabs(result_all - 528.0) < EPSILON) {
        TEST_PASS("f64_where 32 elements all masked = 528.0");
    } else {
        TEST_FAIL("f64_where 32 elements", "Expected 528.0, got %f", result_all);
    }

    // Sum of even indices: 1+3+5+...+31 (odd numbers at even indices)
    // These are: 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31
    // = 16 numbers, sum = 16^2 = 256
    double result_even = fp_reduce_add_f64_where(data, mask_even, 32);
    if (fabs(result_even - 256.0) < EPSILON) {
        TEST_PASS("f64_where 32 elements even indices = 256.0");
    } else {
        TEST_FAIL("f64_where 32 elements even", "Expected 256.0, got %f", result_even);
    }

    // Test with 100 elements
    double large_data[100];
    int large_mask[100];
    double expected_sum = 0.0;

    for (int i = 0; i < 100; i++) {
        large_data[i] = (double)(i + 1);
        large_mask[i] = (i % 3 == 0) ? 1 : 0;  // Every 3rd element
        if (large_mask[i]) {
            expected_sum += large_data[i];
        }
    }

    double result_large = fp_reduce_add_f64_where(large_data, large_mask, 100);
    if (fabs(result_large - expected_sum) < EPSILON) {
        TEST_PASS("f64_where 100 elements every 3rd");
    } else {
        TEST_FAIL("f64_where 100 elements", "Expected %f, got %f", expected_sum, result_large);
    }
}

// ============================================================================
// Test 5: Non-Boolean Mask Values (any non-zero should be treated as true)
// ============================================================================

void test_nonboolean_masks() {
    printf("\n=== Test 5: Non-Boolean Mask Values ===\n");

    double data[] = {10.0, 20.0, 30.0, 40.0};

    // Test with various non-zero values
    int mask_various[] = {-1, 0, 42, 0};  // -1 and 42 should both be "true"
    double result = fp_reduce_add_f64_where(data, mask_various, 4);
    // Expected: 10.0 + 30.0 = 40.0
    if (fabs(result - 40.0) < EPSILON) {
        TEST_PASS("f64_where non-boolean masks [-1,0,42,0] = 40.0");
    } else {
        TEST_FAIL("f64_where non-boolean", "Expected 40.0, got %f", result);
    }

    // Test with negative mask values only
    int mask_negative[] = {-1, -2, -3, -4};
    double result_neg = fp_reduce_add_f64_where(data, mask_negative, 4);
    // All should be treated as true: 10+20+30+40 = 100
    if (fabs(result_neg - 100.0) < EPSILON) {
        TEST_PASS("f64_where all negative masks = 100.0");
    } else {
        TEST_FAIL("f64_where negative masks", "Expected 100.0, got %f", result_neg);
    }

    // Test with large positive values
    int mask_large[] = {1000000, 0, 0, 999999};
    double result_large = fp_reduce_add_f64_where(data, mask_large, 4);
    // Expected: 10.0 + 40.0 = 50.0
    if (fabs(result_large - 50.0) < EPSILON) {
        TEST_PASS("f64_where large mask values = 50.0");
    } else {
        TEST_FAIL("f64_where large masks", "Expected 50.0, got %f", result_large);
    }
}

// ============================================================================
// Test 6: Register Preservation (Multiple Consecutive Calls)
// ============================================================================

void test_register_preservation() {
    printf("\n=== Test 6: Register Preservation ===\n");

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    int mask[] = {1, 0, 1, 0, 1};

    // Call multiple times - results should be consistent
    double result1 = fp_reduce_add_f64_where(data, mask, 5);
    double result2 = fp_reduce_add_f64_where(data, mask, 5);
    double result3 = fp_reduce_add_f64_where(data, mask, 5);

    if (fabs(result1 - 9.0) < EPSILON &&
        fabs(result2 - 9.0) < EPSILON &&
        fabs(result3 - 9.0) < EPSILON) {
        TEST_PASS("f64_where multiple calls return consistent results");
    } else {
        TEST_FAIL("Register preservation", "Results: %f, %f, %f (expected all 9.0)",
                  result1, result2, result3);
    }
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=======================================================\n");
    printf(" Conditional Reduction Test Suite\n");
    printf(" Function: fp_reduce_add_f64_where()\n");
    printf(" Model: claude-sonnet-4-5-20250929\n");
    printf(" Date: 2025-11-23\n");
    printf("=======================================================\n");

    test_edge_cases();
    test_correctness();
    test_consistency();
    test_larger_arrays();
    test_nonboolean_masks();
    test_register_preservation();

    printf("\n=======================================================\n");
    if (failures == 0) {
        printf(" ALL TESTS PASSED!\n");
        printf("=======================================================\n");
        return 0;
    } else {
        printf(" FAILED: %d test(s)\n", failures);
        printf("=======================================================\n");
        return 1;
    }
}
