/**
 * test_fp_stats_template.c - Comprehensive tests for Pattern 1
 *
 * Tests all statistical functions in fp_stats_template.c
 *
 * Compile:
 *   gcc test_fp_stats_template.c -I../../include -o test_fp_stats.exe -lm -O3
 *
 * Run:
 *   ./test_fp_stats.exe
 */

#include "../../src/wrappers/fp_stats_template.c"
#include <stdio.h>
#include <assert.h>
#include <float.h>

#define TOLERANCE 1e-9
#define ASSERT_NEAR(a, b, tol) assert(fabs((a) - (b)) < (tol))

int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    printf("Testing: %s ... ", name); \
    fflush(stdout);

#define PASS() \
    printf("✓ PASS\n"); \
    tests_passed++;

#define FAIL(msg) \
    printf("✗ FAIL: %s\n", msg); \
    tests_failed++;

/* ============================================================================
 * Test Data
 * ============================================================================ */

double simple_data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
size_t simple_n = 5;

double zeros[] = {0.0, 0.0, 0.0};
size_t zeros_n = 3;

double constant[] = {7.0, 7.0, 7.0, 7.0};
size_t constant_n = 4;

double negative[] = {-5.0, -3.0, -1.0, 1.0, 3.0, 5.0};
size_t negative_n = 6;

/* ============================================================================
 * Basic Statistics Tests
 * ============================================================================ */

void test_mean() {
    TEST("fp_mean");

    // Simple data: mean = (1+2+3+4+5)/5 = 3.0
    double mean = fp_mean(simple_data, simple_n);
    ASSERT_NEAR(mean, 3.0, TOLERANCE);

    // Zeros: mean = 0.0
    mean = fp_mean(zeros, zeros_n);
    ASSERT_NEAR(mean, 0.0, TOLERANCE);

    // Constant: mean = 7.0
    mean = fp_mean(constant, constant_n);
    ASSERT_NEAR(mean, 7.0, TOLERANCE);

    // Empty array
    mean = fp_mean(NULL, 0);
    ASSERT_NEAR(mean, 0.0, TOLERANCE);

    PASS();
}

void test_variance() {
    TEST("fp_variance");

    double mean = fp_mean(simple_data, simple_n);
    double var = fp_variance(simple_data, simple_n, mean);

    // Variance = sum((x-3)²)/5 = ((4+1+0+1+4)/5) = 2.0
    ASSERT_NEAR(var, 2.0, TOLERANCE);

    // Constant array: variance = 0
    mean = fp_mean(constant, constant_n);
    var = fp_variance(constant, constant_n, mean);
    ASSERT_NEAR(var, 0.0, TOLERANCE);

    PASS();
}

void test_std() {
    TEST("fp_std");

    double mean = fp_mean(simple_data, simple_n);
    double std = fp_std(simple_data, simple_n, mean);

    // Std = sqrt(2.0) = 1.414...
    ASSERT_NEAR(std, sqrt(2.0), TOLERANCE);

    PASS();
}

void test_mean_variance_welford() {
    TEST("fp_mean_variance_welford");

    MeanVarianceResult result = fp_mean_variance_welford(simple_data, simple_n);

    ASSERT_NEAR(result.mean, 3.0, TOLERANCE);
    ASSERT_NEAR(result.variance, 2.0, TOLERANCE);
    assert(result.count == simple_n);

    // Test on constant array
    result = fp_mean_variance_welford(constant, constant_n);
    ASSERT_NEAR(result.mean, 7.0, TOLERANCE);
    ASSERT_NEAR(result.variance, 0.0, TOLERANCE);

    PASS();
}

void test_min_max() {
    TEST("fp_min_max");

    MinMaxResult mm = fp_min_max(simple_data, simple_n);

    ASSERT_NEAR(mm.min, 1.0, TOLERANCE);
    ASSERT_NEAR(mm.max, 5.0, TOLERANCE);

    // Test with negative numbers
    mm = fp_min_max(negative, negative_n);
    ASSERT_NEAR(mm.min, -5.0, TOLERANCE);
    ASSERT_NEAR(mm.max, 5.0, TOLERANCE);

    PASS();
}

/* ============================================================================
 * Pairwise Statistics Tests
 * ============================================================================ */

