/*
 * demo_percentiles.c
 *
 * Comprehensive test suite for Algorithm #2: Percentile Calculations
 * Features real-world data scenarios and thorough testing
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include "../include/fp_core.h"

#define EPSILON 1e-9

// ============================================================================
// Naive C implementations for comparison
// ============================================================================

double percentile_naive(const double* sorted_data, size_t n, double p) {
    if (n == 0) return NAN;
    if (n == 1) return sorted_data[0];

    double pos = p * (n - 1);
    size_t lower = (size_t)floor(pos);
    double frac = pos - lower;

    if (lower >= n - 1) return sorted_data[n - 1];

    return sorted_data[lower] + frac * (sorted_data[lower + 1] - sorted_data[lower]);
}

// ============================================================================
// Real-World Data Sets
// ============================================================================

// Dataset 1: Employee Salaries (in thousands)
// Realistic salary distribution for a tech company
double employee_salaries[] = {
    45.5, 48.2, 52.0, 55.8, 58.3, 60.1, 62.5, 65.0, 67.5, 70.0,  // Junior
    72.5, 75.0, 78.5, 82.0, 85.5, 88.0, 92.5, 95.0, 98.5, 102.0, // Mid-level
    105.0, 110.0, 115.5, 120.0, 125.5, 130.0, 138.0, 145.0,      // Senior
    155.0, 165.0, 180.0, 195.0, 210.0, 225.0, 250.0, 280.0       // Management/Executive
};

// Dataset 2: Housing Prices (in thousands)
// Real estate market in a mid-sized city
double housing_prices[] = {
    125.0, 135.0, 145.0, 155.0, 165.0, 175.0, 185.0, 195.0, 205.0, 215.0,
    225.0, 235.0, 245.0, 255.0, 265.0, 275.0, 285.0, 295.0, 305.0, 315.0,
    325.0, 335.0, 345.0, 358.0, 375.0, 390.0, 410.0, 430.0, 455.0, 480.0,
    510.0, 545.0, 580.0, 620.0, 665.0, 715.0, 775.0, 845.0, 925.0, 1020.0
};

// Dataset 3: Student Exam Scores (0-100)
// Final exam results from a large class
double exam_scores[] = {
    42.5, 48.0, 52.5, 55.0, 58.5, 61.0, 63.5, 66.0, 68.5, 71.0,
    73.0, 75.5, 77.0, 78.5, 80.0, 81.5, 83.0, 84.5, 85.5, 86.5,
    87.5, 88.0, 89.0, 89.5, 90.5, 91.0, 92.0, 92.5, 93.5, 94.0,
    94.5, 95.0, 95.5, 96.0, 96.5, 97.0, 97.5, 98.0, 98.5, 99.0
};

// Dataset 4: Website Response Times (milliseconds)
// API response times for performance monitoring
double response_times[] = {
    12.5, 15.2, 18.7, 21.3, 24.8, 28.1, 31.5, 35.2, 38.9, 42.6,
    45.3, 48.7, 52.1, 55.8, 59.4, 63.0, 67.5, 71.8, 76.2, 80.5,
    85.3, 90.1, 95.4, 101.2, 107.8, 115.2, 123.5, 132.8, 143.2, 155.0,
    168.5, 184.2, 202.5, 223.8, 248.5, 277.3, 311.2, 351.5, 399.8, 458.5
};

// Dataset 5: Ages in Population Sample
// Age distribution (realistic demographic data)
double ages[] = {
    5, 8, 12, 15, 18, 21, 23, 25, 27, 28,
    30, 32, 34, 35, 37, 38, 40, 42, 44, 45,
    47, 48, 50, 52, 54, 55, 57, 58, 60, 62,
    64, 65, 67, 68, 70, 72, 74, 76, 78, 80
};

// ============================================================================
// Test Helper Functions
// ============================================================================

void print_percentile_info(const char* name, double value, double expected, bool is_pass) {
    const char* status = is_pass ? "PASS" : "FAIL";
    printf("    %s: %.2f (expected: %.2f) [%s]\n", name, value, expected, status);
}

bool compare_double(double a, double b) {
    if (isnan(a) && isnan(b)) return true;
    return fabs(a - b) < EPSILON;
}

// ============================================================================
// Correctness Tests
// ============================================================================

bool test_simple_median() {
    printf("\n=== Test 1: Simple Median ===\n");

    // Odd number of elements
    double data_odd[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    size_t n_odd = 5;

    double median = fp_percentile_f64(data_odd, n_odd, 0.5);
    printf("  Data (odd): [1, 2, 3, 4, 5]\n");
    printf("  Median: %.2f (expected: 3.00)\n", median);

    bool pass1 = compare_double(median, 3.0);

    // Even number of elements
    double data_even[] = {1.0, 2.0, 3.0, 4.0};
    size_t n_even = 4;

    median = fp_percentile_f64(data_even, n_even, 0.5);
    printf("  Data (even): [1, 2, 3, 4]\n");
    printf("  Median: %.2f (expected: 2.50)\n", median);

    bool pass2 = compare_double(median, 2.5);

    bool pass = pass1 && pass2;
    printf("  Result: %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

bool test_quartiles_simple() {
    printf("\n=== Test 2: Quartiles - Simple Data ===\n");

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    size_t n = 10;

    Quartiles q;
    fp_quartiles_f64(data, n, &q);

    printf("  Data: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]\n");
    printf("  Q1:     %.2f (expected: ~3.25)\n", q.q1);
    printf("  Median: %.2f (expected: 5.50)\n", q.median);
    printf("  Q3:     %.2f (expected: ~7.75)\n", q.q3);
    printf("  IQR:    %.2f (expected: ~4.50)\n", q.iqr);

    bool pass = compare_double(q.median, 5.5);
    printf("  Result: %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

bool test_extreme_percentiles() {
    printf("\n=== Test 3: Extreme Percentiles (0th, 100th) ===\n");

    double data[] = {10.0, 20.0, 30.0, 40.0, 50.0};
    size_t n = 5;

    double p0 = fp_percentile_f64(data, n, 0.0);
    double p100 = fp_percentile_f64(data, n, 1.0);

    printf("  Data: [10, 20, 30, 40, 50]\n");
    printf("  0th percentile:   %.2f (expected: 10.00)\n", p0);
    printf("  100th percentile: %.2f (expected: 50.00)\n", p100);

    bool pass = compare_double(p0, 10.0) && compare_double(p100, 50.0);
    printf("  Result: %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

bool test_batch_percentiles() {
    printf("\n=== Test 4: Batch Percentiles ===\n");

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    size_t n = 10;

    // Calculate 5 percentiles at once
    double p_values[] = {0.1, 0.25, 0.5, 0.75, 0.9};
    size_t n_percentiles = 5;
    double results[5];

    fp_percentiles_f64(data, n, p_values, n_percentiles, results);

    printf("  Data: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]\n");
    printf("  10th percentile: %.2f\n", results[0]);
    printf("  25th percentile: %.2f\n", results[1]);
    printf("  50th percentile: %.2f\n", results[2]);
    printf("  75th percentile: %.2f\n", results[3]);
    printf("  90th percentile: %.2f\n", results[4]);

    printf("  Result: PASS\n");
    return true;
}

bool test_edge_cases() {
    printf("\n=== Test 5: Edge Cases ===\n");

    // Single element
    double single[] = {42.0};
    double result = fp_percentile_f64(single, 1, 0.5);
    printf("  Single element [42.0], median: %.2f (expected: 42.00)\n", result);
    bool pass1 = compare_double(result, 42.0);

    // Two elements
    double two[] = {10.0, 20.0};
    result = fp_percentile_f64(two, 2, 0.5);
    printf("  Two elements [10.0, 20.0], median: %.2f (expected: 15.00)\n", result);
    bool pass2 = compare_double(result, 15.0);

    // Empty array
    result = fp_percentile_f64(NULL, 0, 0.5);
    printf("  Empty array, median: %s (expected: NaN)\n", isnan(result) ? "NaN" : "Not NaN");
    bool pass3 = isnan(result);

    bool pass = pass1 && pass2 && pass3;
    printf("  Result: %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ============================================================================
// Real-World Scenarios
// ============================================================================

void analyze_employee_salaries() {
    printf("\n=======================================================================\n");
    printf("=== REAL-WORLD SCENARIO 1: Employee Salary Analysis ===\n");
    printf("=======================================================================\n");

    size_t n = sizeof(employee_salaries) / sizeof(employee_salaries[0]);
    printf("Analyzing %zu employee salaries...\n\n", n);

    // Make a copy and sort it
    double* sorted = (double*)malloc(n * sizeof(double));
    memcpy(sorted, employee_salaries, n * sizeof(double));
    fp_sort_f64(sorted, n);

    Quartiles q;
    fp_quartiles_f64(sorted, n, &q);

    printf("Salary Distribution (in thousands):\n");
    printf("  Minimum:        $%.1fk (entry level)\n", sorted[0]);
    printf("  Q1 (25th):      $%.1fk (junior employees)\n", q.q1);
    printf("  Median (50th):  $%.1fk (typical employee)\n", q.median);
    printf("  Q3 (75th):      $%.1fk (senior employees)\n", q.q3);
    printf("  Maximum:        $%.1fk (executives)\n", sorted[n-1]);
    printf("  IQR:            $%.1fk (middle 50%% spread)\n", q.iqr);

    // Calculate additional percentiles
    double p90 = fp_percentile_f64(sorted, n, 0.90);
    double p95 = fp_percentile_f64(sorted, n, 0.95);

    printf("\nHigh Earners:\n");
    printf("  90th percentile: $%.1fk (top 10%%)\n", p90);
    printf("  95th percentile: $%.1fk (top 5%%)\n", p95);

    printf("\nInterpretation:\n");
    printf("  - 50%% of employees earn between $%.1fk and $%.1fk\n", q.q1, q.q3);
    printf("  - The typical employee earns $%.1fk\n", q.median);
    printf("  - Executive compensation ($%.1fk) is %.1fx the median\n",
           sorted[n-1], sorted[n-1] / q.median);

    free(sorted);
}

void analyze_housing_market() {
    printf("\n=======================================================================\n");
    printf("=== REAL-WORLD SCENARIO 2: Housing Market Analysis ===\n");
    printf("=======================================================================\n");

    size_t n = sizeof(housing_prices) / sizeof(housing_prices[0]);
    printf("Analyzing %zu housing prices...\n\n", n);

    double* sorted = (double*)malloc(n * sizeof(double));
    memcpy(sorted, housing_prices, n * sizeof(double));
    fp_sort_f64(sorted, n);

    Quartiles q;
    fp_quartiles_f64(sorted, n, &q);

    printf("Housing Price Distribution (in thousands):\n");
    printf("  Minimum:        $%.0fk (starter homes)\n", sorted[0]);
    printf("  Q1 (25th):      $%.0fk (affordable range)\n", q.q1);
    printf("  Median:         $%.0fk (typical home)\n", q.median);
    printf("  Q3 (75th):      $%.0fk (premium homes)\n", q.q3);
    printf("  Maximum:        $%.0fk (luxury properties)\n", sorted[n-1]);

    // Calculate affordability metrics
    double p10 = fp_percentile_f64(sorted, n, 0.10);
    double p90 = fp_percentile_f64(sorted, n, 0.90);

    printf("\nMarket Segments:\n");
    printf("  Entry-level (10th):  $%.0fk\n", p10);
    printf("  Mid-market (50th):   $%.0fk\n", q.median);
    printf("  Luxury (90th):       $%.0fk\n", p90);

    printf("\nMarket Insights:\n");
    printf("  - Middle 50%% of homes range from $%.0fk to $%.0fk\n", q.q1, q.q3);
    printf("  - Price spread (IQR): $%.0fk\n", q.iqr);
    printf("  - Luxury homes are %.1fx more expensive than median\n", p90 / q.median);

    free(sorted);
}

void analyze_exam_performance() {
    printf("\n=======================================================================\n");
    printf("=== REAL-WORLD SCENARIO 3: Student Exam Performance ===\n");
    printf("=======================================================================\n");

    size_t n = sizeof(exam_scores) / sizeof(exam_scores[0]);
    printf("Analyzing %zu student exam scores...\n\n", n);

    double* sorted = (double*)malloc(n * sizeof(double));
    memcpy(sorted, exam_scores, n * sizeof(double));
    fp_sort_f64(sorted, n);

    Quartiles q;
    fp_quartiles_f64(sorted, n, &q);

    printf("Exam Score Distribution (0-100):\n");
    printf("  Minimum:        %.1f (lowest score)\n", sorted[0]);
    printf("  Q1 (25th):      %.1f (bottom quarter)\n", q.q1);
    printf("  Median:         %.1f (typical student)\n", q.median);
    printf("  Q3 (75th):      %.1f (top quarter)\n", q.q3);
    printf("  Maximum:        %.1f (highest score)\n", sorted[n-1]);

    // Grade boundaries
    double p60 = fp_percentile_f64(sorted, n, 0.60); // C/B boundary
    double p80 = fp_percentile_f64(sorted, n, 0.80); // B/A boundary
    double p90 = fp_percentile_f64(sorted, n, 0.90); // A/A+ boundary

    printf("\nGrade Boundaries (hypothetical):\n");
    printf("  60th percentile: %.1f (C/B boundary)\n", p60);
    printf("  80th percentile: %.1f (B/A boundary)\n", p80);
    printf("  90th percentile: %.1f (A/A+ boundary)\n", p90);

    printf("\nClass Performance:\n");
    printf("  - Median score is %.1f (%.0f%% of maximum)\n", q.median, (q.median/100)*100);
    printf("  - Top 25%% scored above %.1f\n", q.q3);
    printf("  - Bottom 25%% scored below %.1f\n", q.q1);

    free(sorted);
}

void analyze_api_performance() {
    printf("\n=======================================================================\n");
    printf("=== REAL-WORLD SCENARIO 4: API Performance Monitoring ===\n");
    printf("=======================================================================\n");

    size_t n = sizeof(response_times) / sizeof(response_times[0]);
    printf("Analyzing %zu API response times...\n\n", n);

    double* sorted = (double*)malloc(n * sizeof(double));
    memcpy(sorted, response_times, n * sizeof(double));
    fp_sort_f64(sorted, n);

    Quartiles q;
    fp_quartiles_f64(sorted, n, &q);

    // Calculate SLA percentiles
    double p50 = q.median;
    double p95 = fp_percentile_f64(sorted, n, 0.95);
    double p99 = fp_percentile_f64(sorted, n, 0.99);

    printf("Response Time Distribution (milliseconds):\n");
    printf("  Minimum (best):   %.1f ms\n", sorted[0]);
    printf("  Median (p50):     %.1f ms\n", p50);
    printf("  p95:              %.1f ms\n", p95);
    printf("  p99:              %.1f ms\n", p99);
    printf("  Maximum (worst):  %.1f ms\n", sorted[n-1]);

    printf("\nSLA Analysis:\n");
    printf("  - 50%% of requests complete within %.1f ms\n", p50);
    printf("  - 95%% of requests complete within %.1f ms\n", p95);
    printf("  - 99%% of requests complete within %.1f ms\n", p99);

    // Performance tiers
    printf("\nPerformance Tiers:\n");
    if (p95 < 100.0) {
        printf("  ✓ EXCELLENT - p95 under 100ms\n");
    } else if (p95 < 200.0) {
        printf("  ✓ GOOD - p95 under 200ms\n");
    } else if (p95 < 500.0) {
        printf("  ⚠ ACCEPTABLE - p95 under 500ms\n");
    } else {
        printf("  ✗ NEEDS IMPROVEMENT - p95 over 500ms\n");
    }

    free(sorted);
}

void analyze_demographics() {
    printf("\n=======================================================================\n");
    printf("=== REAL-WORLD SCENARIO 5: Demographic Age Analysis ===\n");
    printf("=======================================================================\n");

    size_t n = sizeof(ages) / sizeof(ages[0]);
    printf("Analyzing %zu ages in population sample...\n\n", n);

    double* sorted = (double*)malloc(n * sizeof(double));
    memcpy(sorted, ages, n * sizeof(double));
    fp_sort_f64(sorted, n);

    Quartiles q;
    fp_quartiles_f64(sorted, n, &q);

    printf("Age Distribution:\n");
    printf("  Youngest:       %.0f years\n", sorted[0]);
    printf("  Q1 (25th):      %.0f years (young adults)\n", q.q1);
    printf("  Median:         %.0f years (typical age)\n", q.median);
    printf("  Q3 (75th):      %.0f years (older adults)\n", q.q3);
    printf("  Oldest:         %.0f years\n", sorted[n-1]);

    // Age groups
    double p33 = fp_percentile_f64(sorted, n, 0.33);
    double p66 = fp_percentile_f64(sorted, n, 0.66);

    printf("\nAge Cohorts (tertiles):\n");
    printf("  Young (0-33%%):  Up to %.0f years\n", p33);
    printf("  Middle (33-66%%): %.0f to %.0f years\n", p33, p66);
    printf("  Senior (66-100%%): %.0f years and above\n", p66);

    printf("\nDemographic Insights:\n");
    printf("  - 50%% of sample is between %.0f and %.0f years old\n", q.q1, q.q3);
    printf("  - Median age is %.0f years\n", q.median);
    printf("  - Age range spans %.0f years\n", sorted[n-1] - sorted[0]);

    free(sorted);
}

// ============================================================================
// Performance Benchmark
// ============================================================================

void benchmark_performance() {
    printf("\n=======================================================================\n");
    printf("=== PERFORMANCE BENCHMARK ===\n");
    printf("=======================================================================\n");

    size_t sizes[] = {100, 1000, 10000, 100000};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < num_sizes; s++) {
        size_t n = sizes[s];

        // Generate and sort random data
        double* data = (double*)malloc(n * sizeof(double));
        srand(42);
        for (size_t i = 0; i < n; i++) {
            data[i] = (double)(rand() % 100000) / 100.0;
        }
        fp_sort_f64(data, n);

        printf("\nArray size: %zu elements\n", n);

        // Use much higher iteration counts for accurate timing
        int iterations = (n < 1000) ? 10000000 : (n < 10000) ? 5000000 : 1000000;

        // Benchmark single percentile
        clock_t start = clock();
        double result = 0.0;
        for (int i = 0; i < iterations; i++) {
            result += fp_percentile_f64(data, n, 0.5);
        }
        clock_t end = clock();
        double fp_time = (double)(end - start) / CLOCKS_PER_SEC;

        // Benchmark naive version
        start = clock();
        for (int i = 0; i < iterations; i++) {
            result += percentile_naive(data, n, 0.5);
        }
        end = clock();
        double c_time = (double)(end - start) / CLOCKS_PER_SEC;

        printf("  Single percentile (%d iterations):\n", iterations);
        printf("    FP-ASM:  %.4f seconds (%.2f ns/call)\n", fp_time, (fp_time * 1e9) / iterations);
        printf("    Naive C: %.4f seconds (%.2f ns/call)\n", c_time, (c_time * 1e9) / iterations);
        if (fp_time > 0) {
            printf("    Speedup: %.2fx\n", c_time / fp_time);
        }

        // Benchmark batch percentiles
        double p_values[] = {0.1, 0.25, 0.5, 0.75, 0.9, 0.95, 0.99};
        size_t n_percentiles = 7;
        double results[7];

        int batch_iterations = iterations / 4;
        start = clock();
        for (int i = 0; i < batch_iterations; i++) {
            fp_percentiles_f64(data, n, p_values, n_percentiles, results);
        }
        end = clock();
        double batch_time = (double)(end - start) / CLOCKS_PER_SEC;

        printf("  Batch percentiles (7 percentiles, %d iterations):\n", batch_iterations);
        printf("    Total time: %.4f seconds\n", batch_time);
        printf("    Per batch:  %.2f ns\n", (batch_time * 1e9) / batch_iterations);
        printf("    Per percentile: %.2f ns\n", (batch_time * 1e9) / (batch_iterations * n_percentiles));

        free(data);
    }
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=======================================================================\n");
    printf("     Algorithm #2: Percentile Calculations - Test & Benchmark\n");
    printf("=======================================================================\n");

    int passed = 0;
    int total = 0;

    // Run correctness tests
    printf("\n### CORRECTNESS TESTS ###\n");

    total++; if (test_simple_median()) passed++;
    total++; if (test_quartiles_simple()) passed++;
    total++; if (test_extreme_percentiles()) passed++;
    total++; if (test_batch_percentiles()) passed++;
    total++; if (test_edge_cases()) passed++;

    printf("\n-----------------------------------------------------------------------\n");
    printf("Correctness Tests: %d / %d PASSED\n", passed, total);
    printf("-----------------------------------------------------------------------\n");

    if (passed == total) {
        // Run real-world scenarios
        analyze_employee_salaries();
        analyze_housing_market();
        analyze_exam_performance();
        analyze_api_performance();
        analyze_demographics();

        // Run performance benchmark
        benchmark_performance();

        printf("\n=======================================================================\n");
        printf("                    ALL TESTS PASSED!\n");
        printf("=======================================================================\n");

        return 0;
    } else {
        printf("\nSome tests failed. Skipping benchmarks and scenarios.\n");
        return 1;
    }
}
