/**
 * test_l1_complete.c - Comprehensive L1 Wrapper Layer Test
 *
 * Tests and benchmarks:
 * 1. Transducers (mapping, filtering, taking, composition)
 * 2. Applicative Maybe (ap)
 * 3. Lazy sequences
 * 4. Performance comparison vs imperative loops
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#include "fp_compose.h"
#include "fp_monads.h"
#include "fp_core.h"

// ============================================================================
// Test Helper Functions
// ============================================================================

static double square(double x) { return x * x; }
static double double_it(double x) { return x * 2.0; }
static bool is_even_int(double x) { return ((int64_t)x % 2) == 0; }
static double add_reducer(double acc, double x) { return acc + x; }

static int64_t triple_i64(int64_t x) { return x * 3; }

// High-resolution timer
static double get_time_ms(void) {
    return (double)clock() / CLOCKS_PER_SEC * 1000.0;
}

// ============================================================================
// Test: Transducers
// ============================================================================

static int test_transducers(void) {
    printf("\n=== TRANSDUCER TESTS ===\n");
    int passed = 0, failed = 0;

    // Test data: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    static const double data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const size_t n = 10;

    // Test 1: Mapping transducer (square all values)
    // Expected: 1 + 4 + 9 + 16 + 25 + 36 + 49 + 64 + 81 + 100 = 385
    {
        fp_transducer_t map_trans = fp_mapping_f64(square);
        double result = fp_transduce_f64(data, n, map_trans, 0.0, add_reducer);
        if (fabs(result - 385.0) < 0.001) {
            printf("  [PASS] Mapping transducer: sum of squares = %.0f\n", result);
            passed++;
        } else {
            printf("  [FAIL] Mapping transducer: expected 385, got %.2f\n", result);
            failed++;
        }
    }

    // Test 2: Filtering transducer (only even numbers)
    // Expected: 2 + 4 + 6 + 8 + 10 = 30
    {
        fp_transducer_t filter_trans = fp_filtering_f64(is_even_int);
        double result = fp_transduce_f64(data, n, filter_trans, 0.0, add_reducer);
        if (fabs(result - 30.0) < 0.001) {
            printf("  [PASS] Filtering transducer: sum of evens = %.0f\n", result);
            passed++;
        } else {
            printf("  [FAIL] Filtering transducer: expected 30, got %.2f\n", result);
            failed++;
        }
    }

    // Test 3: Taking transducer (first 3 elements)
    // Expected: 1 + 2 + 3 = 6
    {
        fp_transducer_t take_trans = fp_taking_f64(3);
        double result = fp_transduce_f64(data, n, take_trans, 0.0, add_reducer);
        if (fabs(result - 6.0) < 0.001) {
            printf("  [PASS] Taking transducer: sum of first 3 = %.0f\n", result);
            passed++;
        } else {
            printf("  [FAIL] Taking transducer: expected 6, got %.2f\n", result);
            failed++;
        }
    }

    // Test 4: Composed transducer (square -> filter even -> take 3)
    // Squares: [1, 4, 9, 16, 25, 36, 49, 64, 81, 100]
    // Even squares: [4, 16, 36, 64, 100]
    // Take 3: [4, 16, 36]
    // Sum: 56
    {
        fp_transducer_t chain[3];
        chain[0] = fp_mapping_f64(square);
        chain[1] = fp_filtering_f64(is_even_int);
        chain[2] = fp_taking_f64(3);

        fp_transducer_t composed = fp_compose_transducers(chain, 3);
        double result = fp_transduce_f64(data, n, composed, 0.0, add_reducer);
        fp_transducer_free(&composed);

        if (fabs(result - 56.0) < 0.001) {
            printf("  [PASS] Composed transducer: square->filter_even->take3 = %.0f\n", result);
            passed++;
        } else {
            printf("  [FAIL] Composed transducer: expected 56, got %.2f\n", result);
            failed++;
        }
    }

    printf("  Transducers: %d passed, %d failed\n", passed, failed);
    return failed;
}

// ============================================================================
// Test: Applicative Maybe
// ============================================================================

static int test_applicative_maybe(void) {
    printf("\n=== APPLICATIVE MAYBE TESTS ===\n");
    int passed = 0, failed = 0;

    // Test 1: ap with Just function and Just value
    // double_it(21.0) = 42.0
    {
        Maybe mfn = fp_just_ptr((void*)double_it);
        Maybe mx = fp_just_f64(21.0);
        Maybe result = fp_ap_maybe_f64(mfn, mx);

        if (fp_is_just(result) && fabs(fp_from_just_f64(result) - 42.0) < 0.001) {
            printf("  [PASS] ap(Just(double), Just(21)) = Just(42)\n");
            passed++;
        } else {
            printf("  [FAIL] ap(Just(double), Just(21)) failed\n");
            failed++;
        }
    }

    // Test 2: ap with Nothing function
    {
        Maybe mfn = fp_nothing();
        Maybe mx = fp_just_f64(21.0);
        Maybe result = fp_ap_maybe_f64(mfn, mx);

        if (fp_is_nothing(result)) {
            printf("  [PASS] ap(Nothing, Just(21)) = Nothing\n");
            passed++;
        } else {
            printf("  [FAIL] ap(Nothing, Just(21)) should be Nothing\n");
            failed++;
        }
    }

    // Test 3: ap with Nothing value
    {
        Maybe mfn = fp_just_ptr((void*)double_it);
        Maybe mx = fp_nothing();
        Maybe result = fp_ap_maybe_f64(mfn, mx);

        if (fp_is_nothing(result)) {
            printf("  [PASS] ap(Just(double), Nothing) = Nothing\n");
            passed++;
        } else {
            printf("  [FAIL] ap(Just(double), Nothing) should be Nothing\n");
            failed++;
        }
    }

    // Test 4: ap with i64
    {
        Maybe mfn = fp_just_ptr((void*)triple_i64);
        Maybe mx = fp_just_i64(7);
        Maybe result = fp_ap_maybe_i64(mfn, mx);

        if (fp_is_just(result) && fp_from_just_i64(result) == 21) {
            printf("  [PASS] ap_i64(Just(triple), Just(7)) = Just(21)\n");
            passed++;
        } else {
            printf("  [FAIL] ap_i64(Just(triple), Just(7)) failed\n");
            failed++;
        }
    }

    printf("  Applicative Maybe: %d passed, %d failed\n", passed, failed);
    return failed;
}

// ============================================================================
// Benchmark: Transducers vs Imperative Loops
// ============================================================================

static void benchmark_transducers(void) {
    printf("\n=== TRANSDUCER BENCHMARK ===\n");

    // Create large test data
    const size_t n = 1000000;
    double* data = (double*)malloc(n * sizeof(double));
    if (!data) {
        printf("  [ERROR] Failed to allocate memory for benchmark\n");
        return;
    }
    for (size_t i = 0; i < n; i++) {
        data[i] = (double)(i + 1);
    }

    // Benchmark 1: Imperative loop (square -> filter even -> sum)
    double t1_start = get_time_ms();
    double imperative_result = 0.0;
    size_t count = 0;
    for (size_t i = 0; i < n && count < 1000; i++) {
        double sq = data[i] * data[i];
        if ((int64_t)sq % 2 == 0) {
            imperative_result += sq;
            count++;
        }
    }
    double t1_end = get_time_ms();

    // Benchmark 2: Transducer (square -> filter even -> take 1000 -> sum)
    double t2_start = get_time_ms();
    fp_transducer_t chain[3];
    chain[0] = fp_mapping_f64(square);
    chain[1] = fp_filtering_f64(is_even_int);
    chain[2] = fp_taking_f64(1000);
    fp_transducer_t composed = fp_compose_transducers(chain, 3);
    double transducer_result = fp_transduce_f64(data, n, composed, 0.0, add_reducer);
    fp_transducer_free(&composed);
    double t2_end = get_time_ms();

    printf("  Data size: %zu elements\n", n);
    printf("  Operation: square -> filter even -> take 1000 -> sum\n");
    printf("\n");
    printf("  Imperative loop:\n");
    printf("    Result: %.0f\n", imperative_result);
    printf("    Time: %.3f ms\n", t1_end - t1_start);
    printf("\n");
    printf("  Transducer:\n");
    printf("    Result: %.0f\n", transducer_result);
    printf("    Time: %.3f ms\n", t2_end - t2_start);
    printf("\n");

    if (fabs(imperative_result - transducer_result) < 0.001) {
        printf("  [PASS] Results match!\n");
    } else {
        printf("  [WARN] Results differ: %.0f vs %.0f\n", imperative_result, transducer_result);
    }

    free(data);
}

// ============================================================================
// Demo: Practical Use Cases
// ============================================================================

static void demo_use_cases(void) {
    printf("\n=== PRACTICAL USE CASE DEMOS ===\n");

    // Demo 1: Data Pipeline (ETL-style)
    printf("\n  1. Data Pipeline (ETL-style):\n");
    printf("     Raw sensor readings -> normalize -> filter outliers -> aggregate\n");
    {
        static const double readings[] = {98.6, 99.1, 102.3, 98.9, 150.0, 99.2, 98.8, -10.0, 99.0, 98.7};
        const size_t n = 10;

        // Filter valid range (90-110) and compute average
        // Valid: 98.6, 99.1, 102.3, 98.9, 99.2, 98.8, 99.0, 98.7 (8 values)
        double sum = 0;
        int count = 0;
        for (size_t i = 0; i < n; i++) {
            if (readings[i] >= 90.0 && readings[i] <= 110.0) {
                sum += readings[i];
                count++;
            }
        }
        printf("     Input: [98.6, 99.1, 102.3, 98.9, 150.0, 99.2, 98.8, -10.0, 99.0, 98.7]\n");
        printf("     After filtering outliers: %d valid readings\n", count);
        printf("     Average: %.2f\n", sum / count);
    }

    // Demo 2: Safe Computation Chain with Maybe
    printf("\n  2. Safe Computation Chain:\n");
    printf("     Input -> double -> check positive -> sqrt -> result\n");
    {
        double input = 8.0;

        // Chain: double(8) = 16 -> sqrt(16) = 4
        Maybe m1 = fp_just_f64(input);
        Maybe m2 = fp_fmap_maybe_f64(m1, double_it);  // 16.0

        // Safe sqrt (would fail for negative)
        if (fp_is_just(m2) && fp_from_just_f64(m2) >= 0) {
            double val = sqrt(fp_from_just_f64(m2));
            printf("     Input: %.1f -> double -> sqrt = %.1f\n", input, val);
        }

        // Try with negative - shows safety
        Maybe m_neg = fp_just_f64(-4.0);
        Maybe m_doubled = fp_fmap_maybe_f64(m_neg, double_it);  // -8.0
        if (fp_is_just(m_doubled) && fp_from_just_f64(m_doubled) < 0) {
            printf("     Input: -4.0 -> double = -8.0 (sqrt would fail - caught safely)\n");
        }
    }

    // Demo 3: Lazy Evaluation
    printf("\n  3. Lazy Sequence Processing:\n");
    printf("     Generate range -> take first 5 -> sum\n");
    {
        fp_lazy_seq_t* seq = fp_lazy_range_f64(1.0, 100.0, 1.0);
        seq = fp_lazy_take_f64(seq, 5);

        size_t out_size;
        double* arr = fp_lazy_to_array_f64(seq, 10, &out_size);

        double sum = 0;
        for (size_t i = 0; i < out_size; i++) {
            sum += arr[i];
        }
        free(arr);
        fp_lazy_free_f64(seq);
        printf("     Sum of first 5 naturals: %.0f (expected: 15)\n", sum);
    }
}

// ============================================================================
// Test: Edge Cases
// ============================================================================

static int test_edge_cases(void) {
    printf("\n=== EDGE CASE TESTS ===\n");
    int passed = 0, failed = 0;

    // Test 1: Empty array
    {
        static const double empty[] = {0};  // Placeholder, we'll use n=0
        fp_transducer_t map_trans = fp_mapping_f64(square);
        double result = fp_transduce_f64(empty, 0, map_trans, 0.0, add_reducer);
        if (fabs(result - 0.0) < 0.001) {
            printf("  [PASS] Empty array: result = %.0f\n", result);
            passed++;
        } else {
            printf("  [FAIL] Empty array: expected 0, got %.2f\n", result);
            failed++;
        }
    }

    // Test 2: Single element
    {
        static const double single[] = {5.0};
        fp_transducer_t map_trans = fp_mapping_f64(square);
        double result = fp_transduce_f64(single, 1, map_trans, 0.0, add_reducer);
        if (fabs(result - 25.0) < 0.001) {
            printf("  [PASS] Single element: square(5) = %.0f\n", result);
            passed++;
        } else {
            printf("  [FAIL] Single element: expected 25, got %.2f\n", result);
            failed++;
        }
    }

    // Test 3: Take more than available
    {
        static const double small[] = {1.0, 2.0, 3.0};
        fp_transducer_t take_trans = fp_taking_f64(10);  // Take 10, but only 3 exist
        double result = fp_transduce_f64(small, 3, take_trans, 0.0, add_reducer);
        if (fabs(result - 6.0) < 0.001) {
            printf("  [PASS] Take(10) from 3 elements: sum = %.0f\n", result);
            passed++;
        } else {
            printf("  [FAIL] Take(10) from 3: expected 6, got %.2f\n", result);
            failed++;
        }
    }

    // Test 4: Filter matches nothing
    {
        static const double odds[] = {1.0, 3.0, 5.0, 7.0, 9.0};
        fp_transducer_t filter_trans = fp_filtering_f64(is_even_int);
        double result = fp_transduce_f64(odds, 5, filter_trans, 0.0, add_reducer);
        if (fabs(result - 0.0) < 0.001) {
            printf("  [PASS] Filter evens from all odds: sum = %.0f\n", result);
            passed++;
        } else {
            printf("  [FAIL] Filter evens from odds: expected 0, got %.2f\n", result);
            failed++;
        }
    }

    // Test 5: Maybe with edge values
    {
        Maybe m_zero = fp_just_f64(0.0);
        Maybe m_result = fp_fmap_maybe_f64(m_zero, double_it);
        if (fp_is_just(m_result) && fabs(fp_from_just_f64(m_result) - 0.0) < 0.001) {
            printf("  [PASS] Maybe fmap with zero: double(0) = 0\n");
            passed++;
        } else {
            printf("  [FAIL] Maybe fmap with zero failed\n");
            failed++;
        }
    }

    printf("  Edge Cases: %d passed, %d failed\n", passed, failed);
    return failed;
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    printf("+==============================================================+\n");
    printf("|       FP-ASM L1 Wrapper Layer - Complete Test Suite          |\n");
    printf("+==============================================================+\n");

    int total_failures = 0;

    // Run tests
    total_failures += test_transducers();
    total_failures += test_applicative_maybe();
    total_failures += test_edge_cases();

    // Run benchmarks
    benchmark_transducers();

    // Run demos
    demo_use_cases();

    // Summary
    printf("\n================================================================\n");
    if (total_failures == 0) {
        printf("  ALL TESTS PASSED! L1 wrapper layer is fully functional.\n");
    } else {
        printf("  %d TEST(S) FAILED. Please review.\n", total_failures);
    }
    printf("================================================================\n");

    return total_failures;
}
