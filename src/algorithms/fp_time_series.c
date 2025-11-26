// fp_time_series.c
//
// Time Series Forecasting Algorithms
// Demonstrates statistical modeling and prediction
//
// This showcases:
// - Moving averages (SMA, EMA)
// - Exponential smoothing (single, double, triple)
// - Trend analysis and forecasting
// - Seasonal decomposition
// - ARIMA modeling (AutoRegressive Integrated Moving Average)
//
// FP Primitives Used:
// - Windowed operations (sliding window for MA)
// - Weighted reductions (exponential smoothing)
// - Statistical computations (mean, variance, autocorrelation)
//
// Applications:
// - Stock price prediction
// - Weather forecasting
// - Sales demand forecasting
// - Sensor data prediction
// - Anomaly detection

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/fp_core.h"          // Assembly primitives
#include "../include/fp_stats_v3_pure.h" // Pure FP statistics (uses assembly)
#include "../include/fp_rng.h"           // Deterministic RNG
#include "../include/fp_time_series.h"   // Public API declarations

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Pattern 1: Array Statistics (Compose Assembly Primitives)
// ============================================================================
// NOTE: Basic stats (mean, variance, std) now use fp_stats_v3_pure.h
// which composes assembly primitives like fp_reduce_add_f64()
//
// Here we add domain-specific helpers for time series forecasting

// Mean Squared Error: Pure FP composition of L1 primitives
// MSE = mean((actual - predicted)²) = dotp(errors, errors) / n
static inline double fp_mse_inline(const double* actual, const double* predicted, size_t n) {
    if (!actual || !predicted || n == 0) return 0.0;

    // Allocate error array
    double* errors = (double*)malloc(n * sizeof(double));
    if (!errors) return 0.0;

    // L1: Compute errors using fp_map_axpy (SIMD!)
    // errors = -1.0 * predicted + actual = actual - predicted
    fp_map_axpy_f64(predicted, actual, errors, n, -1.0);

    // L0: MSE = dotp(errors, errors) / n using assembly primitive!
    double dotp = fp_fold_dotp_f64(errors, errors, n);
    free(errors);

    return dotp / (double)n;
}

// Mean Absolute Error: Pure FP composition of L0 primitives (SIMD AVX2)
// MAE = mean(|actual - predicted|) = reduce_add(map_abs(errors)) / n
static inline double fp_mae_inline(const double* actual, const double* predicted, size_t n) {
    if (!actual || !predicted || n == 0) return 0.0;

    // Compute errors: actual - predicted
    double* errors = (double*)malloc(n * sizeof(double));
    if (!errors) return 0.0;

    // L0: errors = actual - predicted using SIMD map
    fp_map_axpy_f64(predicted, actual, errors, n, -1.0);

    // L0: Compute absolute values in-place using SIMD (AVX2)
    fp_map_abs_f64(errors, errors, n);

    // L0: Sum absolute values using SIMD reduction
    double sum_abs = fp_reduce_add_f64(errors, n);

    free(errors);
    return sum_abs / (double)n;
}

// ============================================================================
// Pattern 2: Sliding Window Operations (Using Assembly Primitives)
// ============================================================================

// Sliding window sum: efficiently compute sum of each window using rolling sum
// Returns array of sums (caller must free)
// Example: data=[1,2,3,4,5], window=3 → [6, 9, 12] (sums of [1,2,3], [2,3,4], [3,4,5])
// Complexity: O(n) instead of O(n*window) naive approach
static inline double* fp_sliding_window_sum_inline(const double* data, size_t n, size_t window, size_t* out_count) {
    if (!data || n < window || window == 0) {
        *out_count = 0;
        return NULL;
    }

    size_t count = n - window + 1;
    double* result = (double*)malloc(count * sizeof(double));
    if (!result) {
        *out_count = 0;
        return NULL;
    }

    // Initial window sum using assembly-optimized reduce!
    double sum = fp_reduce_add_f64(data, window);
    result[0] = sum;

    // Slide window: add new element, remove old element (O(1) per step)
    // This part must be sequential, but we optimized the initial sum
    for (size_t i = 1; i < count; i++) {
        sum = sum - data[i - 1] + data[i + window - 1];
        result[i] = sum;
    }

    *out_count = count;
    return result;
}

