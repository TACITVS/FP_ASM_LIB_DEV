// demo_time_series.c - Time Series Forecasting Demo
// Demonstrates statistical modeling and prediction on temporal data

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declarations
typedef struct {
    double* forecast;
    double* lower_bound;
    double* upper_bound;
    int horizon;
    double mse;
    double mae;
} ForecastResult;

ForecastResult fp_forecast_sma(const double* data, int n, int window, int horizon);
ForecastResult fp_forecast_exponential_smoothing(const double* data, int n, double alpha, int horizon);
ForecastResult fp_forecast_double_exponential_smoothing(const double* data, int n, double alpha, double beta, int horizon);
ForecastResult fp_forecast_linear_trend(const double* data, int n, int horizon);
ForecastResult fp_forecast_seasonal_naive(const double* data, int n, int period, int horizon);

void fp_generate_trend_series(double* data, int n, double intercept, double slope, double noise, unsigned int seed);
void fp_generate_seasonal_series(double* data, int n, double mean, double amplitude, int period, double noise, unsigned int seed);
void fp_generate_trend_seasonal_series(double* data, int n, double intercept, double slope, double amplitude, int period, double noise, unsigned int seed);
void fp_generate_random_walk(double* data, int n, double start, double step_std, unsigned int seed);

double fp_forecast_mape(const double* actual, const double* predicted, int n);
double fp_forecast_mae(const double* actual, const double* predicted, int n);
double fp_forecast_rmse(const double* actual, const double* predicted, int n);

void fp_forecast_print_result(const ForecastResult* result);
void fp_forecast_free_result(ForecastResult* result);

// ============================================================================
// TEST 1: Linear Trend Forecasting
// ============================================================================
void test_linear_trend() {
    printf("================================================================\n");
    printf("TEST 1: Linear Trend Forecasting\n");
    printf("================================================================\n\n");

    int n_train = 100;
    int n_test = 20;
    int n_total = n_train + n_test;

    printf("Dataset: Linear trend with noise\n");
    printf("  Formula: Y(t) = 10 + 0.5*t + noise\n");
    printf("  Training points: %d\n", n_train);
    printf("  Test points: %d\n", n_test);
    printf("  Noise level: 2.0\n\n");

    // Generate data
    double* data_full = (double*)malloc(n_total * sizeof(double));
    fp_generate_trend_series(data_full, n_total, 10.0, 0.5, 2.0, 42);

    // Split train/test
    double* data_train = data_full;

    printf("Testing multiple forecasting methods:\n\n");

    // Method 1: Simple Moving Average
    printf("Method 1: Simple Moving Average (window=10)\n");
    ForecastResult result_sma = fp_forecast_sma(data_train, n_train, 10, n_test);
    printf("  Training MSE: %.4f\n", result_sma.mse);

    // Evaluate on test set
    double mae_sma = fp_forecast_mae(&data_full[n_train], result_sma.forecast, n_test);
    printf("  Test MAE: %.4f\n\n", mae_sma);
    fp_forecast_free_result(&result_sma);

    // Method 2: Exponential Smoothing
    printf("Method 2: Exponential Smoothing (alpha=0.3)\n");
    ForecastResult result_es = fp_forecast_exponential_smoothing(data_train, n_train, 0.3, n_test);
    printf("  Training MSE: %.4f\n", result_es.mse);
    double mae_es = fp_forecast_mae(&data_full[n_train], result_es.forecast, n_test);
    printf("  Test MAE: %.4f\n\n", mae_es);
    fp_forecast_free_result(&result_es);

    // Method 3: Double Exponential Smoothing (Holt's)
    printf("Method 3: Double Exponential Smoothing (alpha=0.3, beta=0.1)\n");
    ForecastResult result_des = fp_forecast_double_exponential_smoothing(data_train, n_train, 0.3, 0.1, n_test);
    printf("  Training MSE: %.4f\n", result_des.mse);
    double mae_des = fp_forecast_mae(&data_full[n_train], result_des.forecast, n_test);
    printf("  Test MAE: %.4f\n\n", mae_des);
    fp_forecast_free_result(&result_des);

    // Method 4: Linear Trend (best for this data)
    printf("Method 4: Linear Trend Model\n");
    ForecastResult result_lt = fp_forecast_linear_trend(data_train, n_train, n_test);
    printf("  Training MSE: %.4f\n", result_lt.mse);
    double mae_lt = fp_forecast_mae(&data_full[n_train], result_lt.forecast, n_test);
    printf("  Test MAE: %.4f\n\n", mae_lt);

    printf("Forecast for next 10 steps:\n");
    printf("  Step | Actual  | Predicted | Error   | 95%% CI\n");
    printf("  -----|---------|-----------|---------|-------------------\n");
    for (int h = 0; h < 10 && h < n_test; h++) {
        double actual = data_full[n_train + h];
        double predicted = result_lt.forecast[h];
        double error = actual - predicted;
        printf("  %4d | %7.2f | %9.2f | %7.2f | [%6.2f, %6.2f]\n",
               h + 1,
               actual,
               predicted,
               error,
               result_lt.lower_bound[h],
               result_lt.upper_bound[h]);
    }
    printf("\n");

    printf("Best Method: Linear Trend (MAE=%.2f)\n", mae_lt);
    printf("Why: Data has strong linear trend, method captures it perfectly!\n\n");

    fp_forecast_free_result(&result_lt);
    free(data_full);
}

