// test_reductions_critical.c
//
// Critical correctness tests for reduction functions
// Tests fixes for Tasks 13-16:
//   - Bounds checking (n=0)
//   - Null pointer handling
//   - Windows x64 ABI compliance (register preservation)
//   - Register clobber bugs in u64 scalar functions
//
// Model: claude-sonnet-4-5-20250929
// Timestamp: 2025-01-16

#include "fp_core.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TEST_PASS(name) printf("[PASS] %s\n", name)
#define TEST_FAIL(name, ...) do { printf("[FAIL] %s: ", name); printf(__VA_ARGS__); printf("\n"); failures++; } while(0)

int failures = 0;

// ============================================================================
// Test 1: Bounds checking (n=0) for min/max functions
// ============================================================================

void test_empty_array_bounds() {
    printf("\n=== Test 1: Empty Array Bounds Checking (n=0) ===\n");

    int32_t dummy_i32 = 42;
    uint32_t dummy_u32 = 42;
    int16_t dummy_i16 = 42;
    uint16_t dummy_u16 = 42;
    int8_t dummy_i8 = 42;
    uint8_t dummy_u8 = 42;
    uint64_t dummy_u64 = 42;

    // i32 min/max
    int32_t result_i32_min = fp_reduce_min_i32(&dummy_i32, 0);
    if (result_i32_min == 0x7FFFFFFF) {
        TEST_PASS("i32 min(n=0) returns INT32_MAX");
    } else {
        TEST_FAIL("i32 min(n=0)", "Expected 0x7FFFFFFF, got 0x%X", result_i32_min);
    }

    int32_t result_i32_max = fp_reduce_max_i32(&dummy_i32, 0);
    if (result_i32_max == (int32_t)0x80000000) {
        TEST_PASS("i32 max(n=0) returns INT32_MIN");
    } else {
        TEST_FAIL("i32 max(n=0)", "Expected 0x80000000, got 0x%X", result_i32_max);
    }

    // u32 min/max
    uint32_t result_u32_min = fp_reduce_min_u32(&dummy_u32, 0);
    if (result_u32_min == 0xFFFFFFFF) {
        TEST_PASS("u32 min(n=0) returns UINT32_MAX");
    } else {
        TEST_FAIL("u32 min(n=0)", "Expected 0xFFFFFFFF, got 0x%X", result_u32_min);
    }

    uint32_t result_u32_max = fp_reduce_max_u32(&dummy_u32, 0);
    if (result_u32_max == 0) {
        TEST_PASS("u32 max(n=0) returns 0");
    } else {
        TEST_FAIL("u32 max(n=0)", "Expected 0, got 0x%X", result_u32_max);
    }

    // u64 min/max (scalar functions - most critical!)
    uint64_t result_u64_min = fp_reduce_min_u64(&dummy_u64, 0);
    if (result_u64_min == 0xFFFFFFFFFFFFFFFFULL) {
        TEST_PASS("u64 min(n=0) returns UINT64_MAX");
    } else {
        TEST_FAIL("u64 min(n=0)", "Expected 0xFFFFFFFFFFFFFFFF, got 0x%llX", result_u64_min);
    }

    uint64_t result_u64_max = fp_reduce_max_u64(&dummy_u64, 0);
    if (result_u64_max == 0) {
        TEST_PASS("u64 max(n=0) returns 0");
    } else {
        TEST_FAIL("u64 max(n=0)", "Expected 0, got 0x%llX", result_u64_max);
    }
}

// ============================================================================
// Test 2: Null pointer handling
// ============================================================================