// Sliding window mean: moving average (optimized O(n))
// Returns array of means (caller must free)
static inline double* fp_sliding_window_mean_inline(const double* data, size_t n, size_t window, size_t* out_count) {
    double* sums = fp_sliding_window_sum_inline(data, n, window, out_count);
    if (!sums) return NULL;

    // Convert sums to means in-place
    for (size_t i = 0; i < *out_count; i++) {
        sums[i] /= (double)window;
    }

    return sums;  // Now contains means instead of sums
}

// Compute MSE between actual values and sliding window predictions
// Used for evaluating forecast accuracy on training data
// This replaces the O(n*window) nested loop pattern with O(n) rolling window
static inline double fp_sliding_window_mse_inline(const double* data, size_t n, size_t window) {
    if (!data || n <= window) return 0.0;

    size_t pred_count;
    // Get predictions for all positions (using data up to i-1 to predict i)
    double* predictions = fp_sliding_window_mean_inline(data, n - 1, window, &pred_count);
    if (!predictions) return 0.0;

    // Compute MSE: compare predictions with actual values (shifted by window)
    double mse = 0.0;
    for (size_t i = 0; i < pred_count; i++) {
        double error = data[i + window] - predictions[i];
        mse += error * error;
    }
    mse /= (double)pred_count;

    free(predictions);
    return mse;
}

// ============================================================================
// Data Structures - defined in fp_time_series.h
// ============================================================================
// ForecastResult, TimeSeriesDecomposition, and ARIMAModel are now defined
// in the public header to ensure declaration/definition consistency.

// ============================================================================
// Basic Statistics (Using fp_stats_v3_pure.h - Assembly Optimized)
// ============================================================================

// REFACTORED: Uses fp_mean_pure() which uses fp_reduce_add_f64() assembly primitive
static double compute_mean(const double* data, int n) {
    return fp_mean_pure(data, n);
}

// REFACTORED: Uses fp_variance_pure() from fp_stats_v3_pure.h
static double compute_variance(const double* data, int n) {
    double mean = fp_mean_pure(data, n);
    return fp_variance_pure(data, n, mean);
}

// REFACTORED: Uses fp_std_pure() from fp_stats_v3_pure.h
static double compute_std(const double* data, int n) {
    double mean = fp_mean_pure(data, n);
    return fp_std_pure(data, n, mean);
}

// Compute autocorrelation at lag k
static double compute_autocorrelation(const double* data, int n, int lag) {
    if (lag >= n) return 0.0;

    double mean = compute_mean(data, n);
    double variance = compute_variance(data, n);

    if (variance < 1e-10) return 0.0;

    double sum = 0.0;
    for (int i = 0; i < n - lag; i++) {
        sum += (data[i] - mean) * (data[i + lag] - mean);
    }

    return sum / ((n - lag) * variance);
}

// ============================================================================
// 1. Simple Moving Average (SMA)
// ============================================================================

// SMA forecast: average of last 'window' values
ForecastResult fp_forecast_sma(
    const double* data,
    int n,
    int window,
    int horizon
) {
    // HIGH-007 FIX: Add input validation
    if (!data || n <= 0 || window <= 0 || horizon <= 0) {
        ForecastResult error = {0};
        return error;
    }

    ForecastResult result;
    result.horizon = horizon;
    result.forecast = (double*)malloc(horizon * sizeof(double));
    result.lower_bound = (double*)malloc(horizon * sizeof(double));
    result.upper_bound = (double*)malloc(horizon * sizeof(double));

    // Forecast: average of last 'window' values
    double sum = 0.0;
    int start = (n > window) ? (n - window) : 0;
    int count = n - start;

    for (int i = start; i < n; i++) {
        sum += data[i];
    }
    double forecast_value = sum / count;

    // Constant forecast for all horizons (SMA assumption)
    for (int h = 0; h < horizon; h++) {
        result.forecast[h] = forecast_value;
    }

    // Compute training error for confidence intervals
    // REFACTORED: Use Pattern 2 fp_sliding_window_mse_inline() - O(n) instead of O(n*window)
    double mse = fp_sliding_window_mse_inline(data, n, window);
    result.mse = mse;
    result.mae = sqrt(mse);  // Approximation

    // Confidence intervals (assume normal errors)
    double std_error = sqrt(mse);
    for (int h = 0; h < horizon; h++) {
        result.lower_bound[h] = result.forecast[h] - 1.96 * std_error;
        result.upper_bound[h] = result.forecast[h] + 1.96 * std_error;
    }

    return result;
}