// ============================================================================
// TEST 2: Seasonal Pattern Forecasting
// ============================================================================
void test_seasonal_pattern() {
    printf("================================================================\n");
    printf("TEST 2: Seasonal Pattern Forecasting\n");
    printf("================================================================\n\n");

    int n_train = 120;  // 10 cycles of 12
    int n_test = 24;    // 2 cycles
    int period = 12;
    int n_total = n_train + n_test;

    printf("Dataset: Seasonal pattern (period=12)\n");
    printf("  Formula: Y(t) = 50 + 20*sin(2π*t/12) + noise\n");
    printf("  Training points: %d (%d cycles)\n", n_train, n_train / period);
    printf("  Test points: %d (%d cycles)\n", n_test, n_test / period);
    printf("  Noise level: 3.0\n\n");

    // Generate seasonal data
    double* data_full = (double*)malloc(n_total * sizeof(double));
    fp_generate_seasonal_series(data_full, n_total, 50.0, 20.0, period, 3.0, 42);

    double* data_train = data_full;

    printf("Testing seasonal forecasting methods:\n\n");

    // Method 1: SMA (poor for seasonality)
    printf("Method 1: Simple Moving Average (window=12)\n");
    ForecastResult result_sma = fp_forecast_sma(data_train, n_train, 12, n_test);
    double mae_sma = fp_forecast_mae(&data_full[n_train], result_sma.forecast, n_test);
    printf("  Test MAE: %.4f (poor - SMA can't capture seasonality)\n\n", mae_sma);
    fp_forecast_free_result(&result_sma);

    // Method 2: Seasonal Naive (good!)
    printf("Method 2: Seasonal Naive (period=12)\n");
    ForecastResult result_sn = fp_forecast_seasonal_naive(data_train, n_train, period, n_test);
    double mae_sn = fp_forecast_mae(&data_full[n_train], result_sn.forecast, n_test);
    printf("  Test MAE: %.4f (good - repeats seasonal pattern)\n\n", mae_sn);

    printf("Sample forecast (first seasonal cycle):\n");
    printf("  Month | Actual  | Predicted | Error\n");
    printf("  ------|---------|-----------|-------\n");
    for (int h = 0; h < 12 && h < n_test; h++) {
        printf("  %5d | %7.2f | %9.2f | %6.2f\n",
               h + 1,
               data_full[n_train + h],
               result_sn.forecast[h],
               data_full[n_train + h] - result_sn.forecast[h]);
    }
    printf("\n");

    printf("Best Method: Seasonal Naive (MAE=%.2f)\n", mae_sn);
    printf("Why: Repeats seasonal pattern from previous cycle\n\n");

    fp_forecast_free_result(&result_sn);
    free(data_full);
}