void test_covariance() {
    TEST("fp_covariance");

    double x[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double y[] = {2.0, 4.0, 6.0, 8.0, 10.0};  // y = 2*x
    size_t n = 5;

    double mean_x = fp_mean(x, n);  // 3.0
    double mean_y = fp_mean(y, n);  // 6.0

    double cov = fp_covariance(x, y, n, mean_x, mean_y);

    // cov(X, 2X) = 2 * var(X) = 2 * 2.0 = 4.0
    ASSERT_NEAR(cov, 4.0, TOLERANCE);

    PASS();
}

void test_correlation() {
    TEST("fp_correlation");

    double x[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double y[] = {2.0, 4.0, 6.0, 8.0, 10.0};  // Perfect positive correlation
    size_t n = 5;

    Maybe corr = fp_correlation(x, y, n);

    assert(fp_is_just_inline(corr));
    ASSERT_NEAR(fp_from_just_f64(corr), 1.0, TOLERANCE);  // Perfect correlation

    // Test uncorrelated data
    double z[] = {5.0, 1.0, 3.0, 2.0, 4.0};  // Random order
    corr = fp_correlation(x, z, n);

    assert(fp_is_just_inline(corr));
    // Correlation should be between -1 and 1
    double corr_val = fp_from_just_f64(corr);
    assert(corr_val >= -1.0 && corr_val <= 1.0);

    PASS();
}

/* ============================================================================
 * Vector Norms & Distances Tests
 * ============================================================================ */

void test_l2_norm() {
    TEST("fp_l2_norm");

    double v[] = {3.0, 4.0};  // 3-4-5 triangle
    double norm = fp_l2_norm(v, 2);

    ASSERT_NEAR(norm, 5.0, TOLERANCE);

    // Unit vector
    double unit[] = {1.0, 0.0, 0.0};
    norm = fp_l2_norm(unit, 3);
    ASSERT_NEAR(norm, 1.0, TOLERANCE);

    PASS();
}

void test_l1_norm() {
    TEST("fp_l1_norm");

    double v[] = {3.0, 4.0};
    double norm = fp_l1_norm(v, 2);

    ASSERT_NEAR(norm, 7.0, TOLERANCE);  // |3| + |4| = 7

    // Test with negatives
    double v2[] = {-2.0, 3.0, -4.0};
    norm = fp_l1_norm(v2, 3);
    ASSERT_NEAR(norm, 9.0, TOLERANCE);  // 2 + 3 + 4 = 9

    PASS();
}

void test_euclidean_distance() {
    TEST("fp_euclidean_distance");

    double x[] = {0.0, 0.0};
    double y[] = {3.0, 4.0};

    double dist = fp_euclidean_distance(x, y, 2);

    ASSERT_NEAR(dist, 5.0, TOLERANCE);  // 3-4-5 triangle

    // Distance from point to itself
    dist = fp_euclidean_distance(x, x, 2);
    ASSERT_NEAR(dist, 0.0, TOLERANCE);

    PASS();
}

void test_manhattan_distance() {
    TEST("fp_manhattan_distance");

    double x[] = {0.0, 0.0};
    double y[] = {3.0, 4.0};

    double dist = fp_manhattan_distance(x, y, 2);

    ASSERT_NEAR(dist, 7.0, TOLERANCE);  // |3-0| + |4-0| = 7

    PASS();
}

void test_dot_product() {
    TEST("fp_dot_product");

    double x[] = {1.0, 2.0, 3.0};
    double y[] = {4.0, 5.0, 6.0};

    double dot = fp_dot_product(x, y, 3);

    ASSERT_NEAR(dot, 32.0, TOLERANCE);  // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32

    // Orthogonal vectors
    double a[] = {1.0, 0.0};
    double b[] = {0.0, 1.0};
    dot = fp_dot_product(a, b, 2);
    ASSERT_NEAR(dot, 0.0, TOLERANCE);

    PASS();
}

void test_cosine_similarity() {
    TEST("fp_cosine_similarity");

    double x[] = {1.0, 0.0};
    double y[] = {1.0, 0.0};  // Same direction

    Maybe sim = fp_cosine_similarity(x, y, 2);

    assert(fp_is_just_inline(sim));
    ASSERT_NEAR(fp_from_just_f64(sim), 1.0, TOLERANCE);

    // Opposite direction
    double z[] = {-1.0, 0.0};
    sim = fp_cosine_similarity(x, z, 2);

    assert(fp_is_just_inline(sim));
    ASSERT_NEAR(fp_from_just_f64(sim), -1.0, TOLERANCE);

    // Orthogonal
    double w[] = {0.0, 1.0};
    sim = fp_cosine_similarity(x, w, 2);

    assert(fp_is_just_inline(sim));
    ASSERT_NEAR(fp_from_just_f64(sim), 0.0, TOLERANCE);

    PASS();
}

/* ============================================================================
 * Entropy & Information Theory Tests
 * ============================================================================ */

void test_entropy() {
    TEST("fp_entropy");

    // Uniform distribution (maximum entropy)
    double uniform[] = {0.25, 0.25, 0.25, 0.25};
    Maybe ent = fp_entropy(uniform, 4);

    assert(fp_is_just_inline(ent));
    ASSERT_NEAR(fp_from_just_f64(ent), 2.0, TOLERANCE);  // log2(4) = 2

    // Deterministic (minimum entropy = 0)
    double deterministic[] = {1.0, 0.0, 0.0};
    ent = fp_entropy(deterministic, 3);

    assert(fp_is_just_inline(ent));
    ASSERT_NEAR(fp_from_just_f64(ent), 0.0, TOLERANCE);

    // Invalid probability (negative)
    double invalid[] = {-0.5, 1.5};
    ent = fp_entropy(invalid, 2);
    assert(fp_is_nothing_inline(ent));

    // Invalid probability (doesn't sum to 1)
    double invalid2[] = {0.3, 0.3};
    ent = fp_entropy(invalid2, 2);
    assert(fp_is_nothing_inline(ent));

    PASS();
}

void test_gini_impurity() {
    TEST("fp_gini_impurity");

    // Uniform distribution
    double uniform[] = {0.25, 0.25, 0.25, 0.25};
    Maybe gini = fp_gini_impurity(uniform, 4);

    assert(fp_is_just_inline(gini));
    // Gini = 1 - sum(p²) = 1 - 4*(0.25²) = 1 - 0.25 = 0.75
    ASSERT_NEAR(fp_from_just_f64(gini), 0.75, TOLERANCE);

    // Pure (no impurity)
    double pure[] = {1.0, 0.0};
    gini = fp_gini_impurity(pure, 2);

    assert(fp_is_just_inline(gini));
    ASSERT_NEAR(fp_from_just_f64(gini), 0.0, TOLERANCE);

    PASS();
}

/* ============================================================================
 * Normalization & Scaling Tests
 * ============================================================================ */

void test_normalize_min_max() {
    TEST("fp_normalize_min_max");

    double input[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double output[5];

    Maybe result = fp_normalize_min_max(input, output, 5);

    assert(fp_is_just_inline(result));

    // Should be normalized to [0, 1]
    ASSERT_NEAR(output[0], 0.0, TOLERANCE);   // (1-1)/(5-1) = 0
    ASSERT_NEAR(output[2], 0.5, TOLERANCE);   // (3-1)/(5-1) = 0.5
    ASSERT_NEAR(output[4], 1.0, TOLERANCE);   // (5-1)/(5-1) = 1

    // Test constant array (should return Nothing)
    double constant_input[] = {7.0, 7.0, 7.0};
    result = fp_normalize_min_max(constant_input, output, 3);
    assert(fp_is_nothing_inline(result));

    PASS();
}

void test_standardize() {
    TEST("fp_standardize");

    double input[] = {1.0, 2.0, 3.0, 4.0, 5.0};  // mean=3, std=sqrt(2)
    double output[5];

    Maybe result = fp_standardize(input, output, 5);

    assert(fp_is_just_inline(result));

    // Check mean ≈ 0 and std ≈ 1
    MeanVarianceResult mv = fp_mean_variance_welford(output, 5);
    ASSERT_NEAR(mv.mean, 0.0, 1e-6);
    ASSERT_NEAR(sqrt(mv.variance), 1.0, 1e-6);

    // Test constant array
    double constant_input[] = {7.0, 7.0, 7.0};
    result = fp_standardize(constant_input, output, 3);
    assert(fp_is_nothing_inline(result));

    PASS();
}

/* ============================================================================
 * Summary Statistics Tests
 * ============================================================================ */

void test_summary_stats() {
    TEST("fp_summary_stats");

    SummaryStats stats = fp_summary_stats(simple_data, simple_n);

    ASSERT_NEAR(stats.mean, 3.0, TOLERANCE);
    ASSERT_NEAR(stats.variance, 2.0, TOLERANCE);
    ASSERT_NEAR(stats.std, sqrt(2.0), TOLERANCE);
    ASSERT_NEAR(stats.min, 1.0, TOLERANCE);
    ASSERT_NEAR(stats.max, 5.0, TOLERANCE);
    ASSERT_NEAR(stats.range, 4.0, TOLERANCE);
    assert(stats.count == 5);

    PASS();
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(void) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  FP Stats Template - Comprehensive Test Suite           ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    // Basic statistics
    test_mean();
    test_variance();
    test_std();
    test_mean_variance_welford();
    test_min_max();

    // Pairwise statistics
    test_covariance();
    test_correlation();

    // Vector norms & distances
    test_l2_norm();
    test_l1_norm();
    test_euclidean_distance();
    test_manhattan_distance();
    test_dot_product();
    test_cosine_similarity();

    // Entropy & information theory
    test_entropy();
    test_gini_impurity();

    // Normalization & scaling
    test_normalize_min_max();
    test_standardize();

    // Summary statistics
    test_summary_stats();

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  Test Results                                            ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Passed: %3d                                              ║\n", tests_passed);
    printf("║  Failed: %3d                                              ║\n", tests_failed);
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    if (tests_failed == 0) {
        printf("✓ ALL TESTS PASSED! Pattern 1 is production-ready.\n\n");
        return 0;
    } else {
        printf("✗ SOME TESTS FAILED! Review errors above.\n\n");
        return 1;
    }
}
