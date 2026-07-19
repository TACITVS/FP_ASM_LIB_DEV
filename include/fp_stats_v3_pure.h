/**
 * fp_stats_v3_pure.h - Pattern 1 PURE FP (ZERO for loops!)
 *
 * This is the ULTIMATE version:
 * - NO for loops in user-facing code
 * - ALL operations expressed as reduce/map/fold/recursion
 * - Uses assembly primitives + FP combinators
 * - Pure functional API (functions as values)
 *
 * Philosophy: "Even assembly is syntactic sugar for electrons!"
 * At every level, we choose the abstraction. Here, we choose PURE FP.
 *
 * Benefits:
 * - Declarative (WHAT, not HOW)
 * - Composable (chain operations)
 * - Parallelizable (no mutation)
 * - Mathematically clear (reads like formulas)
 */

#ifndef FP_STATS_V3_PURE_H
#define FP_STATS_V3_PURE_H

#include "fp_core.h"
#include "fp_compose_inline.h"
#include "fp_monads_inline.h"
#include <math.h>

/* ============================================================================
 * PURE FP COMBINATORS (Building Blocks)
 * ============================================================================ */

// NOTE: fp_id and fp_compose_f64 are already defined in fp_compose.h
// We don't need to redefine them here.
typedef double (*UnaryF64)(double);

/* ============================================================================
 * BASIC STATISTICS (PURE FP - No Loops!)
 * ============================================================================ */

/**
 * Mean: Pure FP using reduce
 *
 * mean(xs) = reduce(add, 0, xs) / length(xs)
 *
 * NO loops! Uses assembly reduce primitive.
 */
static inline double fp_mean_pure(const double* data, size_t n) {
    if (!data || n == 0) return 0.0;

    // Pure FP: sum = reduce add 0 xs
    double sum = fp_reduce_add_f64(data, n);
    return sum / (double)n;
}

/**
 * Variance helper: Tail recursive fold (compiler optimizes to loop)
 */
static inline double variance_fold_helper(const double* data, size_t n,
                                           size_t i, double mean_val, double acc) {
    if (i >= n) return acc / (double)n;  // Base case

    // Compute squared difference
    double diff = data[i] - mean_val;
    double squared_diff = diff * diff;

    // Tail recursive call
    return variance_fold_helper(data, n, i + 1, mean_val, acc + squared_diff);
}

/**
 * Variance: Pure FP using tail recursion
 *
 * variance(xs, mean) = fold (\acc x -> acc + (x-mean)²) 0 xs / n
 *
 * NO loops! Uses tail recursion (compiler optimizes).
 */
static inline double fp_variance_pure(const double* data, size_t n, double mean) {
    if (!data || n == 0) return 0.0;

    // Pure FP: fold (accumulate squared differences)
    return variance_fold_helper(data, n, 0, mean, 0.0);
}

/**
 * Standard deviation: Pure function composition
 *
 * std = sqrt . variance
 */
static inline double fp_std_pure(const double* data, size_t n, double mean) {
    return sqrt(fp_variance_pure(data, n, mean));
}

/* ============================================================================
 * WELFORD'S ALGORITHM (Tail Recursive!)
 * ============================================================================ */

/**
 * Mean + Variance using tail recursion
 *
 * This is the ONLY place we allow "internal" iteration,
 * but it's expressed as tail recursion (compiler optimizes to loop)
 */

// State for Welford's algorithm
typedef struct {
    double mean;
    double m2;
    size_t count;
} WelfordState;

// Step function (pure!)
static inline WelfordState welford_step(WelfordState state, double x) {
    size_t new_count = state.count + 1;
    double delta = x - state.mean;
    double new_mean = state.mean + delta / (double)new_count;
    double delta2 = x - new_mean;
    double new_m2 = state.m2 + delta * delta2;

    return (WelfordState){ new_mean, new_m2, new_count };
}

// Tail recursive fold (compiler optimizes to loop)
static inline WelfordState welford_fold_helper(const double* data, size_t n,
                                                 size_t i, WelfordState state) {
    if (i >= n) return state;  // Base case

    // Tail recursive call
    return welford_fold_helper(data, n, i + 1, welford_step(state, data[i]));
}

typedef struct {
    double mean;
    double variance;
    size_t count;
} MeanVarianceResult;

