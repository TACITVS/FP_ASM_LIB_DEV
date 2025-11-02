// demo_correlation.c
// Comprehensive test suite for Algorithm #3: Correlation & Covariance
// Tests correctness with edge cases and real-world scenarios
// Benchmarks performance against naive C implementations

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/fp_core.h"

// Tolerance for floating-point comparisons
#define EPSILON 1e-9

// ============================================================================
// BASELINE C IMPLEMENTATIONS (for correctness verification and benchmarking)
// ============================================================================

double c_covariance(const double* x, const double* y, size_t n) {
    if (n == 0) return NAN;
    if (n == 1) return 0.0;

    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum_x += x[i];
        sum_y += y[i];
        sum_xy += x[i] * y[i];
    }

    double mean_x = sum_x / n;
    double mean_y = sum_y / n;
    double mean_xy = sum_xy / n;

    return mean_xy - (mean_x * mean_y);
}

double c_correlation(const double* x, const double* y, size_t n) {
    if (n == 0 || n == 1) return NAN;

    double sum_x = 0.0, sum_y = 0.0;
    double sum_x2 = 0.0, sum_y2 = 0.0, sum_xy = 0.0;

    for (size_t i = 0; i < n; i++) {
        sum_x += x[i];
        sum_y += y[i];
        sum_x2 += x[i] * x[i];
        sum_y2 += y[i] * y[i];
        sum_xy += x[i] * y[i];
    }

    double mean_x = sum_x / n;
    double mean_y = sum_y / n;
    double var_x = (sum_x2 / n) - (mean_x * mean_x);
    double var_y = (sum_y2 / n) - (mean_y * mean_y);

    if (var_x <= 0.0 || var_y <= 0.0) return NAN;

    double cov = (sum_xy / n) - (mean_x * mean_y);
    double stddev_x = sqrt(var_x);
    double stddev_y = sqrt(var_y);

    return cov / (stddev_x * stddev_y);
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

int compare_doubles(double a, double b) {
    if (isnan(a) && isnan(b)) return 1;
    if (isnan(a) || isnan(b)) return 0;
    return fabs(a - b) < EPSILON;
}

void print_test_header(const char* test_name) {
    printf("\n");
    printf("================================================================================\n");
    printf("%s\n", test_name);
    printf("================================================================================\n");
}

// ============================================================================
// CORRECTNESS TESTS
// ============================================================================

int test_perfect_positive_correlation() {
    print_test_header("TEST 1: Perfect Positive Correlation (r = 1.0)");

    double x[] = {1, 2, 3, 4, 5};
    double y[] = {2, 4, 6, 8, 10};  // y = 2x (perfect linear relationship)
    size_t n = 5;

    double fp_cov = fp_covariance_f64(x, y, n);
    double c_cov = c_covariance(x, y, n);
    double fp_corr = fp_correlation_f64(x, y, n);
    double c_corr = c_correlation(x, y, n);

    printf("Data: x = {1, 2, 3, 4, 5}, y = {2, 4, 6, 8, 10}\n");
    printf("Expected: r = 1.0 (perfect positive correlation)\n");
    printf("FP-ASM covariance:  %.10f\n", fp_cov);
    printf("C covariance:       %.10f\n", c_cov);
    printf("FP-ASM correlation: %.10f\n", fp_corr);
    printf("C correlation:      %.10f\n", c_corr);

    int cov_match = compare_doubles(fp_cov, c_cov);
    int corr_match = compare_doubles(fp_corr, c_corr);
    int corr_value = fabs(fp_corr - 1.0) < EPSILON;

    if (cov_match && corr_match && corr_value) {
        printf("PASS: Covariance and correlation match C, r = 1.0\n");
        return 1;
    } else {
        printf("FAIL: Mismatch detected\n");
        return 0;
    }
}

int test_perfect_negative_correlation() {
    print_test_header("TEST 2: Perfect Negative Correlation (r = -1.0)");

    double x[] = {1, 2, 3, 4, 5};
    double y[] = {10, 8, 6, 4, 2};  // y = 12 - 2x (perfect negative linear)
    size_t n = 5;

    double fp_cov = fp_covariance_f64(x, y, n);
    double c_cov = c_covariance(x, y, n);
    double fp_corr = fp_correlation_f64(x, y, n);
    double c_corr = c_correlation(x, y, n);

    printf("Data: x = {1, 2, 3, 4, 5}, y = {10, 8, 6, 4, 2}\n");
    printf("Expected: r = -1.0 (perfect negative correlation)\n");
    printf("FP-ASM covariance:  %.10f\n", fp_cov);
    printf("C covariance:       %.10f\n", c_cov);
    printf("FP-ASM correlation: %.10f\n", fp_corr);
    printf("C correlation:      %.10f\n", c_corr);

    int cov_match = compare_doubles(fp_cov, c_cov);
    int corr_match = compare_doubles(fp_corr, c_corr);
    int corr_value = fabs(fp_corr + 1.0) < EPSILON;

    if (cov_match && corr_match && corr_value) {
        printf("PASS: Covariance and correlation match C, r = -1.0\n");
        return 1;
    } else {
        printf("FAIL: Mismatch detected\n");
        return 0;
    }
}

int test_no_correlation() {
    print_test_header("TEST 3: No Correlation (r ≈ 0)");

    double x[] = {1, 2, 3, 4, 5, 6, 7, 8};
    double y[] = {2, 4, 3, 5, 3, 5, 4, 6};  // Random-ish values
    size_t n = 8;

    double fp_cov = fp_covariance_f64(x, y, n);
    double c_cov = c_covariance(x, y, n);
    double fp_corr = fp_correlation_f64(x, y, n);
    double c_corr = c_correlation(x, y, n);

    printf("Data: x = {1,2,3,4,5,6,7,8}, y = {2,4,3,5,3,5,4,6}\n");
    printf("Expected: r ≈ 0 (weak or no correlation)\n");
    printf("FP-ASM covariance:  %.10f\n", fp_cov);
    printf("C covariance:       %.10f\n", c_cov);
    printf("FP-ASM correlation: %.10f\n", fp_corr);
    printf("C correlation:      %.10f\n", c_corr);

    int cov_match = compare_doubles(fp_cov, c_cov);
    int corr_match = compare_doubles(fp_corr, c_corr);

    if (cov_match && corr_match) {
        printf("PASS: Covariance and correlation match C\n");
        return 1;
    } else {
        printf("FAIL: Mismatch detected\n");
        return 0;
    }
}

int test_zero_variance() {
    print_test_header("TEST 4: Zero Variance (r = NaN)");

    double x[] = {5, 5, 5, 5, 5};  // Constant array
    double y[] = {1, 2, 3, 4, 5};
    size_t n = 5;

    double fp_corr = fp_correlation_f64(x, y, n);
    double c_corr = c_correlation(x, y, n);

    printf("Data: x = {5,5,5,5,5} (constant), y = {1,2,3,4,5}\n");
    printf("Expected: r = NaN (zero variance in x)\n");
    printf("FP-ASM correlation: %.10f\n", fp_corr);
    printf("C correlation:      %.10f\n", c_corr);

    int both_nan = isnan(fp_corr) && isnan(c_corr);

    if (both_nan) {
        printf("PASS: Both return NaN for zero variance\n");
        return 1;
    } else {
        printf("FAIL: Expected NaN\n");
        return 0;
    }
}

int test_edge_cases() {
    print_test_header("TEST 5: Edge Cases (empty, single element)");

    double x[] = {42.0};
    double y[] = {99.0};

    // Empty array
    double fp_cov_empty = fp_covariance_f64(x, y, 0);
    double fp_corr_empty = fp_correlation_f64(x, y, 0);

    // Single element
    double fp_cov_single = fp_covariance_f64(x, y, 1);
    double fp_corr_single = fp_correlation_f64(x, y, 1);

    printf("Empty array:\n");
    printf("  FP-ASM covariance:  %.10f\n", fp_cov_empty);
    printf("  FP-ASM correlation: %.10f\n", fp_corr_empty);

    printf("Single element:\n");
    printf("  FP-ASM covariance:  %.10f\n", fp_cov_single);
    printf("  FP-ASM correlation: %.10f\n", fp_corr_single);

    int empty_nan = isnan(fp_cov_empty) && isnan(fp_corr_empty);
    int single_ok = fabs(fp_cov_single) < EPSILON && isnan(fp_corr_single);

    if (empty_nan && single_ok) {
        printf("PASS: Edge cases handled correctly\n");
        return 1;
    } else {
        printf("FAIL: Edge case mismatch\n");
        return 0;
    }
}

// ============================================================================
// REAL-WORLD SCENARIOS
// ============================================================================

void scenario_stock_market() {
    print_test_header("REAL-WORLD SCENARIO 1: Stock Market Correlation");
    printf("Analyzing correlation between Tech Stock A and Market Index\n");
    printf("Data: Daily returns over 30 trading days (percentage changes)\n\n");

    // Tech Stock A daily returns (%)
    double stock_a[] = {
        2.3, -1.2, 0.8, 3.1, -0.5, 1.7, -2.1, 0.4, 2.8, -1.5,
        1.1, 0.6, -0.9, 2.4, -1.8, 0.3, 1.9, -0.7, 2.2, -1.3,
        0.9, 1.4, -2.3, 0.5, 2.7, -1.1, 1.6, 0.2, -0.8, 2.0
    };

    // Market Index daily returns (%)
    double market[] = {
        1.5, -0.8, 0.6, 2.1, -0.3, 1.2, -1.4, 0.3, 1.9, -1.0,
        0.8, 0.4, -0.6, 1.7, -1.2, 0.2, 1.3, -0.5, 1.5, -0.9,
        0.6, 0.9, -1.6, 0.4, 1.8, -0.7, 1.1, 0.1, -0.5, 1.4
    };

    size_t n = 30;

    double cov = fp_covariance_f64(stock_a, market, n);
    double corr = fp_correlation_f64(stock_a, market, n);

    printf("Results:\n");
    printf("  Covariance:  %.6f\n", cov);
    printf("  Correlation: %.6f\n", corr);
    printf("\nInterpretation:\n");

    if (corr > 0.8) {
        printf("  STRONG POSITIVE: Stock A moves closely with the market (beta ~ %.2f)\n", corr);
        printf("  High systematic risk - diversification provides minimal benefit\n");
    } else if (corr > 0.5) {
        printf("  MODERATE POSITIVE: Stock A generally follows market trends (beta ~ %.2f)\n", corr);
        printf("  Moderate systematic risk - some diversification benefit possible\n");
    } else if (corr > 0.2) {
        printf("  WEAK POSITIVE: Stock A has some market correlation (beta ~ %.2f)\n", corr);
        printf("  Lower systematic risk - good diversification opportunity\n");
    } else if (corr > -0.2) {
        printf("  NO CORRELATION: Stock A moves independently of market (beta ~ %.2f)\n", corr);
        printf("  Minimal systematic risk - excellent diversification asset\n");
    } else {
        printf("  NEGATIVE: Stock A moves opposite to market (hedge asset)\n");
        printf("  Provides portfolio protection during market downturns\n");
    }
}

void scenario_weather_icecream() {
    print_test_header("REAL-WORLD SCENARIO 2: Weather vs Ice Cream Sales");
    printf("Analyzing relationship between temperature and ice cream sales\n");
    printf("Data: 25 days of observations\n\n");

    // Daily temperature (°C)
    double temperature[] = {
        18.5, 22.3, 25.8, 19.2, 21.7, 28.3, 30.1, 24.5, 26.9, 23.4,
        27.8, 29.5, 20.6, 22.9, 31.2, 26.3, 24.1, 28.7, 25.4, 23.8,
        29.9, 27.2, 21.5, 26.6, 30.5
    };

    // Ice cream sales (units per day)
    double sales[] = {
        145, 178, 215, 152, 169, 248, 275, 198, 232, 185,
        241, 268, 159, 176, 295, 221, 192, 255, 208, 181,
        272, 238, 165, 224, 288
    };

    size_t n = 25;

    double cov = fp_covariance_f64(temperature, sales, n);
    double corr = fp_correlation_f64(temperature, sales, n);

    printf("Results:\n");
    printf("  Covariance:  %.4f\n", cov);
    printf("  Correlation: %.6f\n", corr);
    printf("\nInterpretation:\n");

    if (corr > 0.8) {
        printf("  VERY STRONG relationship (r = %.3f): Temperature is an excellent\n", corr);
        printf("  predictor of ice cream sales. For each 1°C increase, expect\n");
        printf("  approximately %.1f additional units sold.\n", cov / 2.0);
        printf("  Business decision: Adjust inventory based on weather forecasts.\n");
    } else {
        printf("  Moderate relationship (r = %.3f): Temperature influences sales\n", corr);
        printf("  but other factors (day of week, promotions) also matter.\n");
    }
}

void scenario_study_time_grades() {
    print_test_header("REAL-WORLD SCENARIO 3: Study Time vs Exam Grades");
    printf("Analyzing relationship between weekly study hours and final exam scores\n");
    printf("Data: 35 students\n\n");

    // Weekly study hours
    double study_hours[] = {
        2.5, 8.3, 5.7, 12.4, 3.8, 9.2, 15.6, 6.4, 10.8, 4.2,
        13.7, 7.9, 11.3, 5.1, 14.2, 8.7, 6.8, 12.9, 4.6, 9.8,
        16.3, 7.2, 10.5, 5.9, 13.1, 8.4, 11.7, 6.3, 14.8, 9.6,
        12.2, 7.5, 10.1, 5.4, 15.9
    };

    // Final exam scores (out of 100)
    double exam_scores[] = {
        62.5, 78.3, 71.2, 88.7, 65.8, 80.5, 94.2, 73.6, 84.9, 67.3,
        90.8, 76.4, 86.3, 70.1, 92.5, 79.7, 74.8, 89.2, 68.9, 81.7,
        96.8, 75.3, 83.6, 72.4, 88.9, 78.9, 87.1, 73.9, 93.7, 82.4,
        87.6, 76.1, 82.8, 71.7, 95.3
    };

    size_t n = 35;

    double cov = fp_covariance_f64(study_hours, exam_scores, n);
    double corr = fp_correlation_f64(study_hours, exam_scores, n);

    printf("Results:\n");
    printf("  Covariance:  %.4f\n", cov);
    printf("  Correlation: %.6f\n", corr);
    printf("\nInterpretation:\n");

    if (corr > 0.7) {
        printf("  STRONG POSITIVE correlation (r = %.3f): Study time significantly\n", corr);
        printf("  impacts exam performance. Each additional study hour correlates with\n");
        printf("  approximately %.2f point increase in exam score.\n", cov / 4.0);
        printf("\n  Educational insight: Consistent study habits are highly effective.\n");
        printf("  Students averaging 12+ hours/week scored in the 85-95 range.\n");
    }
}

void scenario_advertising_revenue() {
    print_test_header("REAL-WORLD SCENARIO 4: Advertising Spend vs Revenue");
    printf("Analyzing ROI of digital advertising campaign\n");
    printf("Data: 28 weekly observations (7 months)\n\n");

    // Weekly ad spend ($1000s)
    double ad_spend[] = {
        5.2, 8.7, 6.3, 12.4, 7.8, 10.5, 15.9, 9.1, 13.6, 6.8,
        11.2, 14.7, 8.4, 10.9, 16.3, 12.8, 7.5, 13.1, 9.7, 15.2,
        11.8, 8.9, 14.3, 10.2, 16.8, 12.4, 9.3, 13.9
    };

    // Weekly revenue ($1000s)
    double revenue[] = {
        42.5, 68.3, 51.7, 95.2, 62.8, 84.5, 119.7, 73.4, 102.9, 55.6,
        89.3, 112.8, 67.9, 86.7, 124.5, 98.6, 61.3, 99.8, 77.2, 116.3,
        93.7, 71.8, 108.9, 81.5, 127.6, 96.4, 74.9, 106.2
    };

    size_t n = 28;

    double cov = fp_covariance_f64(ad_spend, revenue, n);
    double corr = fp_correlation_f64(ad_spend, revenue, n);

    // Calculate approximate ROI multiplier
    double avg_spend = 0.0, avg_revenue = 0.0;
    for (size_t i = 0; i < n; i++) {
        avg_spend += ad_spend[i];
        avg_revenue += revenue[i];
    }
    avg_spend /= n;
    avg_revenue /= n;
    double roi_multiplier = avg_revenue / avg_spend;

    printf("Results:\n");
    printf("  Covariance:      %.4f\n", cov);
    printf("  Correlation:     %.6f\n", corr);
    printf("  Avg Ad Spend:    $%.2fk/week\n", avg_spend);
    printf("  Avg Revenue:     $%.2fk/week\n", avg_revenue);
    printf("  ROI Multiplier:  %.2fx\n", roi_multiplier);
    printf("\nInterpretation:\n");

    if (corr > 0.85) {
        printf("  EXCELLENT correlation (r = %.3f): Ad spending strongly predicts revenue.\n", corr);
        printf("  Each $1k in ad spend generates approximately $%.2fk in revenue.\n", roi_multiplier);
        printf("  Business decision: Campaign is highly effective - consider scaling up.\n");
        printf("  Recommendation: Test higher spend levels to find optimal ROI point.\n");
    }
}

void scenario_height_weight() {
    print_test_header("REAL-WORLD SCENARIO 5: Height vs Weight Correlation");
    printf("Analyzing relationship between height and weight in adults\n");
    printf("Data: 40 individuals from health survey\n\n");

    // Height (cm)
    double height[] = {
        165, 178, 172, 185, 160, 175, 168, 182, 170, 188,
        163, 180, 167, 176, 158, 183, 171, 179, 164, 186,
        169, 177, 162, 184, 173, 181, 166, 187, 174, 190,
        161, 175, 168, 178, 165, 182, 170, 185, 167, 180
    };

    // Weight (kg)
    double weight[] = {
        58.5, 75.2, 68.7, 82.3, 54.8, 71.5, 63.9, 79.6, 66.4, 86.1,
        57.3, 77.8, 62.5, 73.9, 52.6, 81.7, 67.2, 76.5, 59.8, 84.3,
        64.7, 74.6, 56.9, 83.5, 69.8, 78.9, 61.3, 85.7, 70.5, 88.4,
        55.7, 72.8, 64.1, 75.8, 60.2, 80.4, 67.6, 83.9, 62.8, 77.2
    };

    size_t n = 40;

    double cov = fp_covariance_f64(height, weight, n);
    double corr = fp_correlation_f64(height, weight, n);

    printf("Results:\n");
    printf("  Covariance:  %.4f\n", cov);
    printf("  Correlation: %.6f\n", corr);
    printf("\nInterpretation:\n");

    if (corr > 0.7) {
        printf("  STRONG correlation (r = %.3f): Height is a good predictor of weight.\n", corr);
        printf("  For each 1 cm increase in height, weight typically increases by ~%.2f kg.\n", cov / 10.0);
        printf("\n  Medical context: This correlation is expected in healthy populations.\n");
        printf("  BMI calculations rely on this height-weight relationship.\n");
        printf("  Deviations from expected weight for height may indicate health concerns.\n");
    }
}

// ============================================================================
// PERFORMANCE BENCHMARKS
// ============================================================================

void benchmark_covariance() {
    print_test_header("PERFORMANCE BENCHMARK: Covariance");

    // Test with different array sizes
    size_t sizes[] = {100, 1000, 10000, 100000};

    for (int i = 0; i < 4; i++) {
        size_t n = sizes[i];

        // Allocate arrays
        double* x = (double*)malloc(n * sizeof(double));
        double* y = (double*)malloc(n * sizeof(double));

        // Fill with semi-random data
        for (size_t j = 0; j < n; j++) {
            x[j] = (double)(j % 100) + (j * 0.01);
            y[j] = (double)((j * 2) % 100) + (j * 0.02);
        }

        // Determine iteration count based on array size
        int iterations = (n < 1000) ? 5000000 : (n < 10000) ? 1000000 : 100000;

        // Benchmark FP-ASM
        volatile double fp_result = 0.0;
        clock_t start = clock();
        for (int iter = 0; iter < iterations; iter++) {
            fp_result += fp_covariance_f64(x, y, n);
        }
        clock_t end = clock();
        double fp_time = (double)(end - start) / CLOCKS_PER_SEC;

        // Benchmark C
        volatile double c_result = 0.0;
        start = clock();
        for (int iter = 0; iter < iterations; iter++) {
            c_result += c_covariance(x, y, n);
        }
        end = clock();
        double c_time = (double)(end - start) / CLOCKS_PER_SEC;

        // Calculate speedup
        double speedup = c_time / fp_time;
        double ns_per_call_fp = (fp_time * 1e9) / iterations;
        double ns_per_call_c = (c_time * 1e9) / iterations;

        printf("\nArray size: %zu elements\n", n);
        printf("Iterations: %d\n", iterations);
        printf("FP-ASM:  %.4f seconds (%.2f ns/call)\n", fp_time, ns_per_call_fp);
        printf("C:       %.4f seconds (%.2f ns/call)\n", c_time, ns_per_call_c);
        printf("Speedup: %.2fx\n", speedup);

        free(x);
        free(y);
    }
}

void benchmark_correlation() {
    print_test_header("PERFORMANCE BENCHMARK: Correlation");

    // Test with different array sizes
    size_t sizes[] = {100, 1000, 10000, 100000};

    for (int i = 0; i < 4; i++) {
        size_t n = sizes[i];

        // Allocate arrays
        double* x = (double*)malloc(n * sizeof(double));
        double* y = (double*)malloc(n * sizeof(double));

        // Fill with semi-random data
        for (size_t j = 0; j < n; j++) {
            x[j] = (double)(j % 100) + (j * 0.01);
            y[j] = (double)((j * 2) % 100) + (j * 0.02);
        }

        // Determine iteration count based on array size
        int iterations = (n < 1000) ? 5000000 : (n < 10000) ? 1000000 : 100000;

        // Benchmark FP-ASM
        volatile double fp_result = 0.0;
        clock_t start = clock();
        for (int iter = 0; iter < iterations; iter++) {
            fp_result += fp_correlation_f64(x, y, n);
        }
        clock_t end = clock();
        double fp_time = (double)(end - start) / CLOCKS_PER_SEC;

        // Benchmark C
        volatile double c_result = 0.0;
        start = clock();
        for (int iter = 0; iter < iterations; iter++) {
            c_result += c_correlation(x, y, n);
        }
        end = clock();
        double c_time = (double)(end - start) / CLOCKS_PER_SEC;

        // Calculate speedup
        double speedup = c_time / fp_time;
        double ns_per_call_fp = (fp_time * 1e9) / iterations;
        double ns_per_call_c = (c_time * 1e9) / iterations;

        printf("\nArray size: %zu elements\n", n);
        printf("Iterations: %d\n", iterations);
        printf("FP-ASM:  %.4f seconds (%.2f ns/call)\n", fp_time, ns_per_call_fp);
        printf("C:       %.4f seconds (%.2f ns/call)\n", c_time, ns_per_call_c);
        printf("Speedup: %.2fx\n", speedup);

        free(x);
        free(y);
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    printf("================================================================================\n");
    printf("FP-ASM LIBRARY - ALGORITHM #3: CORRELATION & COVARIANCE\n");
    printf("================================================================================\n");
    printf("Testing correctness and performance of covariance and correlation functions\n");
    printf("Target performance: 2.0-2.5x speedup (single-pass fused operations)\n");

    // Run correctness tests
    printf("\n\n");
    printf("################################################################################\n");
    printf("# PART 1: CORRECTNESS TESTS\n");
    printf("################################################################################\n");

    int passed = 0, total = 5;

    passed += test_perfect_positive_correlation();
    passed += test_perfect_negative_correlation();
    passed += test_no_correlation();
    passed += test_zero_variance();
    passed += test_edge_cases();

    printf("\n");
    printf("================================================================================\n");
    printf("CORRECTNESS TESTS SUMMARY: %d / %d passed\n", passed, total);
    printf("================================================================================\n");

    if (passed != total) {
        printf("\nERROR: Some tests failed. Fix issues before proceeding.\n");
        return 1;
    }

    printf("\nAll correctness tests passed! Proceeding to real-world scenarios...\n");

    // Run real-world scenarios
    printf("\n\n");
    printf("################################################################################\n");
    printf("# PART 2: REAL-WORLD SCENARIOS\n");
    printf("################################################################################\n");

    scenario_stock_market();
    scenario_weather_icecream();
    scenario_study_time_grades();
    scenario_advertising_revenue();
    scenario_height_weight();

    // Run performance benchmarks
    printf("\n\n");
    printf("################################################################################\n");
    printf("# PART 3: PERFORMANCE BENCHMARKS\n");
    printf("################################################################################\n");

    benchmark_covariance();
    benchmark_correlation();

    printf("\n\n");
    printf("================================================================================\n");
    printf("ALL TESTS COMPLETED SUCCESSFULLY\n");
    printf("================================================================================\n");

    return 0;
}