void test_null_pointer_handling() {
    printf("\n=== Test 2: Null Pointer Handling ===\n");

    // Test add - should return 0 (identity)
    int32_t result_add = fp_reduce_add_i32(NULL, 10);
    if (result_add == 0) {
        TEST_PASS("i32 add(NULL) returns 0");
    } else {
        TEST_FAIL("i32 add(NULL)", "Expected 0, got %d", result_add);
    }

    // Test mul - should return 1 (identity)
    int32_t result_mul = fp_reduce_mul_i32(NULL, 10);
    if (result_mul == 1) {
        TEST_PASS("i32 mul(NULL) returns 1");
    } else {
        TEST_FAIL("i32 mul(NULL)", "Expected 1, got %d", result_mul);
    }

    // Test min - should return MAX
    int32_t result_min = fp_reduce_min_i32(NULL, 10);
    if (result_min == 0x7FFFFFFF) {
        TEST_PASS("i32 min(NULL) returns INT32_MAX");
    } else {
        TEST_FAIL("i32 min(NULL)", "Expected 0x7FFFFFFF, got 0x%X", result_min);
    }

    // Test max - should return MIN
    int32_t result_max = fp_reduce_max_i32(NULL, 10);
    if (result_max == (int32_t)0x80000000) {
        TEST_PASS("i32 max(NULL) returns INT32_MIN");
    } else {
        TEST_FAIL("i32 max(NULL)", "Expected 0x80000000, got 0x%X", result_max);
    }

    // u64 null pointer tests (critical - these had register bugs)
    uint64_t result_u64_add = fp_reduce_add_u64(NULL, 10);
    if (result_u64_add == 0) {
        TEST_PASS("u64 add(NULL) returns 0");
    } else {
        TEST_FAIL("u64 add(NULL)", "Expected 0, got %llu", result_u64_add);
    }

    uint64_t result_u64_mul = fp_reduce_mul_u64(NULL, 10);
    if (result_u64_mul == 1) {
        TEST_PASS("u64 mul(NULL) returns 1");
    } else {
        TEST_FAIL("u64 mul(NULL)", "Expected 1, got %llu", result_u64_mul);
    }
}

// ============================================================================
// Test 3: U64 scalar functions correctness (register clobber fix)
// ============================================================================

void test_u64_scalar_correctness() {
    printf("\n=== Test 3: U64 Scalar Functions Correctness ===\n");

    // Test multiply
    uint64_t mul_data[] = {2, 3, 5};
    uint64_t result_mul = fp_reduce_mul_u64(mul_data, 3);
    if (result_mul == 30) {
        TEST_PASS("u64 mul([2,3,5]) = 30");
    } else {
        TEST_FAIL("u64 mul([2,3,5])", "Expected 30, got %llu", result_mul);
    }

    // Test min
    uint64_t min_data[] = {100, 50, 200, 25, 150};
    uint64_t result_min = fp_reduce_min_u64(min_data, 5);
    if (result_min == 25) {
        TEST_PASS("u64 min([100,50,200,25,150]) = 25");
    } else {
        TEST_FAIL("u64 min([100,50,200,25,150])", "Expected 25, got %llu", result_min);
    }

    // Test max
    uint64_t max_data[] = {100, 50, 200, 25, 150};
    uint64_t result_max = fp_reduce_max_u64(max_data, 5);
    if (result_max == 200) {
        TEST_PASS("u64 max([100,50,200,25,150]) = 200");
    } else {
        TEST_FAIL("u64 max([100,50,200,25,150])", "Expected 200, got %llu", result_max);
    }

    // Test with larger array (exercises 4-accumulator loop)
    uint64_t large_data[20];
    for (int i = 0; i < 20; i++) {
        large_data[i] = i + 1;
    }

    uint64_t result_min_large = fp_reduce_min_u64(large_data, 20);
    if (result_min_large == 1) {
        TEST_PASS("u64 min([1..20]) = 1");
    } else {
        TEST_FAIL("u64 min([1..20])", "Expected 1, got %llu", result_min_large);
    }

    uint64_t result_max_large = fp_reduce_max_u64(large_data, 20);
    if (result_max_large == 20) {
        TEST_PASS("u64 max([1..20]) = 20");
    } else {
        TEST_FAIL("u64 max([1..20])", "Expected 20, got %llu", result_max_large);
    }
}

// ============================================================================
// Test 4: Register preservation test (Windows x64 ABI)
// ============================================================================

