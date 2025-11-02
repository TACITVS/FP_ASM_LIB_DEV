/*
 * demo_descriptive_stats.c
 *
 * Test suite and benchmark for Algorithm #1: Descriptive Statistics Suite
 * Tests correctness and performance of fp_descriptive_stats_f64
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include "../include/fp_core.h"

// Tolerance for floating-point comparisons
#define EPSILON 1e-9

// Naive C implementation for comparison
typedef struct {
    double mean;
    double variance;
    double std_dev;
    double skewness;
    double kurtosis;
} Stats_C;

void descriptive_stats_naive(const double* data, size_t n, Stats_C* stats) {
    if (n == 0) {
        stats->mean = NAN;
        stats->variance = NAN;
        stats->std_dev = NAN;
        stats->skewness = NAN;
        stats->kurtosis = NAN;
        return;
    }

    // Pass 1: Calculate mean
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum += data[i];
    }
    double mean = sum / n;

    // Pass 2: Calculate variance
    double sum_sq_diff = 0.0;
    for (size_t i = 0; i < n; i++) {
        double diff = data[i] - mean;
        sum_sq_diff += diff * diff;
    }
    double variance = sum_sq_diff / n;
    double std_dev = sqrt(variance);

    // Pass 3: Calculate skewness and kurtosis
    double sum_cube = 0.0;
    double sum_quad = 0.0;
    for (size_t i = 0; i < n; i++) {
        double diff = data[i] - mean;
        double diff_sq = diff * diff;
        sum_cube += diff * diff_sq;
        sum_quad += diff_sq * diff_sq;
    }

    double skewness = (sum_cube / n) / (variance * std_dev);
    double kurtosis = (sum_quad / n) / (variance * variance);

    stats->mean = mean;
    stats->variance = variance;
    stats->std_dev = std_dev;
    stats->skewness = skewness;
    stats->kurtosis = kurtosis;
}

// Test helper
bool compare_stats(const DescriptiveStats* fp_stats, const Stats_C* c_stats, const char* test_name) {
    bool pass = true;

    printf("  Testing %s:\n", test_name);

    if (fabs(fp_stats->mean - c_stats->mean) > EPSILON) {
        printf("    FAIL: mean mismatch: FP=%.10f, C=%.10f\n", fp_stats->mean, c_stats->mean);
        pass = false;
    }

    if (fabs(fp_stats->variance - c_stats->variance) > EPSILON) {
        printf("    FAIL: variance mismatch: FP=%.10f, C=%.10f\n", fp_stats->variance, c_stats->variance);
        pass = false;
    }

    if (fabs(fp_stats->std_dev - c_stats->std_dev) > EPSILON) {
        printf("    FAIL: std_dev mismatch: FP=%.10f, C=%.10f\n", fp_stats->std_dev, c_stats->std_dev);
        pass = false;
    }

    if (fabs(fp_stats->skewness - c_stats->skewness) > EPSILON) {
        printf("    FAIL: skewness mismatch: FP=%.10f, C=%.10f\n", fp_stats->skewness, c_stats->skewness);
        pass = false;
    }

    if (fabs(fp_stats->kurtosis - c_stats->kurtosis) > EPSILON) {
        printf("    FAIL: kurtosis mismatch: FP=%.10f, C=%.10f\n", fp_stats->kurtosis, c_stats->kurtosis);
        pass = false;
    }

    if (pass) {
        printf("    PASS: mean=%.4f, var=%.4f, std=%.4f, skew=%.4f, kurt=%.4f\n",
               fp_stats->mean, fp_stats->variance, fp_stats->std_dev,
               fp_stats->skewness, fp_stats->kurtosis);
    }

    return pass;
}

// ============================================================================
// Correctness Tests
// ============================================================================

bool test_simple_sequence() {
    printf("\n=== Test 1: Simple Sequence [1, 2, 3, 4, 5] ===\n");

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    size_t n = 5;

    DescriptiveStats fp_stats;
    Stats_C c_stats;

    fp_descriptive_stats_f64(data, n, &fp_stats);
    descriptive_stats_naive(data, n, &c_stats);

    // Known values: mean=3.0, variance=2.0, std_dev=sqrt(2)
    printf("  Expected: mean=3.0, variance=2.0\n");

    return compare_stats(&fp_stats, &c_stats, "simple sequence");
}

bool test_uniform_values() {
    printf("\n=== Test 2: Uniform Values [5, 5, 5, 5, 5] ===\n");

    double data[] = {5.0, 5.0, 5.0, 5.0, 5.0};
    size_t n = 5;

    DescriptiveStats fp_stats;
    Stats_C c_stats;

    fp_descriptive_stats_f64(data, n, &fp_stats);
    descriptive_stats_naive(data, n, &c_stats);

    // Known values: mean=5.0, variance=0.0
    printf("  Expected: mean=5.0, variance=0.0\n");

    return compare_stats(&fp_stats, &c_stats, "uniform values");
}

bool test_negative_values() {
    printf("\n=== Test 3: Negative Values [-2, -1, 0, 1, 2] ===\n");

    double data[] = {-2.0, -1.0, 0.0, 1.0, 2.0};
    size_t n = 5;

    DescriptiveStats fp_stats;
    Stats_C c_stats;

    fp_descriptive_stats_f64(data, n, &fp_stats);
    descriptive_stats_naive(data, n, &c_stats);

    // Known values: mean=0.0, symmetric distribution (skewness=0)
    printf("  Expected: mean=0.0, skewness=0.0 (symmetric)\n");

    return compare_stats(&fp_stats, &c_stats, "negative values");
}

bool test_large_variance() {
    printf("\n=== Test 4: Large Variance [1, 100, 200, 300, 400] ===\n");

    double data[] = {1.0, 100.0, 200.0, 300.0, 400.0};
    size_t n = 5;

    DescriptiveStats fp_stats;
    Stats_C c_stats;

    fp_descriptive_stats_f64(data, n, &fp_stats);
    descriptive_stats_naive(data, n, &c_stats);

    return compare_stats(&fp_stats, &c_stats, "large variance");
}

bool test_normal_distribution() {
    printf("\n=== Test 5: Approximately Normal Distribution ===\n");

    // Simulated normal-ish data
    double data[] = {2.1, 3.5, 4.2, 4.8, 5.0, 5.2, 5.8, 6.5, 7.9};
    size_t n = 9;

    DescriptiveStats fp_stats;
    Stats_C c_stats;

    fp_descriptive_stats_f64(data, n, &fp_stats);
    descriptive_stats_naive(data, n, &c_stats);

    printf("  Expected: near-zero skewness (symmetric-ish)\n");

    return compare_stats(&fp_stats, &c_stats, "normal-ish distribution");
}

bool test_single_element() {
    printf("\n=== Test 6: Single Element [42.0] ===\n");

    double data[] = {42.0};
    size_t n = 1;

    DescriptiveStats fp_stats;
    Stats_C c_stats;

    fp_descriptive_stats_f64(data, n, &fp_stats);
    descriptive_stats_naive(data, n, &c_stats);

    printf("  Expected: mean=42.0, variance=0.0\n");

    return compare_stats(&fp_stats, &c_stats, "single element");
}

bool test_empty_array() {
    printf("\n=== Test 7: Empty Array [] ===\n");

    double* data = NULL;
    size_t n = 0;

    DescriptiveStats fp_stats;
    Stats_C c_stats;

    fp_descriptive_stats_f64(data, n, &fp_stats);
    descriptive_stats_naive(data, n, &c_stats);

    printf("  Expected: All NaN\n");

    bool pass = true;
    if (!isnan(fp_stats.mean)) {
        printf("    FAIL: mean should be NaN\n");
        pass = false;
    }
    if (!isnan(fp_stats.variance)) {
        printf("    FAIL: variance should be NaN\n");
        pass = false;
    }

    if (pass) {
        printf("    PASS: All values are NaN as expected\n");
    }

    return pass;
}

// ============================================================================
// Performance Benchmark
// ============================================================================

void benchmark_performance() {
    printf("\n");
    printf("=======================================================================\n");
    printf("=== PERFORMANCE BENCHMARK ===\n");
    printf("=======================================================================\n");

    size_t sizes[] = {100, 1000, 10000, 100000, 1000000};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < num_sizes; s++) {
        size_t n = sizes[s];

        // Allocate and initialize random data
        double* data = (double*)malloc(n * sizeof(double));
        if (!data) {
            printf("ERROR: Failed to allocate memory for n=%zu\n", n);
            continue;
        }

        // Fill with random data
        srand(42); // Fixed seed for reproducibility
        for (size_t i = 0; i < n; i++) {
            data[i] = (double)(rand() % 10000) / 100.0; // Range: 0.0 to 100.0
        }

        printf("\nArray size: %zu elements\n", n);

        // Benchmark FP-ASM version
        DescriptiveStats fp_stats;
        clock_t start = clock();
        int iterations = (n >= 100000) ? 100 : 1000;
        for (int i = 0; i < iterations; i++) {
            fp_descriptive_stats_f64(data, n, &fp_stats);
        }
        clock_t end = clock();
        double fp_time = (double)(end - start) / CLOCKS_PER_SEC;

        // Benchmark naive C version
        Stats_C c_stats;
        start = clock();
        for (int i = 0; i < iterations; i++) {
            descriptive_stats_naive(data, n, &c_stats);
        }
        end = clock();
        double c_time = (double)(end - start) / CLOCKS_PER_SEC;

        // Calculate speedup
        double speedup = c_time / fp_time;

        printf("  FP-ASM: %.4f seconds (%d iterations)\n", fp_time, iterations);
        printf("  Naive C: %.4f seconds (%d iterations)\n", c_time, iterations);
        printf("  Speedup: %.2fx\n", speedup);

        // Verify results match
        bool match = true;
        if (fabs(fp_stats.mean - c_stats.mean) > EPSILON) match = false;
        if (fabs(fp_stats.variance - c_stats.variance) > EPSILON) match = false;

        if (match) {
            printf("  Verification: PASS (results match)\n");
        } else {
            printf("  Verification: FAIL (results differ!)\n");
        }

        free(data);
    }
}

// ============================================================================
// Real-World Example
// ============================================================================

void example_real_world() {
    printf("\n");
    printf("=======================================================================\n");
    printf("=== REAL-WORLD EXAMPLE: Analyzing Test Scores ===\n");
    printf("=======================================================================\n");

    // Sample test scores from a class of students
    double scores[] = {
        78.5, 82.0, 91.5, 76.0, 88.5, 95.0, 73.5, 84.0, 89.5, 92.0,
        81.5, 77.0, 90.0, 85.5, 79.0, 93.5, 87.0, 74.5, 86.0, 80.5
    };
    size_t n = sizeof(scores) / sizeof(scores[0]);

    printf("\nAnalyzing %zu test scores...\n", n);

    DescriptiveStats stats;
    fp_descriptive_stats_f64(scores, n, &stats);

    printf("\nDescriptive Statistics:\n");
    printf("  Mean:       %.2f (class average)\n", stats.mean);
    printf("  Std Dev:    %.2f (score spread)\n", stats.std_dev);
    printf("  Variance:   %.2f\n", stats.variance);
    printf("  Skewness:   %.4f ", stats.skewness);

    if (fabs(stats.skewness) < 0.5) {
        printf("(approximately symmetric)\n");
    } else if (stats.skewness > 0) {
        printf("(positively skewed - more low scores)\n");
    } else {
        printf("(negatively skewed - more high scores)\n");
    }

    printf("  Kurtosis:   %.4f ", stats.kurtosis);

    if (fabs(stats.kurtosis - 3.0) < 0.5) {
        printf("(normal distribution)\n");
    } else if (stats.kurtosis > 3.0) {
        printf("(heavy tails - more outliers)\n");
    } else {
        printf("(light tails - fewer outliers)\n");
    }

    // Interpretation
    printf("\nInterpretation:\n");
    printf("  - The class average is %.1f\n", stats.mean);
    printf("  - About 68%% of scores are within %.1f to %.1f\n",
           stats.mean - stats.std_dev, stats.mean + stats.std_dev);
    printf("  - About 95%% of scores are within %.1f to %.1f\n",
           stats.mean - 2*stats.std_dev, stats.mean + 2*stats.std_dev);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=======================================================================\n");
    printf("     Algorithm #1: Descriptive Statistics Suite - Test & Benchmark    \n");
    printf("=======================================================================\n");

    int passed = 0;
    int total = 0;

    // Run correctness tests
    printf("\n### CORRECTNESS TESTS ###\n");

    total++; if (test_simple_sequence()) passed++;
    total++; if (test_uniform_values()) passed++;
    total++; if (test_negative_values()) passed++;
    total++; if (test_large_variance()) passed++;
    total++; if (test_normal_distribution()) passed++;
    total++; if (test_single_element()) passed++;
    total++; if (test_empty_array()) passed++;

    printf("\n-----------------------------------------------------------------------\n");
    printf("Correctness Tests: %d / %d PASSED\n", passed, total);
    printf("-----------------------------------------------------------------------\n");

    if (passed == total) {
        // Run performance benchmark
        benchmark_performance();

        // Run real-world example
        example_real_world();

        printf("\n=======================================================================\n");
        printf("                    ALL TESTS PASSED!                                 \n");
        printf("=======================================================================\n");

        return 0;
    } else {
        printf("\nSome tests failed. Skipping benchmarks.\n");
        return 1;
    }
}