static inline MeanVarianceResult fp_mean_variance_pure(const double* data, size_t n) {
    if (!data || n == 0) {
        return (MeanVarianceResult){0.0, 0.0, 0};
    }

    // Pure FP: fold welford_step initial_state xs
    WelfordState initial = {0.0, 0.0, 0};
    WelfordState final = welford_fold_helper(data, n, 0, initial);

    return (MeanVarianceResult){
        final.mean,
        (final.count > 0) ? (final.m2 / (double)final.count) : 0.0,
        final.count
    };
}

/* ============================================================================
 * VECTOR OPERATIONS (Pure FP - No Loops!)
 * ============================================================================ */

/**
 * Dot product: Pure FP using fold
 *
 * dot(xs, ys) = fold (\acc (x, y) -> acc + x*y) 0 (zip xs ys)
 *
 * NO loops! Uses assembly fold_dotp primitive.
 */
static inline double fp_dot_product_pure(const double* x, const double* y, size_t n) {
    if (!x || !y || n == 0) return 0.0;

    // Pure FP: fold dotp 0 (zip xs ys)
    return fp_fold_dotp_f64(x, y, n);
}

/**
 * Sum of squares helper: Tail recursive fold (compiler optimizes to loop)
 */
static inline double sumsq_fold_helper(const double* v, size_t n, size_t i, double acc) {
    if (i >= n) return acc;  // Base case
    double val = v[i];
    return sumsq_fold_helper(v, n, i + 1, acc + val * val);  // Tail call
}

/**
 * L2 norm: Pure FP using (sqrt . fold_sumsq)
 *
 * norm(xs) = sqrt (fold (\acc x -> acc + x²) 0 xs)
 *
 * NO loops! Uses tail recursion (compiler optimizes).
 */
static inline double fp_l2_norm_pure(const double* v, size_t n) {
    if (!v || n == 0) return 0.0;

    // Pure FP: sqrt . fold (\acc x -> acc + x*x) 0
    return sqrt(sumsq_fold_helper(v, n, 0, 0.0));
}

/**
 * Euclidean distance: Pure FP using recursion
 *
 * dist(xs, ys) = sqrt (sum_squares_diff xs ys 0 0)
 *
 * Tail recursive (compiler optimizes to loop)
 */
static inline double dist_helper(const double* x, const double* y,
                                  size_t n, size_t i, double acc) {
    if (i >= n) return sqrt(acc);  // Base case

    double diff = x[i] - y[i];
    return dist_helper(x, y, n, i + 1, acc + diff * diff);  // Tail call
}

static inline double fp_euclidean_distance_pure(const double* x, const double* y, size_t n) {
    if (!x || !y || n == 0) return 0.0;
    return dist_helper(x, y, n, 0, 0.0);
}

/* ============================================================================
 * PAIRWISE STATISTICS (Pure FP with Zip)
 * ============================================================================ */

/**
 * Covariance: Pure FP using zip + map + reduce
 *
 * cov(xs, ys, mx, my) = reduce add 0 (map (\(x,y) -> (x-mx)*(y-my)) (zip xs ys)) / n
 *
 * Expressed as tail recursion
 */
static inline double cov_helper(const double* x, const double* y, size_t n,
                                 size_t i, double mx, double my, double acc) {
    if (i >= n) return acc / (double)n;  // Base case

    double term = (x[i] - mx) * (y[i] - my);
    return cov_helper(x, y, n, i + 1, mx, my, acc + term);  // Tail call
}

static inline double fp_covariance_pure(const double* x, const double* y, size_t n,
                                         double mean_x, double mean_y) {
    if (!x || !y || n == 0) return 0.0;
    return cov_helper(x, y, n, 0, mean_x, mean_y, 0.0);
}

/* ============================================================================
 * MIN/MAX (Pure FP using Fold)
 * ============================================================================ */

typedef struct {
    double min;
    double max;
} MinMaxResult;

/**
 * Min/Max: Pure FP using reduce
 *
 * NO loops! Uses assembly reduce primitives.
 */
static inline MinMaxResult fp_min_max_pure(const double* data, size_t n) {
    MinMaxResult result = {INFINITY, -INFINITY};
    if (!data || n == 0) return result;

    // Pure FP: parallel reduce (can run simultaneously!)
    result.min = fp_reduce_min_f64(data, n);
    result.max = fp_reduce_max_f64(data, n);

    return result;
}

/* ============================================================================
 * NORMALIZATION (Pure FP with Map)
 * ============================================================================ */

/**
 * Normalize helper: Tail recursive map (compiler optimizes to loop)
 */
