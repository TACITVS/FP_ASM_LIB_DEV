/**
 * fp_time_series.h - Time Series Forecasting Algorithms API
 *
 * Provides time series forecasting methods with confidence intervals.
 * Uses L0 ASM primitives internally for SIMD-accelerated computations.
 *
 * Features:
 * - Moving averages (SMA, EMA)
 * - Exponential smoothing (single, double)
 * - Linear trend forecasting
 * - Seasonal naive forecasting
 * - Forecast evaluation metrics (MAE, RMSE, MAPE)
 *
 * Example:
 *   double data[100];
 *   fp_generate_trend_series(data, 100, 10.0, 0.5, 2.0, 42);
 *   ForecastResult result = fp_forecast_linear_trend(data, 100, 10);
 *   fp_forecast_print_result(&result);
 *   fp_forecast_free_result(&result);
 */

#ifndef FP_TIME_SERIES_H
#define FP_TIME_SERIES_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================ */

/**
 * ForecastResult - Forecast output with confidence intervals
 *
 * Contains predicted values and 95% confidence bounds.
 * Arrays are allocated internally; free with fp_forecast_free_result().
 */
typedef struct {
    double* forecast;           /**< Predicted values (horizon elements) */
    double* lower_bound;        /**< Lower confidence bound (95%) */
    double* upper_bound;        /**< Upper confidence bound (95%) */
    int horizon;                /**< Number of steps forecasted */
    double mse;                 /**< Mean squared error on training data */
    double mae;                 /**< Mean absolute error on training data */
} ForecastResult;

/**
 * TimeSeriesDecomposition - Decomposed time series components
 *
 * Separates time series into trend, seasonal, and residual components.
 */
typedef struct {
    double* trend;              /**< Trend component */
    double* seasonal;           /**< Seasonal component */
    double* residual;           /**< Residual (noise) component */
    int n;                      /**< Length of time series */
} TimeSeriesDecomposition;

/**
 * ARIMAModel - ARIMA model parameters
 *
 * Stores coefficients for AutoRegressive Integrated Moving Average models.
 */
typedef struct {
    int p;                      /**< AR order (autoregressive) */
    int d;                      /**< I order (integrated/differencing) */
    int q;                      /**< MA order (moving average) */
    double* ar_coeffs;          /**< AR coefficients (p elements) */
    double* ma_coeffs;          /**< MA coefficients (q elements) */
    double intercept;           /**< Constant term */
} ARIMAModel;

/* ============================================================================
 * FORECASTING FUNCTIONS
 * ============================================================================ */

/**
 * fp_forecast_sma - Simple Moving Average forecast
 *
 * Predicts future values as the average of the last 'window' observations.
 * Best for: Stationary data without trend or seasonality.
 *
 * @param data      Time series data
 * @param n         Number of observations
 * @param window    Window size for moving average
 * @param horizon   Number of steps to forecast
 * @return ForecastResult with predictions and confidence intervals
 */
ForecastResult fp_forecast_sma(const double* data, int n, int window, int horizon);

/**
 * fp_forecast_exponential_smoothing - Single Exponential Smoothing
 *
 * Applies exponential decay weights: S_t = α·Y_t + (1-α)·S_{t-1}
 * Best for: Stationary data where recent observations matter more.
 *
 * @param data      Time series data
 * @param n         Number of observations
 * @param alpha     Smoothing parameter (0 < α < 1)
 * @param horizon   Number of steps to forecast
 * @return ForecastResult with predictions and confidence intervals
 */
ForecastResult fp_forecast_exponential_smoothing(
    const double* data, int n, double alpha, int horizon
);

/**
 * fp_forecast_double_exponential_smoothing - Holt's Method
 *
 * Handles linear trend: Level + Trend with separate smoothing parameters.
 * Best for: Data with linear trend but no seasonality.
 *
 * @param data      Time series data
 * @param n         Number of observations
 * @param alpha     Level smoothing parameter (0 < α < 1)
 * @param beta      Trend smoothing parameter (0 < β < 1)
 * @param horizon   Number of steps to forecast
 * @return ForecastResult with predictions and confidence intervals
 */
ForecastResult fp_forecast_double_exponential_smoothing(
    const double* data, int n, double alpha, double beta, int horizon
);

/**
 * fp_forecast_linear_trend - Linear Trend Model
 *
 * Fits linear regression: Y_t = a + b*t and extrapolates.
 * Best for: Data with clear linear trend.
 *
 * @param data      Time series data
 * @param n         Number of observations
 * @param horizon   Number of steps to forecast
 * @return ForecastResult with predictions and confidence intervals
 */
ForecastResult fp_forecast_linear_trend(const double* data, int n, int horizon);