// ============================================================================
// 2. Exponential Smoothing (Single)
// ============================================================================

// Single exponential smoothing: S_t = α·Y_t + (1-α)·S_{t-1}
ForecastResult fp_forecast_exponential_smoothing(
    const double* data,
    int n,
    double alpha,           // Smoothing parameter (0 < α < 1)
    int horizon
) {
    // HIGH-007 FIX: Add input validation
    if (!data || n <= 0 || alpha <= 0.0 || alpha >= 1.0 || horizon <= 0) {
        ForecastResult error = {0};
        return error;
    }

    ForecastResult result;
    result.horizon = horizon;
    result.forecast = (double*)malloc(horizon * sizeof(double));
    result.lower_bound = (double*)malloc(horizon * sizeof(double));
    result.upper_bound = (double*)malloc(horizon * sizeof(double));

    // Initialize with first value
    double S = data[0];

    // Update smoothed value through time series
    double mse = 0.0;
    for (int t = 1; t < n; t++) {
        double pred = S;
        double error = data[t] - pred;
        mse += error * error;

        S = alpha * data[t] + (1.0 - alpha) * S;
    }
    mse /= (n - 1);
    result.mse = mse;
    result.mae = sqrt(mse);

    // Forecast: constant (level = S)
    for (int h = 0; h < horizon; h++) {
        result.forecast[h] = S;
    }

    // Confidence intervals
    double std_error = sqrt(mse);
    for (int h = 0; h < horizon; h++) {
        // Variance grows with horizon for ES
        double h_variance = std_error * sqrt(1.0 + (h + 1) * alpha * alpha);
        result.lower_bound[h] = result.forecast[h] - 1.96 * h_variance;
        result.upper_bound[h] = result.forecast[h] + 1.96 * h_variance;
    }

    return result;
}

// ============================================================================
// 3. Double Exponential Smoothing (Holt's Method)
// ============================================================================

// Handles linear trend: Level + Trend
ForecastResult fp_forecast_double_exponential_smoothing(
    const double* data,
    int n,
    double alpha,           // Level smoothing
    double beta,            // Trend smoothing
    int horizon
) {
    // HIGH-007 FIX: Add input validation
    if (!data || n <= 0 || alpha <= 0.0 || alpha >= 1.0 || beta <= 0.0 || beta >= 1.0 || horizon <= 0) {
        ForecastResult error = {0};
        return error;
    }

    ForecastResult result;
    result.horizon = horizon;
    result.forecast = (double*)malloc(horizon * sizeof(double));
    result.lower_bound = (double*)malloc(horizon * sizeof(double));
    result.upper_bound = (double*)malloc(horizon * sizeof(double));

    // Initialize level and trend
    double L = data[0];
    double T = (n > 1) ? (data[1] - data[0]) : 0.0;

    // Update through time series
    double mse = 0.0;
    for (int t = 1; t < n; t++) {
        double pred = L + T;
        double error = data[t] - pred;
        mse += error * error;

        double L_prev = L;
        L = alpha * data[t] + (1.0 - alpha) * (L + T);
        T = beta * (L - L_prev) + (1.0 - beta) * T;
    }
    mse /= (n - 1);
    result.mse = mse;
    result.mae = sqrt(mse);

    // Forecast: Level + h*Trend
    for (int h = 0; h < horizon; h++) {
        result.forecast[h] = L + (h + 1) * T;
    }

    // Confidence intervals (growing with horizon)
    double std_error = sqrt(mse);
    for (int h = 0; h < horizon; h++) {
        double h_variance = std_error * sqrt(1.0 + (h + 1) * 0.5);
        result.lower_bound[h] = result.forecast[h] - 1.96 * h_variance;
        result.upper_bound[h] = result.forecast[h] + 1.96 * h_variance;
    }

    return result;
}

// ============================================================================
// 4. Linear Trend Forecasting
// ============================================================================

