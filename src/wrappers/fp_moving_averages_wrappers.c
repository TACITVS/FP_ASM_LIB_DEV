/**
 * fp_moving_averages_wrappers.c
 *
 * Moving Averages Wrappers (Algorithm #6) - Composition-Based
 *
 * REFACTORED: Following Algorithm #7 composition pattern
 *
 * Design Philosophy:
 *   SMA/EMA/WMA should COMPOSE from existing primitives rather than
 *   reimplementing sliding window logic from scratch.
 *
 * Before (Monolithic):
 *   - fp_map_sma_f64: 120 lines of assembly reimplementing rolling sum
 *   - Code duplication, hard to maintain
 *
 * After (Composition):
 *   - fp_map_sma_f64: 1-line wrapper to fp_rolling_mean_f64_optimized
 *   - Reuses battle-tested O(1) sliding window optimization
 *   - 99.2% code reduction!
 */

#include <stdlib.h>
#include <math.h>
#include "../include/fp_core.h"

/* ============================================================================
 * SMA (Simple Moving Average) - Pure Composition!
 * ============================================================================
 *
 * Mathematical definition:
 *   SMA[i] = (data[i] + data[i-1] + ... + data[i-window+1]) / window
 *          = rolling_mean(data, window)
 *
 * This is EXACTLY what fp_rolling_mean_f64_optimized computes!
 *
 * Before: 120 lines of assembly
 * After:  1 line of composition
 */

void fp_map_sma_f64(const double* data, size_t n, size_t window, double* output) {
    fp_rolling_mean_f64_optimized(data, n, window, output);  // ONE LINE!
}

/* ============================================================================
 * EMA (Exponential Moving Average) - Specialized Implementation
 * ============================================================================
 *
 * EMA cannot be expressed as pure composition of existing primitives because
 * it has exponential weighting with memory (depends on previous EMA value).
 *
 * Formula: EMA[i] = α * data[i] + (1-α) * EMA[i-1]
 *   where α = 2 / (window + 1)
 *
 * This is NOT a rolling window operation, so the assembly implementation
 * is justified (cannot decompose into existing primitives).
 *
 * Status: ✅ NOT A VIOLATION (specialized algorithm)
 */

void fp_map_ema_f64(const double* data, size_t n, size_t window, double* output) {
    if (n == 0 || window == 0) return;

    // Smoothing factor
    double alpha = 2.0 / (window + 1.0);
    double one_minus_alpha = 1.0 - alpha;

    // Initialize EMA with first value
    double ema = data[0];
    output[0] = ema;

    // Exponential smoothing recurrence
    for (size_t i = 1; i < n; i++) {
        ema = alpha * data[i] + one_minus_alpha * ema;
        output[i] = ema;
    }
}

/* ============================================================================
 * WMA (Weighted Moving Average) - Composition Opportunity
 * ============================================================================
 *
 * WMA applies linearly increasing weights to the window.
 * Weight[i] = i + 1 (for i = 0 to window-1)
 *
 * Formula: WMA[i] = (1*data[i-window+1] + 2*data[i-window+2] + ... + window*data[i])
 *                   / (1 + 2 + ... + window)
 *
 * Denominator = window * (window + 1) / 2
 *
 * This CAN be composed:
 *   1. For each window, compute dot product with weight vector
 *   2. Divide by sum of weights
 *
 * However, creating the weight vector for each window has overhead.
 * For now, keep specialized implementation for performance.
 *
 * Future optimization: Compose using fp_fold_dotp_f64 with precomputed weights
 *
 * Status: ⚠️ KEEP SPECIALIZED FOR NOW (optimization candidate)
 */

void fp_map_wma_f64(const double* data, size_t n, size_t window, double* output) {
    if (n < window || window == 0) return;

    size_t out_size = n - window + 1;
    double denominator = (double)window * (double)(window + 1) / 2.0;

    // For each window position
    for (size_t i = 0; i < out_size; i++) {
        double weighted_sum = 0.0;

        // Apply linearly increasing weights
        for (size_t j = 0; j < window; j++) {
            double weight = (double)(j + 1);
            weighted_sum += weight * data[i + j];
        }

        output[i] = weighted_sum / denominator;
    }
}

/* ============================================================================
 * REFACTORING NOTES
 * ============================================================================
 *
 * SMA Refactoring Impact:
 *   - Before: 120 lines of assembly (fp_core_moving_averages.asm lines 26-145)
 *   - After:  1 line of composition
 *   - Code reduction: 99.2%
 *   - Performance: IDENTICAL (both use O(1) sliding window)
 *   - Maintainability: SIGNIFICANTLY IMPROVED
 *
 * Why This Refactoring Is Correct:
 *   1. SMA is mathematically equivalent to rolling_mean
 *   2. fp_rolling_mean_f64_optimized already implements O(1) sliding window
 *   3. No performance loss (same algorithm, same complexity)
 *   4. Reduces code duplication
 *   5. Follows Algorithm #7 composition pattern
 *
 * EMA Status: NOT REFACTORED
 *   - EMA has exponential memory (depends on previous EMA)
 *   - Cannot be expressed as composition of stateless primitives
 *   - Specialized implementation is justified
 *
 * WMA Status: SIMPLIFIED BUT NOT FULLY COMPOSED
 *   - Could potentially use fp_fold_dotp_f64 with weight vector
 *   - Current simple C implementation for clarity
 *   - Future optimization candidate
 *
 * Build Instructions:
 *   gcc -c src/wrappers/fp_moving_averages_wrappers.c \
 *       -o build/obj/fp_moving_averages_wrappers.o \
 *       -I include -O3 -march=native
 *
 * Dependencies:
 *   - fp_rolling_window.o (provides fp_rolling_mean_f64_optimized)
 *   - fp_core_reductions.o (transitively used by rolling_mean)
 */
