/**
 * fp_rolling_window.c
 *
 * Algorithm #7: Rolling Window Statistics
 * FUNCTIONAL COMPOSITION pattern - reuses existing reduce functions!
 *
 * Design Philosophy:
 *   Instead of reimplementing min/max/sum for rolling windows,
 *   we COMPOSE existing optimized functions through higher-order
 *   function abstraction.
 *
 *   rolling(f) = apply f to each sliding window
 *   rolling_min = rolling(reduce_min)  <-- COMPOSITION!
 *   rolling_max = rolling(reduce_max)  <-- COMPOSITION!
 *   rolling_sum = rolling(reduce_add)  <-- COMPOSITION!
 *
 * This is TRUE functional programming in C!
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/fp_core.h"

/* =============================================================================
 * GENERIC ROLLING WINDOW - Higher-Order Function
 * =============================================================================
 *
 * This is the CORE abstraction! It accepts any reduction function
 * and applies it over sliding windows.
 *
 * Type signature (Haskell-style):
 *   rolling_reduce :: (Array -> Double) -> Int -> Array -> Array
 *
 * In C with function pointers:
 *   void rolling_reduce(reduce_fn, window, input, output)
 */

void fp_rolling_reduce_f64(
    const double* data,           // Input array
    size_t n,                     // Array length
    size_t window,                // Window size
    double (*reduce_fn)(const double*, size_t),  // Reduction function (curried!)
    double* output                // Output array
) {
    if (n < window || window == 0) return;

    size_t out_size = n - window + 1;

    // Apply reduction function to each window
    for (size_t i = 0; i < out_size; i++) {
        output[i] = reduce_fn(&data[i], window);  // COMPOSITION!
    }
}

/* Same for i64 */
void fp_rolling_reduce_i64(
    const int64_t* data,
    size_t n,
    size_t window,
    int64_t (*reduce_fn)(const int64_t*, size_t),
    int64_t* output
) {
    if (n < window || window == 0) return;

    size_t out_size = n - window + 1;

    for (size_t i = 0; i < out_size; i++) {
        output[i] = reduce_fn(&data[i], window);
    }
}

/* =============================================================================
 * THIN WRAPPER FUNCTIONS - Pure Composition!
 * =============================================================================
 *
 * These are ONE-LINERS through composition!
 * Each function is just: rolling_reduce(specific_reduce_fn)
 *
 * rolling_min = rolling ∘ reduce_min
 * rolling_max = rolling ∘ reduce_max
 * rolling_sum = rolling ∘ reduce_add
 */

void fp_rolling_min_f64(const double* data, size_t n, size_t window, double* output) {
    fp_rolling_reduce_f64(data, n, window, fp_reduce_min_f64, output);
    // ↑ COMPOSITION! Reuses existing optimized SIMD function
}

void fp_rolling_max_f64(const double* data, size_t n, size_t window, double* output) {
    fp_rolling_reduce_f64(data, n, window, fp_reduce_max_f64, output);
    // ↑ COMPOSITION! Reuses existing optimized SIMD function
}

void fp_rolling_sum_f64(const double* data, size_t n, size_t window, double* output) {
    fp_rolling_reduce_f64(data, n, window, fp_reduce_add_f64, output);
    // ↑ COMPOSITION! Reuses existing optimized SIMD function
}

void fp_rolling_min_i64(const int64_t* data, size_t n, size_t window, int64_t* output) {
    fp_rolling_reduce_i64(data, n, window, fp_reduce_min_i64, output);
}

void fp_rolling_max_i64(const int64_t* data, size_t n, size_t window, int64_t* output) {
    fp_rolling_reduce_i64(data, n, window, fp_reduce_max_i64, output);
}

void fp_rolling_sum_i64(const int64_t* data, size_t n, size_t window, int64_t* output) {
    fp_rolling_reduce_i64(data, n, window, fp_reduce_add_i64, output);
}

