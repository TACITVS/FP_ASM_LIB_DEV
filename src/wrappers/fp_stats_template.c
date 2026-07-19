/**
 * fp_stats_template.c - Array Statistics Pattern Template
 *
 * Reusable FP patterns for common statistical operations.
 * Uses fused map-reduce for single-pass, zero-copy computation.
 *
 * USAGE:
 *   Include this file or copy functions to your algorithm.
 *   All functions use inline-optimized FP operations for maximum performance.
 *
 * ALGORITHMS USING THIS PATTERN:
 *   - K-means (distance statistics)
 *   - Linear regression (loss computation)
 *   - PCA (covariance matrix)
 *   - Decision trees (entropy, gini)
 *   - Naive Bayes (feature statistics)
 *   - Neural networks (batch statistics)
 *   - Time series (moving averages)
 *   - Monte Carlo (sampling statistics)
 *   - FFT (signal statistics)
 *   - Ray tracing (pixel statistics)
 */

#include "../../include/fp_compose_inline.h"
#include "../../include/fp_monads_inline.h"
#include <math.h>
#include <stdio.h>

/* ============================================================================
 * BASIC STATISTICS (Single Pass with Fused Operations)
 * ============================================================================ */

/**
 * Compute mean of array
 *
 * Performance: O(n), single pass
 * Memory: O(1), no temporary arrays
 */
static inline double fp_mean(const double* data, size_t n) {
    if (!data || n == 0) return 0.0;

    double add(double acc, double x) { return acc + x; }
    double sum = fp_simple_reduce_f64_inline(data, n, 0.0, add);

    return sum / (double)n;
}

/**
 * Compute variance (biased: divide by n)
 *
 * Performance: O(n), requires 2 passes (1 for mean, 1 for variance)
 * For single-pass variance, use fp_mean_variance_welford (below)
 */
static inline double fp_variance(const double* data, size_t n, double mean) {
    if (!data || n == 0) return 0.0;

    double sum_sq_diff = 0.0;
    for (size_t i = 0; i < n; i++) {
        double diff = data[i] - mean;
        sum_sq_diff += diff * diff;
    }

    return sum_sq_diff / (double)n;
}

/**
 * Compute standard deviation
 */
static inline double fp_std(const double* data, size_t n, double mean) {
    return sqrt(fp_variance(data, n, mean));
}

/**
 * Compute mean and variance in single pass (Welford's algorithm)
 *
 * Performance: O(n), single pass, numerically stable
 * Memory: O(1)
 *
 * This is the PREFERRED method for computing mean+variance together.
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
    double m2 = 0.0;  // Sum of squared differences

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
 * Compute min and max in single pass
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
 * PAIRWISE STATISTICS (Covariance, Correlation)
 * ============================================================================ */

/**
 * Compute covariance between two arrays
 *
 * cov(X, Y) = E[(X - μX)(Y - μY)]
 *
 * Performance: O(n), single pass after means computed
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

/**
 * Compute Pearson correlation coefficient
 *
 * corr(X, Y) = cov(X, Y) / (σX * σY)
 *
 * Returns: Maybe (Nothing if either std is zero)
 */
static inline Maybe fp_correlation(const double* x, const double* y, size_t n) {
    if (!x || !y || n == 0) return fp_nothing_inline();

    // Compute means
    double mean_x = fp_mean(x, n);
    double mean_y = fp_mean(y, n);

    // Compute covariance
    double cov = fp_covariance(x, y, n, mean_x, mean_y);

    // Compute standard deviations
    double std_x = fp_std(x, n, mean_x);
    double std_y = fp_std(y, n, mean_y);

    // Safe division
    Maybe result = fp_safe_divide_f64_inline(cov, std_x * std_y);

    return result;
}

/* ============================================================================
 * VECTOR NORMS & DISTANCES
 * ============================================================================ */

/**
 * Compute L2 norm (Euclidean length) of vector
 *
 * ||v||₂ = sqrt(sum(v²))
 */
static inline double fp_l2_norm(const double* v, size_t n) {
    if (!v || n == 0) return 0.0;

    double square(double x) { return x * x; }
    double add(double acc, double x) { return acc + x; }

    double sum_sq = fp_fused_map_reduce_f64_inline(v, n, square, 0.0, add);

    return sqrt(sum_sq);
}

/**
 * Compute L1 norm (Manhattan distance)
 *
 * ||v||₁ = sum(|v|)
 */
static inline double fp_l1_norm(const double* v, size_t n) {
    if (!v || n == 0) return 0.0;

    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum += fabs(v[i]);
    }

    return sum;
}

/**
 * Compute Euclidean distance between two vectors
 *
 * d(x, y) = sqrt(sum((x - y)²))
 *
 * Performance: Single pass, fused map-reduce
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
 * Compute Manhattan distance between two vectors
 */
static inline double fp_manhattan_distance(const double* x, const double* y, size_t n) {
    if (!x || !y || n == 0) return 0.0;

    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum += fabs(x[i] - y[i]);
    }

    return sum;
}

/**
 * Compute dot product
 *
 * x · y = sum(xᵢ * yᵢ)
 */
static inline double fp_dot_product(const double* x, const double* y, size_t n) {
    if (!x || !y || n == 0) return 0.0;

    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum += x[i] * y[i];
    }

    return sum;
}

/**
 * Compute cosine similarity
 *
 * sim(x, y) = (x · y) / (||x|| * ||y||)
 *
 * Returns: Maybe (Nothing if either vector has zero norm)
 */
