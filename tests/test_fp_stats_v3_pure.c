/**
 * test_fp_stats_v3_pure.c - Test Pure FP version (v3) - ZERO user-facing loops!
 *
 * This tests the ultimate FP version with:
 * - No manual loops in API
 * - All operations as reduce/map/fold/recursion
 * - Tail-recursive helpers (compiler optimizes)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fp_stats_v3_pure.h"

#define TOLERANCE 1e-9

int test_count = 0;
int pass_count = 0;

void test_mean_pure() {
    printf("Test %d: fp_mean_pure()\n", ++test_count);

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double result = fp_mean_pure(data, 5);
    double expected = 3.0;

    if (fabs(result - expected) < TOLERANCE) {
        printf("  PASS: mean = %.6f (expected %.6f)\n", result, expected);
        pass_count++;
    } else {
        printf("  FAIL: mean = %.6f (expected %.6f)\n", result, expected);
    }
}

void test_variance_pure() {
    printf("Test %d: fp_variance_pure()\n", ++test_count);

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double mean = fp_mean_pure(data, 5);
    double result = fp_variance_pure(data, 5, mean);
    double expected = 2.0;  // Variance of [1,2,3,4,5] = 2.0

    if (fabs(result - expected) < TOLERANCE) {
        printf("  PASS: variance = %.6f (expected %.6f)\n", result, expected);
        pass_count++;
    } else {
        printf("  FAIL: variance = %.6f (expected %.6f)\n", result, expected);
    }
}

void test_mean_variance_welford() {
    printf("Test %d: fp_mean_variance_pure() - Welford single-pass\n", ++test_count);

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    MeanVarianceResult result = fp_mean_variance_pure(data, 5);

    if (fabs(result.mean - 3.0) < TOLERANCE &&
        fabs(result.variance - 2.0) < TOLERANCE) {
        printf("  PASS: mean=%.6f, variance=%.6f\n", result.mean, result.variance);
        pass_count++;
    } else {
        printf("  FAIL: mean=%.6f, variance=%.6f\n", result.mean, result.variance);
    }
}

void test_euclidean_distance_pure() {
    printf("Test %d: fp_euclidean_distance_pure() - Tail recursive\n", ++test_count);

    double x[] = {1.0, 2.0, 3.0};
    double y[] = {4.0, 5.0, 6.0};
    double result = fp_euclidean_distance_pure(x, y, 3);
    double expected = sqrt(27.0);  // sqrt((3^2 + 3^2 + 3^2)) = sqrt(27)

    if (fabs(result - expected) < TOLERANCE) {
        printf("  PASS: distance = %.6f (expected %.6f)\n", result, expected);
        pass_count++;
    } else {
        printf("  FAIL: distance = %.6f (expected %.6f)\n", result, expected);
    }
}

void test_covariance_pure() {
    printf("Test %d: fp_covariance_pure() - Tail recursive\n", ++test_count);

    double x[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double y[] = {2.0, 4.0, 6.0, 8.0, 10.0};  // y = 2*x
    double mean_x = fp_mean_pure(x, 5);
    double mean_y = fp_mean_pure(y, 5);
    double result = fp_covariance_pure(x, y, 5, mean_x, mean_y);
    double expected = 4.0;  // Cov(x, 2x) = 2*Var(x) = 2*2 = 4

    if (fabs(result - expected) < TOLERANCE) {
        printf("  PASS: covariance = %.6f (expected %.6f)\n", result, expected);
        pass_count++;
    } else {
        printf("  FAIL: covariance = %.6f (expected %.6f)\n", result, expected);
    }
}

void test_normalize_pure() {
    printf("Test %d: fp_normalize_pure() - Tail recursive map\n", ++test_count);

    double input[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double output[5];
    Maybe result = fp_normalize_pure(input, output, 5);

    if (result.tag == FP_JUST) {
        // Normalized [1,2,3,4,5] should be [0, 0.25, 0.5, 0.75, 1.0]
        int ok = 1;
        ok &= fabs(output[0] - 0.0) < TOLERANCE;
        ok &= fabs(output[1] - 0.25) < TOLERANCE;
        ok &= fabs(output[2] - 0.5) < TOLERANCE;
        ok &= fabs(output[3] - 0.75) < TOLERANCE;
        ok &= fabs(output[4] - 1.0) < TOLERANCE;

        if (ok) {
            printf("  PASS: normalized to [0, 0.25, 0.5, 0.75, 1.0]\n");
            pass_count++;
        } else {
            printf("  FAIL: unexpected values\n");
        }
    } else {
        printf("  FAIL: returned Nothing\n");
    }
}

void test_standardize_pure() {
    printf("Test %d: fp_standardize_pure() - Tail recursive map\n", ++test_count);

    double input[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double output[5];
    Maybe result = fp_standardize_pure(input, output, 5);

    if (result.tag == FP_JUST) {
        // Standardized array should have mean=0, std=1
        double mean = fp_mean_pure(output, 5);
        double std = fp_std_pure(output, 5, mean);

        if (fabs(mean) < 1e-9 && fabs(std - 1.0) < 1e-6) {
            printf("  PASS: mean=%.9f, std=%.6f\n", mean, std);
            pass_count++;
        } else {
            printf("  FAIL: mean=%.9f, std=%.6f\n", mean, std);
        }
    } else {
        printf("  FAIL: returned Nothing\n");
    }
}

void test_summary_stats() {
    printf("Test %d: fp_summary_stats_pure() - FP composition\n", ++test_count);

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    SummaryStats stats = fp_summary_stats_pure(data, 5);

    int ok = 1;
    ok &= fabs(stats.mean - 3.0) < TOLERANCE;
    ok &= fabs(stats.variance - 2.0) < TOLERANCE;
    ok &= fabs(stats.min - 1.0) < TOLERANCE;
    ok &= fabs(stats.max - 5.0) < TOLERANCE;
    ok &= fabs(stats.range - 4.0) < TOLERANCE;
    ok &= stats.count == 5;

    if (ok) {
        printf("  PASS: mean=%.2f, var=%.2f, min=%.2f, max=%.2f, range=%.2f, n=%zu\n",
               stats.mean, stats.variance, stats.min, stats.max, stats.range, stats.count);
        pass_count++;
    } else {
        printf("  FAIL: unexpected values\n");
    }
}

int main() {
    printf("================================================================================\n");
    printf("  FP STATS V3 PURE - TESTS (ZERO User-Facing Loops!)\n");
    printf("================================================================================\n\n");

    test_mean_pure();
    test_variance_pure();
    test_mean_variance_welford();
    test_euclidean_distance_pure();
    test_covariance_pure();
    test_normalize_pure();
    test_standardize_pure();
    test_summary_stats();

    printf("\n================================================================================\n");
    printf("  RESULTS: %d/%d tests passed\n", pass_count, test_count);
    printf("================================================================================\n");

    return (pass_count == test_count) ? 0 : 1;
}
