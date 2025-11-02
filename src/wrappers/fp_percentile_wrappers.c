/**
 * fp_percentile_wrappers.c
 *
 * FP-Pure wrappers for percentile functions
 *
 * These wrappers provide the FP-friendly interface that accepts unsorted data.
 * They handle copying and sorting internally, then delegate to the optimized
 * assembly implementations.
 *
 * Design: Keep assembly pure and fast (works on sorted data), add C wrappers
 * for user convenience and FP compliance.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/fp_core.h"

/* Forward declarations for internal assembly functions (work on sorted data) */
extern double fp_percentile_sorted_f64(const double* sorted_data, size_t n, double p);
extern void fp_percentiles_sorted_f64(const double* sorted_data, size_t n,
                                       const double* p_values, size_t n_percentiles,
                                       double* results);
extern void fp_quartiles_sorted_f64(const double* sorted_data, size_t n, Quartiles* quartiles);
extern size_t fp_detect_outliers_iqr_sorted_f64(const double* sorted_data, size_t n,
                                                  double k_factor, uint8_t* is_outlier);

/* Comparison function for qsort */
static int compare_double(const void* a, const void* b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

/**
 * FP-Pure wrapper: Accepts unsorted data, sorts internally
 */
double fp_percentile_f64(const double* data, size_t n, double p) {
    if (n == 0) return 0.0;
    if (n == 1) return data[0];

    // Allocate temporary buffer
    double* sorted = (double*)malloc(n * sizeof(double));
    if (!sorted) return 0.0;  // Memory allocation failed

    // Copy and sort
    memcpy(sorted, data, n * sizeof(double));
    qsort(sorted, n, sizeof(double), compare_double);

    // Call optimized assembly function with sorted data
    double result = fp_percentile_sorted_f64(sorted, n, p);

    // Cleanup
    free(sorted);

    return result;
}

/**
 * FP-Pure wrapper: Accepts unsorted data, sorts internally once
 */
void fp_percentiles_f64(const double* data, size_t n,
                        const double* p_values, size_t n_percentiles,
                        double* results) {
    if (n == 0 || n_percentiles == 0) return;

    // Allocate temporary buffer
    double* sorted = (double*)malloc(n * sizeof(double));
    if (!sorted) return;  // Memory allocation failed

    // Copy and sort
    memcpy(sorted, data, n * sizeof(double));
    qsort(sorted, n, sizeof(double), compare_double);

    // Call optimized assembly function with sorted data
    fp_percentiles_sorted_f64(sorted, n, p_values, n_percentiles, results);

    // Cleanup
    free(sorted);
}

/**
 * FP-Pure wrapper: Accepts unsorted data, sorts internally
 */
void fp_quartiles_f64(const double* data, size_t n, Quartiles* quartiles) {
    if (n == 0 || !quartiles) return;

    // Allocate temporary buffer
    double* sorted = (double*)malloc(n * sizeof(double));
    if (!sorted) {
        // Memory allocation failed - return zeros
        quartiles->q1 = 0.0;
        quartiles->median = 0.0;
        quartiles->q3 = 0.0;
        quartiles->iqr = 0.0;
        return;
    }

    // Copy and sort
    memcpy(sorted, data, n * sizeof(double));
    qsort(sorted, n, sizeof(double), compare_double);

    // Call optimized assembly function with sorted data
    fp_quartiles_sorted_f64(sorted, n, quartiles);

    // Cleanup
    free(sorted);
}

/**
 * PHASE 4 REFACTORING: Composition-based IQR outlier detection
 *
 * Composes from fp_quartiles_f64 instead of calling assembly
 *
 * Mathematical formula:
 *   IQR = Q3 - Q1
 *   lower_bound = Q1 - k * IQR
 *   upper_bound = Q3 + k * IQR
 *   is_outlier[i] = (data[i] < lower_bound) OR (data[i] > upper_bound)
 */
size_t fp_detect_outliers_iqr_f64(const double* data, size_t n,
                                   double k_factor, uint8_t* is_outlier) {
    if (n < 4 || !is_outlier) return 0;

    // COMPOSITION: Use existing quartiles function!
    Quartiles quartiles;
    fp_quartiles_f64(data, n, &quartiles);

    double q1 = quartiles.q1;
    double q3 = quartiles.q3;
    double iqr = quartiles.iqr;

    // Edge case: No variation in data (IQR = 0)
    if (iqr == 0.0) {
        for (size_t i = 0; i < n; i++) {
            is_outlier[i] = 0;
        }
        return 0;
    }

    // Calculate bounds
    double lower_bound = q1 - k_factor * iqr;
    double upper_bound = q3 + k_factor * iqr;

    // Mark outliers outside bounds
    size_t outlier_count = 0;
    for (size_t i = 0; i < n; i++) {
        if (data[i] < lower_bound || data[i] > upper_bound) {
            is_outlier[i] = 1;
            outlier_count++;
        } else {
            is_outlier[i] = 0;
        }
    }

    return outlier_count;
}

/**
 * PHASE 4 REFACTORING: Composition-based Z-score outlier detection
 *
 * Composes from fp_descriptive_stats_f64 instead of calling assembly
 *
 * Z-score measures how many standard deviations a point is from the mean:
 *   z = (x - mean) / stddev
 *
 * Mathematical formula:
 *   is_outlier[i] = |z[i]| > threshold
 *   where z[i] = (data[i] - mean) / stddev
 *
 * NOTE: GCC 15.1.0 has an optimizer bug with fabs() comparisons at -O3.
 * Using #pragma GCC optimize("O2") as workaround.
 */
#pragma GCC push_options
#pragma GCC optimize("O2")
size_t fp_detect_outliers_zscore_f64(const double* data, size_t n,
                                      double threshold, uint8_t* is_outlier) {
    // Edge case: NULL pointer or insufficient data
    if (!is_outlier || n < 2) {
        return 0;
    }

    // COMPOSITION: Use existing descriptive stats function!
    DescriptiveStats stats;
    fp_descriptive_stats_f64(data, n, &stats);

    // Edge case: All values identical (stddev = 0) or invalid stddev
    if (stats.std_dev == 0.0 || !isfinite(stats.std_dev)) {
        // No outliers when there's no variation
        for (size_t i = 0; i < n; i++) {
            is_outlier[i] = 0;
        }
        return 0;
    }

    // Calculate Z-scores and mark outliers
    size_t outlier_count = 0;
    for (size_t i = 0; i < n; i++) {
        double z_score = (data[i] - stats.mean) / stats.std_dev;

        // Use both positive and negative comparisons to work around GCC 15.1.0 optimizer bug
        // Bug: fabs(x) > threshold gets miscompiled with -O3
        // Workaround: (x > threshold || x < -threshold) is equivalent and works correctly
        if (z_score > threshold || z_score < -threshold) {
            is_outlier[i] = 1;
            outlier_count++;
        } else {
            is_outlier[i] = 0;
        }
    }

    return outlier_count;
}
#pragma GCC pop_options
