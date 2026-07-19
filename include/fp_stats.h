/**
 * fp_stats.h - Array Statistics Pattern (Header-Only Template)
 *
 * Production-ready statistical functions using FP patterns.
 * All functions are static inline for zero overhead.
 *
 * USAGE:
 *   #include "fp_stats.h"
 *
 *   double data[] = {1, 2, 3, 4, 5};
 *   double mean = fp_mean(data, 5);
 *   SummaryStats stats = fp_summary_stats(data, 5);
 */

#ifndef FP_STATS_H
#define FP_STATS_H

#include "fp_compose_inline.h"
#include "fp_monads_inline.h"
#include <math.h>

/* ============================================================================
 * BASIC STATISTICS
 * ============================================================================ */

/**
 * Mean: average of array
 */
static inline double fp_mean(const double* data, size_t n) {
    if (!data || n == 0) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) sum += data[i];
    return sum / (double)n;
}

/**
 * Variance: average of squared differences from mean
 */
static inline double fp_variance(const double* data, size_t n, double mean) {
    if (!data || n == 0) return 0.0;
    double sum_sq = 0.0;
    for (size_t i = 0; i < n; i++) {
        double diff = data[i] - mean;
        sum_sq += diff * diff;
    }
    return sum_sq / (double)n;
}

/**
 * Standard deviation
 */
static inline double fp_std(const double* data, size_t n, double mean) {
    return sqrt(fp_variance(data, n, mean));
}

/**
 * Mean and variance in single pass (Welford's algorithm - numerically stable)
 */
typedef struct {
    double mean;
    double variance;
    size_t count;
} MeanVarianceResult;

static inline MeanVarianceResult fp_mean_variance_welford(const double* data, size_t n) {
    MeanVarianceResult result = {0.0, 0.0, 0};
    if (!data || n == 0) return result;

    double mean = 0.0;
    double m2 = 0.0;

    for (size_t i = 0; i < n; i++) {
        double delta = data[i] - mean;
        mean += delta / (double)(i + 1);
        double delta2 = data[i] - mean;
        m2 += delta * delta2;
    }

    result.mean = mean;
    result.variance = (n > 0) ? (m2 / (double)n) : 0.0;
    result.count = n;
    return result;
}

/**
 * Min and max in single pass
 */
typedef struct {
    double min;
    double max;
} MinMaxResult;

static inline MinMaxResult fp_min_max(const double* data, size_t n) {
    MinMaxResult result = {INFINITY, -INFINITY};
    if (!data || n == 0) return result;

    result.min = data[0];
    result.max = data[0];

    for (size_t i = 1; i < n; i++) {
        if (data[i] < result.min) result.min = data[i];
        if (data[i] > result.max) result.max = data[i];
    }
    return result;
}

/* ============================================================================
 * PAIRWISE STATISTICS
 * ============================================================================ */

/**
 * Covariance between two arrays
 */
static inline double fp_covariance(const double* x, const double* y, size_t n,
                                    double mean_x, double mean_y) {
    if (!x || !y || n == 0) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum += (x[i] - mean_x) * (y[i] - mean_y);
    }
    return sum / (double)n;
}

/* ============================================================================
 * VECTOR NORMS & DISTANCES
 * ============================================================================ */

/**
 * L2 norm (Euclidean length)
 */
static inline double fp_l2_norm(const double* v, size_t n) {
    if (!v || n == 0) return 0.0;
    double sum_sq = 0.0;
    for (size_t i = 0; i < n; i++) sum_sq += v[i] * v[i];
    return sqrt(sum_sq);
}

/**
 * Euclidean distance between two vectors
 */
static inline double fp_euclidean_distance(const double* x, const double* y, size_t n) {
    if (!x || !y || n == 0) return 0.0;
    double sum_sq = 0.0;
    for (size_t i = 0; i < n; i++) {
        double diff = x[i] - y[i];
        sum_sq += diff * diff;
    }
    return sqrt(sum_sq);
}

/**
 * Dot product
 */
static inline double fp_dot_product(const double* x, const double* y, size_t n) {
    if (!x || !y || n == 0) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) sum += x[i] * y[i];
    return sum;
}

/* ============================================================================
 * NORMALIZATION (Type-Safe with Maybe)
 * ============================================================================ */

/**
 * Normalize to [0, 1] (min-max scaling)
 * Returns: Maybe (Nothing if constant array)
 */
static inline Maybe fp_normalize_min_max(const double* input, double* output, size_t n) {
    if (!input || !output || n == 0) return fp_nothing_inline();

    MinMaxResult mm = fp_min_max(input, n);
    if (mm.max == mm.min) return fp_nothing_inline();  // Constant array

    double range = mm.max - mm.min;
    for (size_t i = 0; i < n; i++) {
        output[i] = (input[i] - mm.min) / range;
    }
    return fp_just_f64_inline(range);
}

/**
 * Standardize to mean=0, std=1 (Z-score normalization)
 * Returns: Maybe (Nothing if constant array)
 */
static inline Maybe fp_standardize(const double* input, double* output, size_t n) {
    if (!input || !output || n == 0) return fp_nothing_inline();

    MeanVarianceResult mv = fp_mean_variance_welford(input, n);
    double std = sqrt(mv.variance);

    if (std == 0.0) return fp_nothing_inline();  // Constant array

    for (size_t i = 0; i < n; i++) {
        output[i] = (input[i] - mv.mean) / std;
    }
    return fp_just_f64_inline(std);
}

/* ============================================================================
 * COMPREHENSIVE SUMMARY
 * ============================================================================ */

typedef struct {
    double mean;
    double variance;
    double std;
    double min;
    double max;
    double range;
    size_t count;
} SummaryStats;

static inline SummaryStats fp_summary_stats(const double* data, size_t n) {
    SummaryStats stats = {0.0, 0.0, 0.0, INFINITY, -INFINITY, 0.0, 0};
    if (!data || n == 0) return stats;

    MeanVarianceResult mv = fp_mean_variance_welford(data, n);
    MinMaxResult mm = fp_min_max(data, n);

    stats.mean = mv.mean;
    stats.variance = mv.variance;
    stats.std = sqrt(mv.variance);
    stats.min = mm.min;
    stats.max = mm.max;
    stats.range = mm.max - mm.min;
    stats.count = n;

    return stats;
}

#endif /* FP_STATS_H */