/* =============================================================================
 * COMPOSED OPERATIONS - Functions built from other functions!
 * =============================================================================
 *
 * These demonstrate composition of multiple operations:
 * rolling_mean = scale(rolling_sum, 1/window)
 * rolling_range = subtract(rolling_max, rolling_min)
 */

void fp_rolling_mean_f64(const double* data, size_t n, size_t window, double* output) {
    // mean = sum / window
    // Compose: rolling(sum) then map(scale)
    fp_rolling_sum_f64(data, n, window, output);

    size_t out_size = n - window + 1;
    double scale_factor = 1.0 / window;

    // In-place scaling (could also use fp_map_scale_f64!)
    for (size_t i = 0; i < out_size; i++) {
        output[i] *= scale_factor;
    }
}

void fp_rolling_range_f64(const double* data, size_t n, size_t window, double* output) {
    // range = max - min
    // Compose: rolling(max) - rolling(min)

    if (n < window || window == 0) return;

    size_t out_size = n - window + 1;
    double* temp_min = (double*)malloc(out_size * sizeof(double));
    if (!temp_min) return;

    fp_rolling_max_f64(data, n, window, output);      // max into output
    fp_rolling_min_f64(data, n, window, temp_min);    // min into temp

    // output = max - min
    for (size_t i = 0; i < out_size; i++) {
        output[i] -= temp_min[i];
    }

    free(temp_min);
}

/* =============================================================================
 * STATISTICAL COMPOSITIONS
 * =============================================================================
 *
 * For variance and std deviation, we compose with descriptive_stats
 */

void fp_rolling_std_f64(const double* data, size_t n, size_t window, double* output) {
    if (n < window || window == 0) return;

    size_t out_size = n - window + 1;

    // For each window, use fp_descriptive_stats_f64 which is already optimized!
    for (size_t i = 0; i < out_size; i++) {
        DescriptiveStats stats;
        fp_descriptive_stats_f64(&data[i], window, &stats);
        output[i] = stats.std_dev;  // REUSE!
    }
}

void fp_rolling_variance_f64(const double* data, size_t n, size_t window, double* output) {
    if (n < window || window == 0) return;

    size_t out_size = n - window + 1;

    // For each window, use fp_descriptive_stats_f64
    for (size_t i = 0; i < out_size; i++) {
        DescriptiveStats stats;
        fp_descriptive_stats_f64(&data[i], window, &stats);
        output[i] = stats.variance;  // REUSE!
    }
}

/* =============================================================================
 * OPTIMIZED SPECIALIZATIONS (Optional - for hot paths)
 * =============================================================================
 *
 * For sum/mean, we can use sliding window optimization like SMA
 * These are O(1) per step instead of O(window)
 */

void fp_rolling_sum_f64_optimized(const double* data, size_t n, size_t window, double* output) {
    if (n < window || window == 0) return;

    // Compute initial window sum
    double sum = 0.0;
    for (size_t i = 0; i < window; i++) {
        sum += data[i];
    }
    output[0] = sum;

    // Sliding window: subtract oldest, add newest (O(1) per step!)
    size_t out_size = n - window + 1;
    for (size_t i = 1; i < out_size; i++) {
        sum = sum - data[i - 1] + data[i + window - 1];
        output[i] = sum;
    }
}

void fp_rolling_mean_f64_optimized(const double* data, size_t n, size_t window, double* output) {
    // mean = sum / window (optimized version)
    fp_rolling_sum_f64_optimized(data, n, window, output);

    size_t out_size = n - window + 1;
    double scale_factor = 1.0 / window;

    for (size_t i = 0; i < out_size; i++) {
        output[i] *= scale_factor;
    }
}

/* =============================================================================
 * DEMONSTRATION: User can create custom rolling functions!
 * =============================================================================
 *
 * Example: Rolling product (if we had fp_reduce_product_f64)
 *
 * void fp_rolling_product_f64(const double* data, size_t n, size_t window, double* output) {
 *     fp_rolling_reduce_f64(data, n, window, fp_reduce_product_f64, output);
 * }
 *
 * That's it! One line through composition!
 * This is the power of higher-order functions and FP composition.
 */