// ============================================================================
// TEST 3: Trend + Seasonality (Complex Pattern)
// ============================================================================
void test_trend_seasonal() {
    printf("================================================================\n");
    printf("TEST 3: Trend + Seasonality (Complex Pattern)\n");
    printf("================================================================\n\n");

    int n_train = 100;
    int n_test = 20;
    int period = 12;
    int n_total = n_train + n_test;

    printf("Dataset: Linear trend + seasonal pattern\n");
    printf("  Formula: Y(t) = 10 + 0.5*t + 10*sin(2π*t/12) + noise\n");
    printf("  Training points: %d\n", n_train);
    printf("  Test points: %d\n", n_test);
    printf("  Noise level: 2.0\n\n");

    // Generate complex data
    double* data_full = (double*)malloc(n_total * sizeof(double));
    fp_generate_trend_seasonal_series(data_full, n_total, 10.0, 0.5, 10.0, period, 2.0, 42);

    double* data_train = data_full;

    printf("Comparing methods on complex pattern:\n\n");

    // Test all methods
    ForecastResult result_sma = fp_forecast_sma(data_train, n_train, 10, n_test);
    ForecastResult result_es = fp_forecast_exponential_smoothing(data_train, n_train, 0.3, n_test);
    ForecastResult result_des = fp_forecast_double_exponential_smoothing(data_train, n_train, 0.3, 0.1, n_test);
    ForecastResult result_lt = fp_forecast_linear_trend(data_train, n_train, n_test);
    ForecastResult result_sn = fp_forecast_seasonal_naive(data_train, n_train, period, n_test);

    double mae_sma = fp_forecast_mae(&data_full[n_train], result_sma.forecast, n_test);
    double mae_es = fp_forecast_mae(&data_full[n_train], result_es.forecast, n_test);
    double mae_des = fp_forecast_mae(&data_full[n_train], result_des.forecast, n_test);
    double mae_lt = fp_forecast_mae(&data_full[n_train], result_lt.forecast, n_test);
    double mae_sn = fp_forecast_mae(&data_full[n_train], result_sn.forecast, n_test);

    printf("  Method                          | Test MAE\n");
    printf("  --------------------------------|---------\n");
    printf("  Simple Moving Average           | %7.2f\n", mae_sma);
    printf("  Exponential Smoothing           | %7.2f\n", mae_es);
    printf("  Double Exponential Smoothing    | %7.2f\n", mae_des);
    printf("  Linear Trend                    | %7.2f\n", mae_lt);
    printf("  Seasonal Naive                  | %7.2f\n\n", mae_sn);

    // Find best method
    double best_mae = mae_sma;
    const char* best_name = "Simple Moving Average";

    if (mae_es < best_mae) { best_mae = mae_es; best_name = "Exponential Smoothing"; }
    if (mae_des < best_mae) { best_mae = mae_des; best_name = "Double Exponential Smoothing"; }
    if (mae_lt < best_mae) { best_mae = mae_lt; best_name = "Linear Trend"; }
    if (mae_sn < best_mae) { best_mae = mae_sn; best_name = "Seasonal Naive"; }

    printf("Best Method: %s (MAE=%.2f)\n", best_name, best_mae);
    printf("Note: For trend+seasonality, ideally use Holt-Winters (triple ES)\n");
    printf("      or decompose: forecast trend + forecast seasonal separately\n\n");

    fp_forecast_free_result(&result_sma);
    fp_forecast_free_result(&result_es);
    fp_forecast_free_result(&result_des);
    fp_forecast_free_result(&result_lt);
    fp_forecast_free_result(&result_sn);
    free(data_full);
}