/**
 * fp_forecast_seasonal_naive - Seasonal Naive Forecast
 *
 * Predicts future values as values from the same season last period.
 * Best for: Strongly seasonal data with stable patterns.
 *
 * @param data      Time series data
 * @param n         Number of observations
 * @param period    Seasonal period (e.g., 12 for monthly data)
 * @param horizon   Number of steps to forecast
 * @return ForecastResult with predictions and confidence intervals
 */
ForecastResult fp_forecast_seasonal_naive(
    const double* data, int n, int period, int horizon
);

/* ============================================================================
 * DATA GENERATION (for testing and demos)
 * ============================================================================ */

/**
 * fp_generate_trend_series - Generate time series with linear trend
 *
 * Formula: Y(t) = intercept + slope*t + noise
 *
 * @param data          Output array (pre-allocated, n elements)
 * @param n             Number of points to generate
 * @param intercept     Starting value (Y-intercept)
 * @param slope         Trend slope
 * @param noise_level   Amplitude of random noise
 * @param seed          RNG seed for deterministic generation
 */
void fp_generate_trend_series(
    double* data, int n, double intercept, double slope,
    double noise_level, uint64_t seed
);

/**
 * fp_generate_seasonal_series - Generate time series with seasonality
 *
 * Formula: Y(t) = mean + amplitude*sin(2π*t/period) + noise
 *
 * @param data          Output array (pre-allocated, n elements)
 * @param n             Number of points to generate
 * @param mean          Mean value
 * @param amplitude     Seasonal amplitude
 * @param period        Seasonal period
 * @param noise_level   Amplitude of random noise
 * @param seed          RNG seed for deterministic generation
 */
void fp_generate_seasonal_series(
    double* data, int n, double mean, double amplitude,
    int period, double noise_level, uint64_t seed
);

/**
 * fp_generate_trend_seasonal_series - Generate time series with trend + seasonality
 *
 * Formula: Y(t) = intercept + slope*t + amplitude*sin(2π*t/period) + noise
 *
 * @param data          Output array (pre-allocated, n elements)
 * @param n             Number of points to generate
 * @param intercept     Starting value (Y-intercept)
 * @param slope         Trend slope
 * @param amplitude     Seasonal amplitude
 * @param period        Seasonal period
 * @param noise_level   Amplitude of random noise
 * @param seed          RNG seed for deterministic generation
 */
void fp_generate_trend_seasonal_series(
    double* data, int n, double intercept, double slope,
    double amplitude, int period, double noise_level, uint64_t seed
);

/**
 * fp_generate_random_walk - Generate random walk time series
 *
 * Formula: Y(t) = Y(t-1) + random_step
 *
 * @param data          Output array (pre-allocated, n elements)
 * @param n             Number of points to generate
 * @param start         Starting value
 * @param step_std      Standard deviation of random steps
 * @param seed          RNG seed for deterministic generation
 */
void fp_generate_random_walk(
    double* data, int n, double start, double step_std, uint64_t seed
);

/* ============================================================================
 * EVALUATION METRICS
 * ============================================================================ */

/**
 * fp_forecast_mape - Mean Absolute Percentage Error
 *
 * MAPE = (100/n) * Σ |actual - predicted| / |actual|
 * Skips zero values in actual to avoid division by zero.
 *
 * @param actual        Actual values
 * @param predicted     Predicted values
 * @param n             Number of values
 * @return MAPE as percentage (0-100+)
 */
double fp_forecast_mape(const double* actual, const double* predicted, size_t n);

/**
 * fp_forecast_mae - Mean Absolute Error
 *
 * MAE = (1/n) * Σ |actual - predicted|
 * Uses SIMD-accelerated computation via L0 primitives.
 *
 * @param actual        Actual values
 * @param predicted     Predicted values
 * @param n             Number of values
 * @return Mean absolute error
 */
double fp_forecast_mae(const double* actual, const double* predicted, size_t n);

/**
 * fp_forecast_rmse - Root Mean Squared Error
 *
 * RMSE = sqrt((1/n) * Σ (actual - predicted)²)
 * Uses SIMD-accelerated computation via L0 primitives.
 *
 * @param actual        Actual values
 * @param predicted     Predicted values
 * @param n             Number of values
 * @return Root mean squared error
 */
double fp_forecast_rmse(const double* actual, const double* predicted, size_t n);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * fp_forecast_print_result - Print forecast result summary
 *
 * Displays horizon, training errors, and first 10 forecast values with
 * confidence intervals.
 *
 * @param result    Pointer to ForecastResult to print
 */
void fp_forecast_print_result(const ForecastResult* result);

/**
 * fp_forecast_free_result - Free internal arrays of ForecastResult
 *
 * Frees result->forecast, result->lower_bound, result->upper_bound.
 *
 * @param result    Pointer to ForecastResult to free
 */
void fp_forecast_free_result(ForecastResult* result);

#endif /* FP_TIME_SERIES_H */
