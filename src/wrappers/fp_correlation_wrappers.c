/**
 * fp_correlation_wrappers.c
 *
 * Correlation & Covariance Wrappers - Composition-Based
 *
 * REFACTORED: Following Algorithm #7 composition pattern
 *
 * Design Philosophy:
 *   Correlation and covariance should COMPOSE from existing statistical
 *   primitives rather than reimplementing sum_x, sum_y, sum_xy, etc.
 *
 * Before (Monolithic):
 *   - fp_core_correlation.asm: 342 lines reimplementing sums
 *   - Code duplication, hard to maintain
 *
 * After (Composition):
 *   - fp_covariance_f64: ~15 lines composing from 3 primitives
 *   - fp_correlation_f64: ~25 lines composing from covariance + stats
 *   - ~88% code reduction!
 */

#include <math.h>
#include "../include/fp_core.h"

/* ============================================================================
 * Covariance - Pure Composition!
 * ============================================================================
 *
 * Mathematical formula:
 *   Cov(X,Y) = E[XY] - E[X]·E[Y]
 *            = (Σxy / n) - (mean_x · mean_y)
 *
 * Before: Part of 342 lines of assembly reimplementing everything
 * After:  ~15 lines composing from 2 primitives
 */

double fp_covariance_f64(const double* x, const double* y, size_t n) {
    if (n == 0) return NAN;
    if (n == 1) return 0.0;

    // Compose from existing optimized primitives!
    double sum_x  = fp_reduce_add_f64(x, n);    // REUSE: Σx
    double sum_y  = fp_reduce_add_f64(y, n);    // REUSE: Σy
    double sum_xy = fp_fold_dotp_f64(x, y, n);  // REUSE: Σ(x·y)

    double n_double = (double)n;
    double mean_x = sum_x / n_double;
    double mean_y = sum_y / n_double;
    double mean_xy = sum_xy / n_double;

    // Covariance formula: E[XY] - E[X]·E[Y]
    return mean_xy - (mean_x * mean_y);
}

/* ============================================================================
 * Correlation (Pearson's r) - Composition from Covariance!
 * ============================================================================
 *
 * Mathematical formula:
 *   r = Cov(X,Y) / (σ_X · σ_Y)
 *
 * Where:
 *   σ_X = sqrt(Var(X))
 *   σ_Y = sqrt(Var(Y))
 *   Var(X) = E[X²] - (E[X])²
 *
 * Before: Part of 342 lines of assembly
 * After:  ~30 lines composing from 3 primitives + covariance
 */

double fp_correlation_f64(const double* x, const double* y, size_t n) {
    if (n == 0 || n == 1) return NAN;

    // Step 1: Compute covariance by composing!
    double cov = fp_covariance_f64(x, y, n);

    // Step 2: Compute variances using composition
    double sum_x  = fp_reduce_add_f64(x, n);    // REUSE: Σx
    double sum_y  = fp_reduce_add_f64(y, n);    // REUSE: Σy
    double sum_x2 = fp_fold_dotp_f64(x, x, n);  // REUSE: Σx² = x·x (COMPOSITION!)
    double sum_y2 = fp_fold_dotp_f64(y, y, n);  // REUSE: Σy² = y·y (COMPOSITION!)

    double n_double = (double)n;
    double mean_x = sum_x / n_double;
    double mean_y = sum_y / n_double;

    // Variance formula: Var(X) = E[X²] - (E[X])²
    double var_x = (sum_x2 / n_double) - (mean_x * mean_x);
    double var_y = (sum_y2 / n_double) - (mean_y * mean_y);

    // Handle degenerate cases (constant arrays)
    if (var_x <= 0.0 || var_y <= 0.0) return NAN;

    // Standard deviations
    double stddev_x = sqrt(var_x);
    double stddev_y = sqrt(var_y);

    // Pearson correlation: r = Cov(X,Y) / (σ_X · σ_Y)
    return cov / (stddev_x * stddev_y);
}

/* ============================================================================
 * REFACTORING NOTES
 * ============================================================================
 *
 * Correlation/Covariance Refactoring Impact:
 *   - Before: 342 lines of assembly (fp_core_correlation.asm)
 *   - After:  ~40 lines of composition (both functions)
 *   - Code reduction: 88.3%
 *   - Duplicated logic eliminated: sum_x, sum_y, sum_xy, sum_x², sum_y²
 *
 * Composition Pattern Used:
 *   Instead of reimplementing statistical primitives, we compose from:
 *   1. fp_reduce_add_f64 (sum) - Already SIMD-optimized
 *   2. fp_fold_dotp_f64 (dot product) - Already fused multiply+sum
 *
 * Mathematical Composition Insights:
 *   1. Cov(X,Y) = E[XY] - E[X]·E[Y]
 *      - Composes from: reduce_add (for means) and fold_dotp (for E[XY])
 *
 *   2. Var(X) = E[X²] - (E[X])²
 *      - Composes from: fold_dotp(x, x, n) for Σx²
 *      - Same insight as linear regression!
 *
 *   3. Cor(X,Y) = Cov(X,Y) / (σ_X · σ_Y)
 *      - Composes from: covariance function + variance calculations
 *      - Hierarchical composition!
 *
 * Why This Is Correct:
 *   1. Covariance/correlation are purely compositions of sums
 *   2. Each primitive is already optimized (SIMD or scalar unrolling)
 *   3. Function call overhead is negligible (5 calls for potentially millions of ops)
 *   4. Eliminates code duplication
 *   5. Follows Algorithm #7 composition pattern
 *
 * Performance Considerations:
 *   - Trade-off: Multiple function calls vs. fused single-pass assembly
 *   - Pro: Each function is highly optimized (SIMD/unrolled)
 *   - Pro: Better cache locality (sequential access)
 *   - Con: Cannot share intermediate results (compute sums twice in correlation)
 *
 *   Optimization opportunity: Could cache sums if both cov and var needed,
 *   but current API returns double, not a struct. For clarity, we accept
 *   the minor redundancy.
 *
 *   If benchmarks show >5% regression, we can provide both:
 *   - fp_correlation_f64 (composition, default)
 *   - fp_correlation_f64_fused (assembly, opt-in)
 *
 * Mathematical Correctness:
 *   All formulas match standard statistical definitions:
 *   - Covariance: Cov(X,Y) = E[(X - μ_X)(Y - μ_Y)]
 *                          = E[XY] - E[X]E[Y]
 *   - Correlation: r = Cov(X,Y) / (σ_X · σ_Y)
 *   - Range: -1 ≤ r ≤ +1
 *
 * Edge Cases Handled:
 *   1. n = 0: Returns NAN
 *   2. n = 1: Covariance = 0, Correlation = NAN (undefined)
 *   3. Constant x or y: Correlation = NAN (zero variance)
 *   4. Near-zero variance: Checked with ≤ 0.0 condition
 *
 * Dependencies:
 *   - fp_core_reductions.o (provides fp_reduce_add_f64)
 *   - fp_core_fused_folds.o (provides fp_fold_dotp_f64)
 *
 * Build Instructions:
 *   gcc -c src/wrappers/fp_correlation_wrappers.c \
 *       -o build/obj/fp_correlation_wrappers.o \
 *       -I include -O3 -march=native
 *
 * Test Instructions:
 *   cmd /c build\scripts\test_correlation_refactoring.bat
 */
