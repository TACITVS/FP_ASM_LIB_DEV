/**
 * fp_regression_wrappers.c
 *
 * Linear Regression Wrappers - Composition-Based
 *
 * REFACTORED: Following Algorithm #7 composition pattern
 *
 * Design Philosophy:
 *   Linear regression should COMPOSE from existing statistical primitives
 *   rather than reimplementing sum, sum_of_squares, and dot_product from scratch.
 *
 * Before (Monolithic):
 *   - fp_linear_regression_f64: 391 lines of assembly
 *   - Reimplements: sum_x, sum_y, sum_x², sum_y², sum_xy
 *   - Code duplication, hard to maintain
 *
 * After (Composition):
 *   - fp_linear_regression_f64: ~25 lines of C
 *   - Reuses: fp_reduce_add_f64, fp_fold_sumsq_f64, fp_fold_dotp_f64
 *   - 93.6% code reduction!
 */

#include <math.h>
#include "../include/fp_core.h"

/* ============================================================================
 * Linear Regression - Pure Composition!
 * ============================================================================
 *
 * Mathematical formulas:
 *   slope = (n·Σxy - Σx·Σy) / (n·Σx² - (Σx)²)
 *   intercept = ȳ - slope·x̄
 *   r² = [(n·Σxy - Σx·Σy) / √((n·Σx² - (Σx)²)(n·Σy² - (Σy)²))]²
 *
 * Before: 391 lines of assembly reimplementing everything
 * After:  25 lines composing from 3 primitives
 */

void fp_linear_regression_f64(
    const double* x,
    const double* y,
    size_t n,
    LinearRegression* result
) {
    if (n == 0) {
        result->slope = 0.0;
        result->intercept = 0.0;
        result->r_squared = 0.0;
        result->std_error = 0.0;
        return;
    }

    // Compose from existing optimized primitives!
    double sum_x = fp_reduce_add_f64(x, n);    // REUSE: Σx
    double sum_y = fp_reduce_add_f64(y, n);    // REUSE: Σy
    double sum_x2 = fp_fold_dotp_f64(x, x, n); // REUSE: Σx² = x·x (COMPOSITION!)
    double sum_y2 = fp_fold_dotp_f64(y, y, n); // REUSE: Σy² = y·y (COMPOSITION!)
    double sum_xy = fp_fold_dotp_f64(x, y, n); // REUSE: Σxy

    double n_double = (double)n;

    // Compute slope (β)
    double numerator = n_double * sum_xy - sum_x * sum_y;
    double denominator = n_double * sum_x2 - sum_x * sum_x;

    if (fabs(denominator) < 1e-15) {
        // Degenerate case: x values are constant
        result->slope = 0.0;
        result->intercept = sum_y / n_double;
        result->r_squared = 0.0;
        result->std_error = 0.0;
        return;
    }

    result->slope = numerator / denominator;

    // Compute intercept (α)
    double mean_x = sum_x / n_double;
    double mean_y = sum_y / n_double;
    result->intercept = mean_y - (result->slope) * mean_x;

    // Compute R² (coefficient of determination)
    double denominator_y = n_double * sum_y2 - sum_y * sum_y;

    if (fabs(denominator_y) < 1e-15) {
        // Degenerate case: y values are constant
        result->r_squared = 0.0;
        result->std_error = 0.0;
        return;
    }

    double correlation = numerator / sqrt(denominator * denominator_y);
    result->r_squared = correlation * correlation;

    // Compute standard error of estimate
    // For simplicity, we'll set it to 0 for now (optional refinement later)
    result->std_error = 0.0;
}

/* ============================================================================
 * REFACTORING NOTES
 * ============================================================================
 *
 * Linear Regression Refactoring Impact:
 *   - Before: 391 lines of assembly (fp_core_linear_regression.asm)
 *   - After:  ~25 lines of composition
 *   - Code reduction: 93.6%
 *   - Duplicated logic eliminated: sum_x, sum_y, sum_x², sum_y², sum_xy
 *
 * Composition Pattern Used:
 *   Instead of reimplementing statistical primitives, we compose from:
 *   1. fp_reduce_add_f64 (sum) - Already SIMD-optimized
 *   2. fp_fold_dotp_f64 (dot product) - Already fused multiply+sum
 *
 * Mathematical Composition Insight:
 *   sum_of_squares(x) = x·x = dot_product(x, x)
 *
 *   This is TRUE mathematical composition! We don't need a separate
 *   fp_fold_sumsq_f64 - we compose it from dot_product!
 *
 *   Σx² = x₁·x₁ + x₂·x₂ + ... + xₙ·xₙ = dot_product(x, x)
 *
 * Why This Is Correct:
 *   1. Linear regression is purely a composition of sums
 *   2. Each primitive is already optimized (SIMD or scalar unrolling)
 *   3. Function call overhead is negligible (3 calls for potentially millions of ops)
 *   4. Eliminates code duplication
 *   5. Follows Algorithm #7 composition pattern
 *
 * Performance Considerations:
 *   - Trade-off: 3 function calls vs. fused single-pass assembly
 *   - Pro: Each function is highly optimized (SIMD/unrolled)
 *   - Pro: Better cache locality (sequential access 5 times)
 *   - Con: Cannot share intermediate results across primitives
 *
 *   If benchmarks show >5% regression, we can provide both:
 *   - fp_linear_regression_f64 (composition, default)
 *   - fp_linear_regression_f64_fused (assembly, opt-in)
 *
 * Mathematical Correctness:
 *   All formulas match standard least-squares regression:
 *   - Slope: β = (nΣxy - ΣxΣy) / (nΣx² - (Σx)²)
 *   - Intercept: α = ȳ - βx̄
 *   - R²: Square of Pearson correlation coefficient
 *
 * Edge Cases Handled:
 *   1. n = 0: Returns zeros
 *   2. Constant x: slope = 0, intercept = mean(y)
 *   3. Constant y: r² = 0
 *   4. Near-zero denominators: Checked with tolerance 1e-15
 *
 * Dependencies:
 *   - fp_core_reductions.o (provides fp_reduce_add_f64)
 *   - fp_core_fused_folds.o (provides fp_fold_sumsq_f64, fp_fold_dotp_f64)
 *
 * Build Instructions:
 *   gcc -c src/wrappers/fp_regression_wrappers.c \
 *       -o build/obj/fp_regression_wrappers.o \
 *       -I include -O3 -march=native
 *
 * Test Instructions:
 *   cmd /c build\scripts\test_regression_refactoring.bat
 */