static inline Maybe fp_cosine_similarity(const double* x, const double* y, size_t n) {
    if (!x || !y || n == 0) return fp_nothing_inline();

    double dot = fp_dot_product(x, y, n);
    double norm_x = fp_l2_norm(x, n);
    double norm_y = fp_l2_norm(y, n);

    return fp_safe_divide_f64_inline(dot, norm_x * norm_y);
}

/* ============================================================================
 * ENTROPY & INFORMATION THEORY (for Decision Trees, Naive Bayes)
 * ============================================================================ */

/**
 * Compute Shannon entropy
 *
 * H(X) = -sum(p(x) * log2(p(x)))
 *
 * Input: probabilities (must sum to 1.0)
 * Returns: Maybe (Nothing if any probability is negative or > 1)
 */
static inline Maybe fp_entropy(const double* probabilities, size_t n) {
    if (!probabilities || n == 0) return fp_nothing_inline();

    // Validate probabilities
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        if (probabilities[i] < 0.0 || probabilities[i] > 1.0) {
            return fp_nothing_inline();  // Invalid probability
        }
        sum += probabilities[i];
    }

    // Allow small tolerance for floating-point errors
    if (fabs(sum - 1.0) > 1e-6) {
        return fp_nothing_inline();  // Probabilities don't sum to 1
    }

    // Compute entropy
    double entropy = 0.0;
    for (size_t i = 0; i < n; i++) {
        if (probabilities[i] > 0.0) {  // Skip zero probabilities (0 * log(0) = 0)
            entropy -= probabilities[i] * log2(probabilities[i]);
        }
    }

    return fp_just_f64_inline(entropy);
}

/**
 * Compute Gini impurity
 *
 * Gini(X) = 1 - sum(p(x)²)
 *
 * Input: probabilities (must sum to 1.0)
 */
static inline Maybe fp_gini_impurity(const double* probabilities, size_t n) {
    if (!probabilities || n == 0) return fp_nothing_inline();

    // Validate and compute
    double sum = 0.0;
    double sum_sq = 0.0;

    for (size_t i = 0; i < n; i++) {
        if (probabilities[i] < 0.0 || probabilities[i] > 1.0) {
            return fp_nothing_inline();
        }
        sum += probabilities[i];
        sum_sq += probabilities[i] * probabilities[i];
    }

    if (fabs(sum - 1.0) > 1e-6) {
        return fp_nothing_inline();
    }

    double gini = 1.0 - sum_sq;

    return fp_just_f64_inline(gini);
}

/* ============================================================================
 * NORMALIZATION & SCALING (Type-Safe with Maybe)
 * ============================================================================ */

/**
 * Normalize array to [0, 1] range (min-max scaling)
 *
 * x' = (x - min) / (max - min)
 *
 * Returns: Maybe (Nothing if max == min, i.e., constant array)
 */
static inline Maybe fp_normalize_min_max(const double* input, double* output, size_t n) {
    if (!input || !output || n == 0) return fp_nothing_inline();

    MinMaxResult mm = fp_min_max(input, n);

    // Check if array is constant
    if (mm.max == mm.min) {
        return fp_nothing_inline();  // Cannot normalize constant array
    }

    double range = mm.max - mm.min;

    for (size_t i = 0; i < n; i++) {
        output[i] = (input[i] - mm.min) / range;
    }

    return fp_just_f64_inline(range);  // Return range as success value
}

/**
 * Standardize array to mean=0, std=1 (Z-score normalization)
 *
 * x' = (x - μ) / σ
 *
 * Returns: Maybe (Nothing if std == 0, i.e., constant array)
 */
static inline Maybe fp_standardize(const double* input, double* output, size_t n) {
    if (!input || !output || n == 0) return fp_nothing_inline();

    MeanVarianceResult mv = fp_mean_variance_welford(input, n);
    double std = sqrt(mv.variance);

    // Check if array is constant
    if (std == 0.0) {
        return fp_nothing_inline();  // Cannot standardize constant array
    }

    for (size_t i = 0; i < n; i++) {
        output[i] = (input[i] - mv.mean) / std;
    }

    return fp_just_f64_inline(std);  // Return std as success value
}

/* ============================================================================
 * COMPREHENSIVE STATISTICS STRUCT (All-in-One)
 * ============================================================================ */

/**
 * Complete statistical summary of array
 *
 * Computes: mean, variance, std, min, max, range
 * Performance: O(n), minimal passes (Welford + min/max in parallel)
 */
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

    // Compute mean and variance (Welford's algorithm)
    MeanVarianceResult mv = fp_mean_variance_welford(data, n);

    // Compute min and max
    MinMaxResult mm = fp_min_max(data, n);

    // Fill result
    stats.mean = mv.mean;
    stats.variance = mv.variance;
    stats.std = sqrt(mv.variance);
    stats.min = mm.min;
    stats.max = mm.max;
    stats.range = mm.max - mm.min;
    stats.count = n;

    return stats;
}

/**
 * Print summary statistics (debugging/reporting)
 */
static inline void fp_print_summary_stats(const SummaryStats* stats, const char* label) {
    if (!stats) return;

    printf("\n=== Summary Statistics: %s ===\n", label ? label : "Data");
    printf("  Count:    %zu\n", stats->count);
    printf("  Mean:     %.6f\n", stats->mean);
    printf("  Std Dev:  %.6f\n", stats->std);
    printf("  Variance: %.6f\n", stats->variance);
    printf("  Min:      %.6f\n", stats->min);
    printf("  Max:      %.6f\n", stats->max);
    printf("  Range:    %.6f\n", stats->range);
    printf("\n");
}
