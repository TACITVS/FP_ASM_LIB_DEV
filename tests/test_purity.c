/**
 * test_purity.c
 *
 * Purity Contract Verification
 * Tests that percentile and outlier functions maintain input immutability
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/fp_core.h"

#define TEST_SIZE 100
#define TOLERANCE 1e-9

// Test helper: verify array unchanged
int verify_unchanged(const double* original, const double* current, size_t n, const char* test_name) {
    for (size_t i = 0; i < n; i++) {
        if (fabs(original[i] - current[i]) > TOLERANCE) {
            printf("❌ PURITY VIOLATION in %s: data[%zu] changed from %.6f to %.6f\n",
                   test_name, i, original[i], current[i]);
            return 0;
        }
    }
    printf("✅ PURITY VERIFIED: %s - input data unchanged\n", test_name);
    return 1;
}

int main(void) {
    printf("=== FP-ASM PURITY CONTRACT VERIFICATION ===\n\n");

    // Create unsorted test data (intentionally out of order)
    double data[TEST_SIZE];
    double backup[TEST_SIZE];

    // Fill with unsorted values
    for (size_t i = 0; i < TEST_SIZE; i++) {
        data[i] = (double)(TEST_SIZE - i) + 0.5 * (i % 7);
    }

    // Create backup for verification
    memcpy(backup, data, sizeof(data));

    printf("Test Data: Intentionally UNSORTED array of %d elements\n", TEST_SIZE);
    printf("First 10 values: ");
    for (int i = 0; i < 10; i++) {
        printf("%.1f ", data[i]);
    }
    printf("...\n\n");

    int all_passed = 1;

    // ========================================================================
    // Test 1: fp_percentile_f64
    // ========================================================================
    printf("--- Test 1: fp_percentile_f64 ---\n");
    double median = fp_percentile_f64(data, TEST_SIZE, 0.5);
    printf("Median (50th percentile): %.6f\n", median);

    if (!verify_unchanged(backup, data, TEST_SIZE, "fp_percentile_f64")) {
        all_passed = 0;
    }
    printf("\n");

    // ========================================================================
    // Test 2: fp_percentiles_f64
    // ========================================================================
    printf("--- Test 2: fp_percentiles_f64 ---\n");
    double p_values[] = {0.25, 0.5, 0.75, 0.95};
    double results[4];
    fp_percentiles_f64(data, TEST_SIZE, p_values, 4, results);

    printf("Percentiles:\n");
    printf("  25th: %.6f\n", results[0]);
    printf("  50th: %.6f\n", results[1]);
    printf("  75th: %.6f\n", results[2]);
    printf("  95th: %.6f\n", results[3]);

    if (!verify_unchanged(backup, data, TEST_SIZE, "fp_percentiles_f64")) {
        all_passed = 0;
    }
    printf("\n");

    // ========================================================================
    // Test 3: fp_quartiles_f64
    // ========================================================================
    printf("--- Test 3: fp_quartiles_f64 ---\n");
    Quartiles quartiles;
    fp_quartiles_f64(data, TEST_SIZE, &quartiles);

    printf("Quartiles:\n");
    printf("  Q1:     %.6f\n", quartiles.q1);
    printf("  Median: %.6f\n", quartiles.median);
    printf("  Q3:     %.6f\n", quartiles.q3);
    printf("  IQR:    %.6f\n", quartiles.iqr);

    if (!verify_unchanged(backup, data, TEST_SIZE, "fp_quartiles_f64")) {
        all_passed = 0;
    }
    printf("\n");

    // ========================================================================
    // Test 4: fp_detect_outliers_iqr_f64
    // ========================================================================
    printf("--- Test 4: fp_detect_outliers_iqr_f64 ---\n");
    uint8_t is_outlier[TEST_SIZE];
    size_t outlier_count = fp_detect_outliers_iqr_f64(data, TEST_SIZE, 1.5, is_outlier);

    printf("Outliers detected: %zu / %d\n", outlier_count, TEST_SIZE);
    if (outlier_count > 0 && outlier_count < 20) {
        printf("Outlier indices: ");
        for (size_t i = 0; i < TEST_SIZE; i++) {
            if (is_outlier[i]) {
                printf("%zu ", i);
            }
        }
        printf("\n");
    }

    if (!verify_unchanged(backup, data, TEST_SIZE, "fp_detect_outliers_iqr_f64")) {
        all_passed = 0;
    }
    printf("\n");

    // ========================================================================
    // Final Verification
    // ========================================================================
    printf("=== FINAL VERIFICATION ===\n");
    printf("Input array state after ALL 4 function calls:\n");

    int total_changed = 0;
    for (size_t i = 0; i < TEST_SIZE; i++) {
        if (fabs(backup[i] - data[i]) > TOLERANCE) {
            total_changed++;
        }
    }

    if (total_changed == 0) {
        printf("✅ PERFECT PURITY: 0 / %d elements changed\n", TEST_SIZE);
        printf("✅ All functions maintain FP purity contract\n");
    } else {
        printf("❌ PURITY VIOLATION: %d / %d elements changed\n", total_changed, TEST_SIZE);
        all_passed = 0;
    }

    printf("\n=== TEST RESULT ===\n");
    if (all_passed) {
        printf("✅ ALL TESTS PASSED - Purity contract verified\n");
        return 0;
    } else {
        printf("❌ TESTS FAILED - Purity violations detected\n");
        return 1;
    }
}