// Fit linear model: Y_t = a + b*t
ForecastResult fp_forecast_linear_trend(
    const double* data,
    int n,
    int horizon
) {
    // HIGH-007 FIX: Add input validation
    if (!data || n <= 0 || horizon <= 0) {
        ForecastResult error = {0};
        return error;
    }

    ForecastResult result;
    result.horizon = horizon;
    result.forecast = (double*)malloc(horizon * sizeof(double));
    result.lower_bound = (double*)malloc(horizon * sizeof(double));
    result.upper_bound = (double*)malloc(horizon * sizeof(double));

    // Compute linear regression: Y = a + b*t
    double sum_t = 0.0, sum_y = 0.0, sum_ty = 0.0, sum_tt = 0.0;

    for (int t = 0; t < n; t++) {
        sum_t += t;
        sum_y += data[t];
        sum_ty += t * data[t];
        sum_tt += t * t;
    }

    double mean_t = sum_t / n;
    double mean_y = sum_y / n;

    double b = (sum_ty - n * mean_t * mean_y) / (sum_tt - n * mean_t * mean_t);
    double a = mean_y - b * mean_t;

    // Compute MSE
    double mse = 0.0;
    for (int t = 0; t < n; t++) {
        double pred = a + b * t;
        double error = data[t] - pred;
        mse += error * error;
    }
    mse /= n;
    result.mse = mse;
    result.mae = sqrt(mse);

    // Forecast
    for (int h = 0; h < horizon; h++) {
        int t_future = n + h;
        result.forecast[h] = a + b * t_future;
    }

    // Confidence intervals (wider further out)
    double std_error = sqrt(mse);
    for (int h = 0; h < horizon; h++) {
        int t_future = n + h;
        // Standard error grows with distance from mean
        double t_diff = t_future - mean_t;
        double se_pred = std_error * sqrt(1.0 + 1.0/n + t_diff*t_diff/(sum_tt - n*mean_t*mean_t));
        result.lower_bound[h] = result.forecast[h] - 1.96 * se_pred;
        result.upper_bound[h] = result.forecast[h] + 1.96 * se_pred;
    }

    return result;
}

// ============================================================================
// 5. Seasonal Naive Forecast
// ============================================================================

// Seasonal naive: forecast = value from same season last period
ForecastResult fp_forecast_seasonal_naive(
    const double* data,
    int n,
    int period,             // Seasonal period (e.g., 12 for monthly data)
    int horizon
) {
    // HIGH-007 FIX: Add input validation
    if (!data || n <= 0 || period <= 0 || horizon <= 0) {
        ForecastResult error = {0};
        return error;
    }

    ForecastResult result;
    result.horizon = horizon;
    result.forecast = (double*)malloc(horizon * sizeof(double));
    result.lower_bound = (double*)malloc(horizon * sizeof(double));
    result.upper_bound = (double*)malloc(horizon * sizeof(double));

    // Forecast: repeat last seasonal cycle
    for (int h = 0; h < horizon; h++) {
        int lookback = n - period + (h % period);
        if (lookback >= 0 && lookback < n) {
            result.forecast[h] = data[lookback];
        } else {
            result.forecast[h] = data[n - 1];
        }
    }

    // Compute MSE from seasonal naive predictions
    double mse = 0.0;
    int count = 0;
    for (int t = period; t < n; t++) {
        double pred = data[t - period];
        double error = data[t] - pred;
        mse += error * error;
        count++;
    }
    if (count > 0) {
        mse /= count;
    }
    result.mse = mse;
    result.mae = sqrt(mse);

    // Confidence intervals
    double std_error = sqrt(mse);
    for (int h = 0; h < horizon; h++) {
        result.lower_bound[h] = result.forecast[h] - 1.96 * std_error;
        result.upper_bound[h] = result.forecast[h] + 1.96 * std_error;
    }

    return result;
}

// ============================================================================
// Data Generation (for testing)
// ============================================================================

// Generate time series with trend
// REFACTORED: Uses fp_rng for deterministic, reproducible data generation
void fp_generate_trend_series(
    double* data,
    int n,
    double intercept,
    double slope,
    double noise_level,
    uint64_t seed
) {
    fp_rng_t rng = fp_rng_create(seed);
    for (int t = 0; t < n; t++) {
        double trend = intercept + slope * t;
        double noise;
        rng = fp_rng_next_f64_range(rng, -noise_level, noise_level, &noise);
        data[t] = trend + noise;
    }
}

// Generate time series with seasonality
// REFACTORED: Uses fp_rng for deterministic data generation
void fp_generate_seasonal_series(
    double* data,
    int n,
    double mean,
    double seasonal_amplitude,
    int period,
    double noise_level,
    uint64_t seed
) {
    fp_rng_t rng = fp_rng_create(seed);
    for (int t = 0; t < n; t++) {
        double seasonal = seasonal_amplitude * sin(2.0 * M_PI * t / period);
        double noise;
        rng = fp_rng_next_f64_range(rng, -noise_level, noise_level, &noise);
        data[t] = mean + seasonal + noise;
    }
}

