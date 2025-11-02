// demo_outliers.c
// Comprehensive test suite for Algorithm #5: Outlier Detection
// Tests Z-score and IQR methods with real-world scenarios

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "../include/fp_core.h"

#define EPSILON 1e-9

// ============================================================================
// BASELINE C IMPLEMENTATIONS
// ============================================================================

size_t c_detect_outliers_zscore(const double* data, size_t n, double threshold, uint8_t* is_outlier) {
    if (n < 2) {
        for (size_t i = 0; i < n; i++) is_outlier[i] = 0;
        return 0;
    }

    // Compute mean and stddev
    double sum = 0.0, sum_sq = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum += data[i];
        sum_sq += data[i] * data[i];
    }
    double mean = sum / n;
    double variance = (sum_sq / n) - (mean * mean);

    if (variance <= 0.0) {
        for (size_t i = 0; i < n; i++) is_outlier[i] = 0;
        return 0;
    }

    double stddev = sqrt(variance);

    // Mark outliers
    size_t count = 0;
    for (size_t i = 0; i < n; i++) {
        double z = fabs((data[i] - mean) / stddev);
        is_outlier[i] = (z > threshold) ? 1 : 0;
        if (is_outlier[i]) count++;
    }
    return count;
}

size_t c_detect_outliers_iqr(const double* sorted_data, size_t n, double k, uint8_t* is_outlier) {
    if (n < 4) {
        for (size_t i = 0; i < n; i++) is_outlier[i] = 0;
        return 0;
    }

    // Calculate Q1 and Q3
    size_t q1_idx = n / 4;
    size_t q3_idx = (3 * n) / 4;
    double q1 = sorted_data[q1_idx];
    double q3 = sorted_data[q3_idx];
    double iqr = q3 - q1;

    double lower = q1 - k * iqr;
    double upper = q3 + k * iqr;

    size_t count = 0;
    for (size_t i = 0; i < n; i++) {
        is_outlier[i] = (sorted_data[i] < lower || sorted_data[i] > upper) ? 1 : 0;
        if (is_outlier[i]) count++;
    }
    return count;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void print_test_header(const char* test_name) {
    printf("\n================================================================================\n");
    printf("%s\n", test_name);
    printf("================================================================================\n");
}

int compare_arrays(const uint8_t* a, const uint8_t* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

void print_outliers(const double* data, const uint8_t* is_outlier, size_t n, size_t max_print) {
    size_t printed = 0;
    for (size_t i = 0; i < n && printed < max_print; i++) {
        if (is_outlier[i]) {
            printf("    Index %zu: %.2f\n", i, data[i]);
            printed++;
        }
    }
    if (printed == max_print && printed > 0) {
        size_t remaining = 0;
        for (size_t i = 0; i < n; i++) if (is_outlier[i]) remaining++;
        remaining -= printed;
        if (remaining > 0) printf("    ... and %zu more\n", remaining);
    }
}

// ============================================================================
// CORRECTNESS TESTS
// ============================================================================

int test_zscore_normal_distribution() {
    print_test_header("TEST 1: Z-Score on Normal Distribution");

    double data[] = {10, 12, 13, 11, 14, 13, 12, 15, 13, 12,  // Normal
                     50, 11, 13, 12, 14};  // One outlier (50)
    size_t n = 15;

    uint8_t fp_result[15], c_result[15];

    size_t fp_count = fp_detect_outliers_zscore_f64(data, n, 3.0, fp_result);
    size_t c_count = c_detect_outliers_zscore(data, n, 3.0, c_result);

    printf("Data: Mostly 10-15, with one extreme value (50)\n");
    printf("Threshold: 3.0 standard deviations\n");
    printf("FP-ASM detected: %zu outliers\n", fp_count);
    printf("C detected:      %zu outliers\n", c_count);

    int match = compare_arrays(fp_result, c_result, n) && (fp_count == c_count);
    printf("\n%s\n", match ? "PASS: Implementations match" : "FAIL: Mismatch");
    return match;
}

int test_iqr_method() {
    print_test_header("TEST 2: IQR Method on Sorted Data");

    double sorted[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,  // Normal range
                       25, 30};  // Outliers
    size_t n = 12;

    uint8_t fp_result[12], c_result[12];

    size_t fp_count = fp_detect_outliers_iqr_f64(sorted, n, 1.5, fp_result);
    size_t c_count = c_detect_outliers_iqr(sorted, n, 1.5, c_result);

    printf("Data: 1-10 (normal), 25, 30 (outliers)\n");
    printf("k_factor: 1.5\n");
    printf("FP-ASM detected: %zu outliers\n", fp_count);
    printf("C detected:      %zu outliers\n", c_count);

    int match = compare_arrays(fp_result, c_result, n) && (fp_count == c_count);
    printf("\n%s\n", match ? "PASS: Implementations match" : "FAIL: Mismatch");
    return match;
}

int test_no_outliers() {
    print_test_header("TEST 3: No Outliers Present");

    double data[] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    size_t n = 10;

    uint8_t fp_result[10];
    size_t fp_count = fp_detect_outliers_zscore_f64(data, n, 3.0, fp_result);

    printf("Data: Uniform range 10-19\n");
    printf("FP-ASM detected: %zu outliers\n", fp_count);

    int pass = (fp_count == 0);
    printf("\n%s\n", pass ? "PASS: Correctly found no outliers" : "FAIL");
    return pass;
}

int test_all_outliers() {
    print_test_header("TEST 4: All Values Identical (Edge Case)");

    double data[] = {42, 42, 42, 42, 42, 42};
    size_t n = 6;

    uint8_t fp_result[6];
    size_t fp_count = fp_detect_outliers_zscore_f64(data, n, 3.0, fp_result);

    printf("Data: All values = 42 (zero variance)\n");
    printf("FP-ASM detected: %zu outliers\n", fp_count);

    int pass = (fp_count == 0);
    printf("\n%s\n", pass ? "PASS: Zero variance handled correctly" : "FAIL");
    return pass;
}

// ============================================================================
// REAL-WORLD SCENARIOS
// ============================================================================

void scenario_sensor_data() {
    print_test_header("REAL-WORLD SCENARIO 1: Temperature Sensor Anomaly Detection");
    printf("Industrial IoT: Detecting faulty sensor readings\n");
    printf("Data: 50 temperature measurements (degrees C)\n\n");

    double temps[] = {
        21.2, 21.5, 21.3, 21.8, 21.4, 21.6, 21.7, 21.5, 21.3, 21.6,
        21.4, 21.7, 21.5, 21.8, 21.3, 21.6, 21.4, 21.7, 21.5, 21.3,
        45.2,  // Faulty sensor spike
        21.6, 21.4, 21.5, 21.7, 21.3, 21.6, 21.8, 21.4, 21.5,
        21.7, 21.3, 21.6, 21.4, 21.8, 21.5, 21.3, 21.6, 21.7, 21.4,
        -5.3,  // Faulty sensor drop
        21.5, 21.6, 21.4, 21.7, 21.3, 21.8, 21.5, 21.6, 21.4
    };
    size_t n = 50;

    uint8_t outliers[50];
    size_t count = fp_detect_outliers_zscore_f64(temps, n, 3.0, outliers);

    printf("Results:\n");
    printf("  Total measurements: %zu\n", n);
    printf("  Outliers detected: %zu (%.1f%%)\n", count, (count * 100.0) / n);
    printf("\n  Anomalous readings:\n");
    print_outliers(temps, outliers, n, 5);

    printf("\nInterpretation:\n");
    printf("  Normal range: 21.2-21.8 C\n");
    printf("  Detected %zu faulty readings requiring sensor maintenance\n", count);
    printf("  Action: Flag sensors at these timestamps for inspection\n");
}

void scenario_fraud_detection() {
    print_test_header("REAL-WORLD SCENARIO 2: Credit Card Fraud Detection");
    printf("Financial Security: Identifying suspicious transactions\n");
    printf("Data: 40 transaction amounts ($)\n\n");

    double transactions[] = {
        45.32, 23.15, 67.89, 34.50, 56.78, 43.21, 38.90, 52.34, 41.67, 49.20,
        35.80, 58.45, 42.10, 61.23, 47.55, 39.88, 54.32, 36.70, 48.90, 44.15,
        5432.10,  // Suspicious large transaction
        50.25, 46.78, 55.60, 41.30, 59.12, 37.85, 51.40, 43.90, 48.20,
        2150.00,  // Another suspicious transaction
        42.55, 57.30, 40.10, 53.75, 46.20, 49.80, 44.65, 52.10, 38.45
    };
    size_t n = 40;

    uint8_t outliers[40];
    size_t count = fp_detect_outliers_zscore_f64(transactions, n, 2.5, outliers);

    printf("Results:\n");
    printf("  Total transactions: %zu\n", n);
    printf("  Flagged for review: %zu (%.1f%%)\n", count, (count * 100.0) / n);
    printf("\n  Suspicious transactions:\n");
    print_outliers(transactions, outliers, n, 5);

    printf("\nInterpretation:\n");
    printf("  Normal transaction range: $23-$68\n");
    printf("  Detected %zu transactions requiring fraud review\n", count);
    printf("  Action: Contact customer to verify large purchases\n");
}

void scenario_manufacturing_qc() {
    print_test_header("REAL-WORLD SCENARIO 3: Manufacturing Quality Control");
    printf("Production Line: Detecting defective components\n");
    printf("Data: 45 component weights (grams) - SORTED for IQR\n\n");

    double weights[] = {
        98.5, 98.7, 98.9, 99.0, 99.1, 99.2, 99.3, 99.4, 99.5, 99.6,
        99.7, 99.8, 99.9, 100.0, 100.1, 100.2, 100.3, 100.4, 100.5, 100.6,
        100.7, 100.8, 100.9, 101.0, 101.1, 101.2, 101.3, 101.4, 101.5, 101.6,
        95.0,  // Underweight defect
        101.7, 101.8, 101.9, 102.0, 102.1, 102.2, 102.3, 102.4, 102.5,
        106.5,  // Overweight defect
        102.6, 102.7, 102.8, 102.9
    };
    size_t n = 45;

    // Sort the array for IQR method
    for (size_t i = 0; i < n-1; i++) {
        for (size_t j = i+1; j < n; j++) {
            if (weights[i] > weights[j]) {
                double temp = weights[i];
                weights[i] = weights[j];
                weights[j] = temp;
            }
        }
    }

    uint8_t outliers[45];
    size_t count = fp_detect_outliers_iqr_f64(weights, n, 1.5, outliers);

    printf("Results:\n");
    printf("  Components inspected: %zu\n", n);
    printf("  Defects found: %zu (%.1f%% rejection rate)\n", count, (count * 100.0) / n);
    printf("\n  Defective components:\n");
    print_outliers(weights, outliers, n, 5);

    printf("\nInterpretation:\n");
    printf("  Target weight: 100g +/- 2.5g\n");
    printf("  IQR method detected %zu out-of-spec components\n", count);
    printf("  Action: Remove defects, investigate production drift\n");
}

void scenario_network_traffic() {
    print_test_header("REAL-WORLD SCENARIO 4: Network Traffic Analysis");
    printf("Cybersecurity: Detecting DDoS attack signatures\n");
    printf("Data: 35 requests per second measurements\n\n");

    double rps[] = {
        120, 135, 128, 142, 131, 139, 126, 144, 133, 137,
        129, 141, 134, 138, 127, 143, 132, 136, 130, 140,
        5200,  // DDoS spike
        135, 129, 142, 131, 138, 127, 144, 133, 139,
        4800,  // Another attack spike
        128, 141, 134, 137, 130
    };
    size_t n = 35;

    uint8_t outliers[35];
    size_t count = fp_detect_outliers_zscore_f64(rps, n, 3.0, outliers);

    printf("Results:\n");
    printf("  Time periods analyzed: %zu\n", n);
    printf("  Attack signatures: %zu (%.1f%%)\n", count, (count * 100.0) / n);
    printf("\n  Suspicious traffic spikes:\n");
    print_outliers(rps, outliers, n, 5);

    printf("\nInterpretation:\n");
    printf("  Normal traffic: 120-144 requests/second\n");
    printf("  Detected %zu periods with abnormal traffic (>3 sigma)\n", count);
    printf("  Action: Activate DDoS mitigation, block suspicious IPs\n");
}

void scenario_medical_measurements() {
    print_test_header("REAL-WORLD SCENARIO 5: Medical Lab Results");
    printf("Healthcare: Identifying measurement errors\n");
    printf("Data: 30 glucose readings (mg/dL)\n\n");

    double glucose[] = {
        95, 102, 98, 106, 100, 104, 97, 108, 101, 105,
        99, 107, 103, 96, 109, 102, 98, 106, 100, 104,
        250,  // Potentially erroneous reading
        97, 105, 101, 103, 99, 108, 102, 96, 104
    };
    size_t n = 30;

    uint8_t outliers[30];
    size_t count = fp_detect_outliers_zscore_f64(glucose, n, 3.0, outliers);

    printf("Results:\n");
    printf("  Samples tested: %zu\n", n);
    printf("  Flagged results: %zu (%.1f%%)\n", count, (count * 100.0) / n);
    printf("\n  Suspicious readings:\n");
    print_outliers(glucose, outliers, n, 5);

    printf("\nInterpretation:\n");
    printf("  Normal fasting glucose: 95-109 mg/dL\n");
    printf("  Detected %zu reading(s) requiring retest\n", count);
    printf("  Action: Repeat test, check for sample contamination\n");
}

// ============================================================================
// PERFORMANCE BENCHMARKS
// ============================================================================

void benchmark_zscore() {
    print_test_header("PERFORMANCE BENCHMARK: Z-Score Method");

    size_t sizes[] = {100, 1000, 10000, 100000};

    for (int i = 0; i < 4; i++) {
        size_t n = sizes[i];

        double* data = (double*)malloc(n * sizeof(double));
        uint8_t* fp_out = (uint8_t*)malloc(n);
        uint8_t* c_out = (uint8_t*)malloc(n);

        for (size_t j = 0; j < n; j++) {
            data[j] = 100.0 + (j % 50) - 25;
        }

        int iterations = (n < 1000) ? 100000 : (n < 10000) ? 10000 : 1000;

        volatile size_t fp_result = 0;
        clock_t start = clock();
        for (int iter = 0; iter < iterations; iter++) {
            fp_result = fp_detect_outliers_zscore_f64(data, n, 3.0, fp_out);
        }
        clock_t end = clock();
        double fp_time = (double)(end - start) / CLOCKS_PER_SEC;

        volatile size_t c_result = 0;
        start = clock();
        for (int iter = 0; iter < iterations; iter++) {
            c_result = c_detect_outliers_zscore(data, n, 3.0, c_out);
        }
        end = clock();
        double c_time = (double)(end - start) / CLOCKS_PER_SEC;

        double speedup = c_time / fp_time;
        double us_per_call_fp = (fp_time * 1e6) / iterations;
        double us_per_call_c = (c_time * 1e6) / iterations;

        printf("\nArray size: %zu elements\n", n);
        printf("FP-ASM:  %.4f seconds (%.2f us/call)\n", fp_time, us_per_call_fp);
        printf("C:       %.4f seconds (%.2f us/call)\n", c_time, us_per_call_c);
        printf("Speedup: %.2fx\n", speedup);

        free(data);
        free(fp_out);
        free(c_out);
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    printf("================================================================================\n");
    printf("FP-ASM LIBRARY - ALGORITHM #5: OUTLIER DETECTION\n");
    printf("================================================================================\n");
    printf("Testing Z-score and IQR outlier detection methods\n");
    printf("Target performance: 2.0-2.5x speedup (SIMD statistics pass)\n");

    printf("\n\n################################################################################\n");
    printf("# PART 1: CORRECTNESS TESTS\n");
    printf("################################################################################\n");

    int passed = 0, total = 4;
    passed += test_zscore_normal_distribution();
    passed += test_iqr_method();
    passed += test_no_outliers();
    passed += test_all_outliers();

    printf("\n================================================================================\n");
    printf("CORRECTNESS TESTS SUMMARY: %d / %d passed\n", passed, total);
    printf("================================================================================\n");

    if (passed != total) {
        printf("\nERROR: Some tests failed.\n");
        return 1;
    }

    printf("\nAll correctness tests passed! Proceeding to real-world scenarios...\n");

    printf("\n\n################################################################################\n");
    printf("# PART 2: REAL-WORLD SCENARIOS\n");
    printf("################################################################################\n");

    scenario_sensor_data();
    scenario_fraud_detection();
    scenario_manufacturing_qc();
    scenario_network_traffic();
    scenario_medical_measurements();

    printf("\n\n################################################################################\n");
    printf("# PART 3: PERFORMANCE BENCHMARKS\n");
    printf("################################################################################\n");

    benchmark_zscore();

    printf("\n\n================================================================================\n");
    printf("ALL TESTS COMPLETED SUCCESSFULLY\n");
    printf("================================================================================\n");

    return 0;
}
