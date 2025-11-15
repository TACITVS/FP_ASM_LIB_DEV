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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Data Structures
// ============================================================================

// Forecast result with confidence intervals
typedef struct {
    double* forecast;           // Predicted values
    double* lower_bound;        // Lower confidence bound (95%)
    double* upper_bound;        // Upper confidence bound (95%)
    int horizon;                // Number of steps forecasted
    double mse;                 // Mean squared error on training data
    double mae;                 // Mean absolute error on training data
} ForecastResult;

// Time series decomposition
typedef struct {
    double* trend;              // Trend component
    double* seasonal;           // Seasonal component
    double* residual;           // Residual (noise) component
    int n;                      // Length of time series
} TimeSeriesDecomposition;

// ARIMA model parameters
typedef struct {
    int p;                      // AR order (autoregressive)
    int d;                      // I order (integrated/differencing)
    int q;                      // MA order (moving average)
    double* ar_coeffs;          // AR coefficients (p)
    double* ma_coeffs;          // MA coefficients (q)
    double intercept;           // Constant term
} ARIMAModel;

// ============================================================================
// Basic Statistics
// ============================================================================

static double compute_mean(const double* data, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += data[i];
    }
    return sum / n;
}

static double compute_variance(const double* data, int n) {
    double mean = compute_mean(data, n);
    double sum_sq = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = data[i] - mean;
        sum_sq += diff * diff;
    }
    return sum_sq / (n - 1);
}

static double compute_std(const double* data, int n) {
    return sqrt(compute_variance(data, n));
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
    double mse = 0.0;
    for (int i = window; i < n; i++) {
        double sum_window = 0.0;
        for (int j = i - window; j < i; j++) {
            sum_window += data[j];
        }
        double pred = sum_window / window;
        double error = data[i] - pred;
        mse += error * error;
    }
    mse /= (n - window);
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
void fp_generate_trend_series(
    double* data,
    int n,
    double intercept,
    double slope,
    double noise_level,
    unsigned int seed
) {
    srand(seed);
    for (int t = 0; t < n; t++) {
        double trend = intercept + slope * t;
        double noise = noise_level * (2.0 * (double)rand() / RAND_MAX - 1.0);
        data[t] = trend + noise;
    }
}

// Generate time series with seasonality
void fp_generate_seasonal_series(
    double* data,
    int n,
    double mean,
    double seasonal_amplitude,
    int period,
    double noise_level,
    unsigned int seed
) {
    srand(seed);
    for (int t = 0; t < n; t++) {
        double seasonal = seasonal_amplitude * sin(2.0 * M_PI * t / period);
        double noise = noise_level * (2.0 * (double)rand() / RAND_MAX - 1.0);
        data[t] = mean + seasonal + noise;
    }
}

// Generate time series with trend + seasonality
void fp_generate_trend_seasonal_series(
    double* data,
    int n,
    double intercept,
    double slope,
    double seasonal_amplitude,
    int period,
    double noise_level,
    unsigned int seed
) {
    srand(seed);
    for (int t = 0; t < n; t++) {
        double trend = intercept + slope * t;
        double seasonal = seasonal_amplitude * sin(2.0 * M_PI * t / period);
        double noise = noise_level * (2.0 * (double)rand() / RAND_MAX - 1.0);
        data[t] = trend + seasonal + noise;
    }
}

// Generate random walk
void fp_generate_random_walk(
    double* data,
    int n,
    double start,
    double step_std,
    unsigned int seed
) {
    srand(seed);
    data[0] = start;
    for (int t = 1; t < n; t++) {
        double step = step_std * (2.0 * (double)rand() / RAND_MAX - 1.0);
        data[t] = data[t - 1] + step;
    }
}

// ============================================================================
// Evaluation Metrics
// ============================================================================

// Mean Absolute Percentage Error
double fp_forecast_mape(const double* actual, const double* predicted, int n) {
    double sum_ape = 0.0;
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (fabs(actual[i]) > 1e-10) {  // Avoid division by zero
            sum_ape += fabs((actual[i] - predicted[i]) / actual[i]);
            count++;
        }
    }
    return (count > 0) ? (100.0 * sum_ape / count) : 0.0;
}

// Mean Absolute Error
double fp_forecast_mae(const double* actual, const double* predicted, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += fabs(actual[i] - predicted[i]);
    }
    return sum / n;
}

// Root Mean Squared Error
double fp_forecast_rmse(const double* actual, const double* predicted, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        double error = actual[i] - predicted[i];
        sum += error * error;
    }
    return sqrt(sum / n);
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