// Generate time series with trend + seasonality
// REFACTORED: Uses fp_rng for deterministic data generation
void fp_generate_trend_seasonal_series(
    double* data,
    int n,
    double intercept,
    double slope,
    double seasonal_amplitude,
    int period,
    double noise_level,
    uint64_t seed
) {
    fp_rng_t rng = fp_rng_create(seed);
    for (int t = 0; t < n; t++) {
        double trend = intercept + slope * t;
        double seasonal = seasonal_amplitude * sin(2.0 * M_PI * t / period);
        double noise;
        rng = fp_rng_next_f64_range(rng, -noise_level, noise_level, &noise);
        data[t] = trend + seasonal + noise;
    }
}

// Generate random walk
// REFACTORED: Uses fp_rng for deterministic data generation
void fp_generate_random_walk(
    double* data,
    int n,
    double start,
    double step_std,
    uint64_t seed
) {
    fp_rng_t rng = fp_rng_create(seed);
    data[0] = start;
    for (int t = 1; t < n; t++) {
        double step;
        rng = fp_rng_next_f64_range(rng, -step_std, step_std, &step);
        data[t] = data[t - 1] + step;
    }
}

// ============================================================================
// Evaluation Metrics
// ============================================================================

// Mean Absolute Percentage Error
double fp_forecast_mape(const double* actual, const double* predicted, size_t n) {
    double sum_ape = 0.0;
    size_t count = 0;
    for (size_t i = 0; i < n; i++) {
        if (fabs(actual[i]) > 1e-10) {  // Avoid division by zero
            sum_ape += fabs((actual[i] - predicted[i]) / actual[i]);
            count++;
        }
    }
    return (count > 0) ? (100.0 * sum_ape / count) : 0.0;
}

// Mean Absolute Error - REFACTORED to use L0 SIMD primitives
double fp_forecast_mae(const double* actual, const double* predicted, size_t n) {
    if (n == 0) return 0.0;

    // Allocate temp buffer for errors
    double* errors = (double*)malloc(n * sizeof(double));
    if (!errors) return 0.0;

    // L0: errors = actual - predicted
    fp_map_axpy_f64(predicted, actual, errors, n, -1.0);

    // L0: Compute absolute values in-place (AVX2)
    fp_map_abs_f64(errors, errors, n);

    // L0: Sum absolute values
    double sum = fp_reduce_add_f64(errors, n);

    free(errors);
    return sum / (double)n;
}

// Root Mean Squared Error - REFACTORED to use L0 SIMD primitives
double fp_forecast_rmse(const double* actual, const double* predicted, size_t n) {
    if (n == 0) return 0.0;

    // Allocate temp buffer for errors
    double* errors = (double*)malloc(n * sizeof(double));
    if (!errors) return 0.0;

    // L0: errors = actual - predicted
    fp_map_axpy_f64(predicted, actual, errors, n, -1.0);

    // L0: Sum of squared errors using dot product (AVX2)
    double sum_sq = fp_fold_dotp_f64(errors, errors, n);

    free(errors);
    return sqrt(sum_sq / (double)n);
}

// ============================================================================
// Printing Utilities
// ============================================================================

void fp_forecast_print_result(const ForecastResult* result) {
    printf("Forecast Results:\n");
    printf("  Horizon: %d steps\n", result->horizon);
    printf("  Training MSE: %.6f\n", result->mse);
    printf("  Training MAE: %.6f\n\n", result->mae);

    printf("  Step | Forecast | 95%% CI Lower | 95%% CI Upper\n");
    printf("  -----|----------|--------------|-------------\n");
    for (int h = 0; h < result->horizon && h < 10; h++) {
        printf("  %4d | %8.2f | %12.2f | %12.2f\n",
               h + 1,
               result->forecast[h],
               result->lower_bound[h],
               result->upper_bound[h]);
    }
    if (result->horizon > 10) {
        printf("  ... (%d more steps)\n", result->horizon - 10);
    }
    printf("\n");
}

void fp_forecast_free_result(ForecastResult* result) {
    free(result->forecast);
    free(result->lower_bound);
    free(result->upper_bound);
}