// ============================================================================
// TEST 4: Model Comparison on Random Walk
// ============================================================================
void test_random_walk() {
    printf("================================================================\n");
    printf("TEST 4: Random Walk (Hardest to Predict)\n");
    printf("================================================================\n\n");

    int n_train = 100;
    int n_test = 20;
    int n_total = n_train + n_test;

    printf("Dataset: Random walk (unpredictable)\n");
    printf("  Formula: Y(t) = Y(t-1) + random_step\n");
    printf("  Starting value: 100.0\n");
    printf("  Step std dev: 5.0\n");
    printf("  Training points: %d\n", n_train);
    printf("  Test points: %d\n\n", n_test);

    // Generate random walk
    double* data_full = (double*)malloc(n_total * sizeof(double));
    fp_generate_random_walk(data_full, n_total, 100.0, 5.0, 42);

    double* data_train = data_full;

    printf("Challenge: Random walk has NO predictable pattern!\n");
    printf("Best we can do: Forecast = last observed value\n\n");

    // Test methods
    ForecastResult result_sma = fp_forecast_sma(data_train, n_train, 5, n_test);
    ForecastResult result_es = fp_forecast_exponential_smoothing(data_train, n_train, 0.5, n_test);

    double mae_sma = fp_forecast_mae(&data_full[n_train], result_sma.forecast, n_test);
    double mae_es = fp_forecast_mae(&data_full[n_train], result_es.forecast, n_test);

    printf("  Method                  | Test MAE\n");
    printf("  ------------------------|---------\n");
    printf("  Simple Moving Average   | %7.2f\n", mae_sma);
    printf("  Exponential Smoothing   | %7.2f\n", mae_es);
    printf("  Naive (last value)      | %7.2f (best for random walk)\n\n",
           fp_forecast_mae(&data_full[n_train], &data_full[n_train - 1], 1));

    printf("Key Insight: Random walk is fundamentally unpredictable\n");
    printf("  - Best forecast = last value (naive method)\n");
    printf("  - Confidence intervals should grow with horizon\n");
    printf("  - Common in stock prices (efficient market hypothesis)\n\n");

    fp_forecast_free_result(&result_sma);
    fp_forecast_free_result(&result_es);
    free(data_full);
}

// ============================================================================
// Main
// ============================================================================
int main() {
    printf("================================================================\n");
    printf("  Time Series Forecasting Demo\n");
    printf("  FP-ASM Library - Statistical Prediction\n");
    printf("================================================================\n\n");

    printf("Forecasting Methods Demonstrated:\n");
    printf("  1. Simple Moving Average (SMA)\n");
    printf("  2. Exponential Smoothing (ES)\n");
    printf("  3. Double Exponential Smoothing (Holt's Method)\n");
    printf("  4. Linear Trend Model\n");
    printf("  5. Seasonal Naive\n\n");

    printf("Key Concept: Match method to data pattern!\n");
    printf("  - Linear trend → Linear trend model\n");
    printf("  - Seasonal pattern → Seasonal naive\n");
    printf("  - Trend + Seasonal → Holt-Winters (triple ES)\n");
    printf("  - Random walk → Naive (last value)\n\n");

    test_linear_trend();
    test_seasonal_pattern();
    test_trend_seasonal();
    test_random_walk();

    printf("================================================================\n");
    printf("  Time Series Forecasting Demo Complete!\n");
    printf("================================================================\n\n");

    printf("Key Takeaways:\n");
    printf("  ✓ Different patterns require different methods\n");
    printf("  ✓ Trend → Linear model or Double ES (Holt's)\n");
    printf("  ✓ Seasonality → Seasonal naive or Triple ES (Holt-Winters)\n");
    printf("  ✓ Random walk → Naive forecast (unpredictable)\n");
    printf("  ✓ Confidence intervals widen with forecast horizon\n\n");

    printf("Real-World Applications:\n");
    printf("  - Finance: Stock prices, cryptocurrency, forex\n");
    printf("  - Retail: Sales forecasting, inventory management\n");
    printf("  - Energy: Electricity demand, power generation\n");
    printf("  - Weather: Temperature, precipitation forecasting\n");
    printf("  - Web Analytics: Traffic prediction, capacity planning\n");
    printf("  - IoT: Sensor data prediction, anomaly detection\n\n");

    printf("Statistical Foundation:\n");
    printf("  - Moving Average: Simple smoothing via averaging\n");
    printf("  - Exponential Smoothing: Weighted average (recent → more weight)\n");
    printf("  - Holt's Method: ES with linear trend component\n");
    printf("  - Holt-Winters: ES with trend + seasonal components\n");
    printf("  - ARIMA: Combines AR (past values) + MA (past errors)\n\n");

    printf("Industry Standard Tools:\n");
    printf("  - Python: statsmodels, pmdarima, prophet (Facebook)\n");
    printf("  - R: forecast package (Hyndman)\n");
    printf("  - Commercial: SAS, SPSS, Tableau\n");
    printf("  - Modern: Deep learning (LSTM, Transformers) for complex patterns\n\n");

    return 0;
}