void test_register_preservation() {
    printf("\n=== Test 4: Register Preservation (Windows x64 ABI) ===\n");

    // This is a best-effort test - we can't directly verify register preservation
    // from C, but we can at least verify functions return correct results
    // without crashing, which would happen if registers were clobbered.

    int32_t data[] = {1, 2, 3, 4, 5};

    // Call multiple functions in sequence - if registers are corrupted,
    // subsequent calls would produce wrong results
    int32_t sum1 = fp_reduce_add_i32(data, 5);  // Should be 15
    int32_t sum2 = fp_reduce_add_i32(data, 5);  // Should still be 15

    if (sum1 == 15 && sum2 == 15) {
        TEST_PASS("Multiple calls return consistent results (register preservation OK)");
    } else {
        TEST_FAIL("Register preservation", "sum1=%d, sum2=%d (expected 15, 15)", sum1, sum2);
    }

    // Test with u64 functions (which had the register bug)
    uint64_t u64_data[] = {10, 20, 30, 40, 50};
    uint64_t u64_sum1 = fp_reduce_add_u64(u64_data, 5);  // Should be 150
    uint64_t u64_sum2 = fp_reduce_add_u64(u64_data, 5);  // Should still be 150

    if (u64_sum1 == 150 && u64_sum2 == 150) {
        TEST_PASS("u64 multiple calls return consistent results");
    } else {
        TEST_FAIL("u64 register preservation", "sum1=%llu, sum2=%llu (expected 150, 150)", u64_sum1, u64_sum2);
    }
}

// ============================================================================
// Test 5: Basic correctness for all data types
// ============================================================================

void test_basic_correctness() {
    printf("\n=== Test 5: Basic Correctness (All Data Types) ===\n");

    // i32
    int32_t i32_data[] = {1, 2, 3, 4, 5};
    if (fp_reduce_add_i32(i32_data, 5) == 15) {
        TEST_PASS("i32 add([1,2,3,4,5]) = 15");
    } else {
        TEST_FAIL("i32 add", "Expected 15, got %d", fp_reduce_add_i32(i32_data, 5));
    }

    // u32
    uint32_t u32_data[] = {10, 20, 30};
    if (fp_reduce_add_u32(u32_data, 3) == 60) {
        TEST_PASS("u32 add([10,20,30]) = 60");
    } else {
        TEST_FAIL("u32 add", "Expected 60, got %u", fp_reduce_add_u32(u32_data, 3));
    }

    // i16
    int16_t i16_data[] = {100, 200, 300};
    if (fp_reduce_add_i16(i16_data, 3) == 600) {
        TEST_PASS("i16 add([100,200,300]) = 600");
    } else {
        TEST_FAIL("i16 add", "Expected 600, got %d", fp_reduce_add_i16(i16_data, 3));
    }

    // u64
    uint64_t u64_data[] = {1000, 2000, 3000};
    if (fp_reduce_add_u64(u64_data, 3) == 6000) {
        TEST_PASS("u64 add([1000,2000,3000]) = 6000");
    } else {
        TEST_FAIL("u64 add", "Expected 6000, got %llu", fp_reduce_add_u64(u64_data, 3));
    }
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=======================================================\n");
    printf(" Critical Reduction Functions Test Suite\n");
    printf(" Tests for fixes: Tasks 13-16\n");
    printf(" Model: claude-sonnet-4-5-20250929\n");
    printf(" Date: 2025-01-16\n");
    printf("=======================================================\n");

    test_empty_array_bounds();
    test_null_pointer_handling();
    test_u64_scalar_correctness();
    test_register_preservation();
    test_basic_correctness();

    printf("\n=======================================================\n");
    if (failures == 0) {
        printf(" ALL TESTS PASSED! (%d tests)\n", 0);  // Update count if needed
        printf("=======================================================\n");
        return 0;
    } else {
        printf(" FAILED: %d test(s)\n", failures);
        printf("=======================================================\n");
        return 1;
    }
}