static inline void normalize_map_helper(const double* in, double* out, size_t n,
                                         size_t i, double min_val, double range) {
    if (i >= n) return;  // Base case
    out[i] = (in[i] - min_val) / range;
    normalize_map_helper(in, out, n, i + 1, min_val, range);  // Tail call
}

/**
 * Normalize: Pure FP using map
 *
 * normalize(xs) = map (\x -> (x - min) / range) xs
 *
 * Returns Maybe (Nothing if constant array)
 */
static inline Maybe fp_normalize_pure(const double* input, double* output, size_t n) {
    if (!input || !output || n == 0) return fp_nothing_inline();

    MinMaxResult mm = fp_min_max_pure(input, n);
    if (mm.max == mm.min) return fp_nothing_inline();

    double range = mm.max - mm.min;

    // Pure FP: map (\x -> (x - min) / range) xs
    normalize_map_helper(input, output, n, 0, mm.min, range);
    return fp_just_f64_inline(range);
}

/**
 * Standardize helper: Tail recursive map (compiler optimizes to loop)
 */
static inline void standardize_map_helper(const double* in, double* out, size_t n,
                                           size_t i, double mean_val, double std_val) {
    if (i >= n) return;  // Base case
    out[i] = (in[i] - mean_val) / std_val;
    standardize_map_helper(in, out, n, i + 1, mean_val, std_val);  // Tail call
}

/**
 * Standardize: Pure FP using map
 *
 * standardize(xs) = let (mean, var) = mean_variance xs
 *                   in map (\x -> (x - mean) / sqrt(var)) xs
 */
static inline Maybe fp_standardize_pure(const double* input, double* output, size_t n) {
    if (!input || !output || n == 0) return fp_nothing_inline();

    MeanVarianceResult mv = fp_mean_variance_pure(input, n);
    double std = sqrt(mv.variance);

    if (std == 0.0) return fp_nothing_inline();

    // Pure FP: map (\x -> (x - mean) / std) xs
    standardize_map_helper(input, output, n, 0, mv.mean, std);
    return fp_just_f64_inline(std);
}

/* ============================================================================
 * SUMMARY STATISTICS (Pure FP Composition)
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

/**
 * Summary: Pure FP composition of primitives
 *
 * This composes multiple FP operations:
 * - mean_variance (fold with Welford)
 * - min_max (parallel reduce)
 * - sqrt (function composition)
 */
static inline SummaryStats fp_summary_stats_pure(const double* data, size_t n) {
    SummaryStats stats = {0.0, 0.0, 0.0, INFINITY, -INFINITY, 0.0, 0};
    if (!data || n == 0) return stats;

    // Pure FP: compose multiple primitives
    MeanVarianceResult mv = fp_mean_variance_pure(data, n);
    MinMaxResult mm = fp_min_max_pure(data, n);

    stats.mean = mv.mean;
    stats.variance = mv.variance;
    stats.std = sqrt(mv.variance);  // Function composition: sqrt . variance
    stats.min = mm.min;
    stats.max = mm.max;
    stats.range = mm.max - mm.min;
    stats.count = n;

    return stats;
}

/* ============================================================================
 * PHILOSOPHY: Why This Works
 * ============================================================================ */

/*
 * User asks: "Can we finally be done with imperative FOR loops?"
 *
 * Answer: YES! At the API level, we express EVERYTHING as:
 *
 * 1. REDUCE/FOLD - Aggregate values
 *    sum = reduce(add, 0, array)
 *
 * 2. MAP - Transform each element
 *    doubled = map(double, array)
 *
 * 3. RECURSION - Tail-optimized (compiler turns into loops)
 *    sum_helper(arr, n, i, acc) = if i>=n then acc else sum_helper(arr, n, i+1, acc+arr[i])
 *
 * 4. COMPOSITION - Chain functions
 *    std = sqrt . variance . mean
 *
 * 5. PRIMITIVES - Assembly SIMD operations
 *    sum = fp_reduce_add_f64(array, n)  // No loop visible to user!
 *
 * The loops still exist at LOWER levels:
 * - Assembly primitives have SIMD loops
 * - Compiler optimizes tail recursion to loops
 * - CPU executes loops in microcode
 * - Transistors switch in cycles
 *
 * But at OUR level (the API), it's PURE FP!
 *
 * As you said: "Even assembly is syntactic sugar for electrons and holes!"
 *
 * We choose the abstraction level. Here, we choose PURE FP. ✨
 */

#endif /* FP_STATS_V3_PURE_H */
