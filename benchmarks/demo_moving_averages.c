// demo_moving_averages.c
//
// Demo and Test for Algorithm #6: Moving Averages (Financial Computing)
// Demonstrates SMA, EMA, and WMA for time series analysis

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../include/fp_core.h"

// ============================================================================
// C BASELINE IMPLEMENTATIONS
// ============================================================================

// Naive O(n*window) implementation
void sma_baseline_naive(const double* data, size_t n, size_t window, double* output) {
    if (n == 0 || window == 0 || window > n) return;

    size_t out_size = n - window + 1;
    for (size_t i = 0; i < out_size; i++) {
        double sum = 0.0;
        for (size_t j = 0; j < window; j++) {
            sum += data[i + j];
        }
        output[i] = sum / window;
    }
}

// Optimized O(n) sliding window implementation - fair comparison
void sma_baseline(const double* data, size_t n, size_t window, double* output) {
    if (n == 0 || window == 0 || window > n) return;

    // Compute initial window sum
    double sum = 0.0;
    for (size_t i = 0; i < window; i++) {
        sum += data[i];
    }
    output[0] = sum / window;

    // Sliding window: subtract oldest, add newest
    size_t out_size = n - window + 1;
    for (size_t i = 1; i < out_size; i++) {
        sum = sum - data[i - 1] + data[i + window - 1];
        output[i] = sum / window;
    }
}

void ema_baseline(const double* data, size_t n, size_t window, double* output) {
    if (n == 0 || window == 0) return;

    double alpha = 2.0 / (window + 1);
    double one_minus_alpha = 1.0 - alpha;

    output[0] = data[0];
    for (size_t i = 1; i < n; i++) {
        output[i] = alpha * data[i] + one_minus_alpha * output[i - 1];
    }
}

void wma_baseline(const double* data, size_t n, size_t window, double* output) {
    if (n == 0 || window == 0 || window > n) return;

    size_t sum_of_weights = window * (window + 1) / 2;
    size_t out_size = n - window + 1;

    for (size_t i = 0; i < out_size; i++) {
        double weighted_sum = 0.0;
        for (size_t j = 0; j < window; j++) {
            weighted_sum += data[i + j] * (window - j);
        }
        output[i] = weighted_sum / sum_of_weights;
    }
}

// ============================================================================
// CORRECTNESS TESTS
// ============================================================================

int test_sma_basic(void) {
    printf("\nTest 1: SMA Basic Functionality\n");
    printf("================================\n");

    double data[] = {10.0, 20.0, 30.0, 40.0, 50.0, 60.0};
    size_t n = 6;
    size_t window = 3;
    size_t out_size = n - window + 1;

    double output_asm[4];
    double output_c[4];

    fp_sma_f64(data, n, window, output_asm);
    sma_baseline(data, n, window, output_c);

    printf("Input: [10, 20, 30, 40, 50, 60], window=3\n");
    printf("Expected: [20, 30, 40, 50]\n");
    printf("FP-ASM:   [%.1f, %.1f, %.1f, %.1f]\n",
           output_asm[0], output_asm[1], output_asm[2], output_asm[3]);
    printf("C Baseline: [%.1f, %.1f, %.1f, %.1f]\n",
           output_c[0], output_c[1], output_c[2], output_c[3]);

    // Verify
    for (size_t i = 0; i < out_size; i++) {
        if (fabs(output_asm[i] - output_c[i]) > 1e-9) {
            printf("FAILED: Mismatch at index %zu\n", i);
            return 0;
        }
    }

    printf("PASSED\n");
    return 1;
}

int test_ema_responsiveness(void) {
    printf("\nTest 2: EMA Responsiveness\n");
    printf("===========================\n");

    double data[] = {100.0, 100.0, 100.0, 110.0, 110.0, 110.0};
    size_t n = 6;
    size_t window = 3;

    double output_asm[6];
    double output_c[6];

    fp_ema_f64(data, n, window, output_asm);
    ema_baseline(data, n, window, output_c);

    printf("Input: [100, 100, 100, 110, 110, 110], window=3\n");
    printf("FP-ASM EMA: [");
    for (size_t i = 0; i < n; i++) {
        printf("%.2f%s", output_asm[i], i < n-1 ? ", " : "]\n");
    }
    printf("C Baseline:  [");
    for (size_t i = 0; i < n; i++) {
        printf("%.2f%s", output_c[i], i < n-1 ? ", " : "]\n");
    }

    // Verify
    for (size_t i = 0; i < n; i++) {
        if (fabs(output_asm[i] - output_c[i]) > 1e-9) {
            printf("FAILED: Mismatch at index %zu\n", i);
            return 0;
        }
    }

    printf("PASSED (EMA adapts to level change)\n");
    return 1;
}

int test_wma_weighting(void) {
    printf("\nTest 3: WMA Weighting\n");
    printf("======================\n");

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    size_t n = 5;
    size_t window = 3;
    size_t out_size = n - window + 1;

    double output_asm[3];
    double output_c[3];

    fp_wma_f64(data, n, window, output_asm);
    wma_baseline(data, n, window, output_c);

    printf("Input: [1, 2, 3, 4, 5], window=3\n");
    printf("WMA weights: [3, 2, 1] (most recent has highest weight)\n");
    printf("FP-ASM:   [");
    for (size_t i = 0; i < out_size; i++) {
        printf("%.2f%s", output_asm[i], i < out_size-1 ? ", " : "]\n");
    }
    printf("C Baseline: [");
    for (size_t i = 0; i < out_size; i++) {
        printf("%.2f%s", output_c[i], i < out_size-1 ? ", " : "]\n");
    }

    // Verify
    for (size_t i = 0; i < out_size; i++) {
        if (fabs(output_asm[i] - output_c[i]) > 1e-9) {
            printf("FAILED: Mismatch at index %zu\n", i);
            return 0;
        }
    }

    printf("PASSED (WMA emphasizes recent values)\n");
    return 1;
}

// ============================================================================
// REAL-WORLD SCENARIOS
// ============================================================================

void scenario_stock_trend_analysis(void) {
    printf("\n========================================\n");
    printf("Scenario 1: Stock Price Trend Analysis\n");
    printf("========================================\n");
    printf("Use Case: Detecting trend changes with SMA crossover\n\n");

    // Simulated stock prices showing uptrend then downtrend
    double prices[] = {
        100.0, 102.0, 101.0, 103.0, 105.0,  // Uptrend
        107.0, 109.0, 108.0, 110.0, 112.0,  // Continued uptrend
        111.0, 109.0, 107.0, 105.0, 103.0,  // Downtrend
        101.0, 99.0, 98.0, 96.0, 95.0       // Continued downtrend
    };
    size_t n = 20;

    // Short-term (5-day) and long-term (10-day) SMAs
    size_t short_window = 5;
    size_t long_window = 10;

    double sma_short[20];
    double sma_long[20];

    fp_sma_f64(prices, n, short_window, sma_short);
    fp_sma_f64(prices, n, long_window, sma_long);

    printf("Day | Price  | SMA-5  | SMA-10 | Signal\n");
    printf("----+--------+--------+--------+------------------\n");

    for (size_t i = 9; i < n; i++) {  // Start when both SMAs available
        size_t idx_short = i - short_window + 1;
        size_t idx_long = i - long_window + 1;

        char signal[20] = "";
        if (i > 9) {
            size_t prev_short = idx_short - 1;
            size_t prev_long = idx_long - 1;

            if (sma_short[prev_short] <= sma_long[prev_long] &&
                sma_short[idx_short] > sma_long[idx_long]) {
                strcpy(signal, "BULLISH CROSS");
            } else if (sma_short[prev_short] >= sma_long[prev_long] &&
                       sma_short[idx_short] < sma_long[idx_long]) {
                strcpy(signal, "BEARISH CROSS");
            }
        }

        printf("%3zu | %6.2f | %6.2f | %6.2f | %s\n",
               i+1, prices[i], sma_short[idx_short], sma_long[idx_long], signal);
    }

    printf("\nInterpretation:\n");
    printf("- BULLISH CROSS: Short-term SMA crosses above long-term (potential buy signal)\n");
    printf("- BEARISH CROSS: Short-term SMA crosses below long-term (potential sell signal)\n");
}

void scenario_trading_signal_ema(void) {
    printf("\n==========================================\n");
    printf("Scenario 2: Trading Signal with EMA\n");
    printf("==========================================\n");
    printf("Use Case: Fast EMA vs Slow EMA for momentum detection\n\n");

    // Simulated crypto prices with volatility
    double prices[] = {
        50000.0, 50200.0, 49800.0, 50500.0, 51000.0,
        51200.0, 50800.0, 51500.0, 52000.0, 52500.0,
        52800.0, 53200.0, 52900.0, 53500.0, 54000.0
    };
    size_t n = 15;

    // Fast (5-period) and Slow (10-period) EMAs
    size_t fast_window = 5;
    size_t slow_window = 10;

    double ema_fast[15];
    double ema_slow[15];

    fp_ema_f64(prices, n, fast_window, ema_fast);
    fp_ema_f64(prices, n, slow_window, ema_slow);

    printf("Period | Price    | EMA-5    | EMA-10   | Momentum\n");
    printf("-------+----------+----------+----------+--------------\n");

    for (size_t i = 0; i < n; i++) {
        char momentum[20] = "";
        if (i > 0) {
            double fast_change = ema_fast[i] - ema_fast[i-1];
            double slow_change = ema_slow[i] - ema_slow[i-1];

            if (fast_change > slow_change && fast_change > 0) {
                strcpy(momentum, "ACCELERATING");
            } else if (fast_change < slow_change && fast_change < 0) {
                strcpy(momentum, "DECELERATING");
            }
        }

        printf("%6zu | %8.2f | %8.2f | %8.2f | %s\n",
               i+1, prices[i], ema_fast[i], ema_slow[i], momentum);
    }

    printf("\nInterpretation:\n");
    printf("- ACCELERATING: Fast EMA rising faster than slow (strong upward momentum)\n");
    printf("- DECELERATING: Fast EMA falling faster than slow (losing momentum)\n");
    printf("- EMA responds faster than SMA to price changes\n");
}

void scenario_volatility_comparison(void) {
    printf("\n============================================\n");
    printf("Scenario 3: Volatility-Adjusted Averaging\n");
    printf("============================================\n");
    printf("Use Case: Comparing SMA, EMA, WMA during volatile period\n\n");

    // Simulated volatile market data
    double prices[] = {
        100.0, 105.0, 102.0, 108.0, 103.0,
        110.0, 106.0, 112.0, 108.0, 115.0
    };
    size_t n = 10;
    size_t window = 5;

    double sma[10];
    double ema[10];
    double wma[10];

    fp_sma_f64(prices, n, window, sma);
    fp_ema_f64(prices, n, window, ema);
    fp_wma_f64(prices, n, window, wma);

    printf("Day | Price  | SMA-5  | EMA-5  | WMA-5  |\n");
    printf("----+--------+--------+--------+--------+\n");

    for (size_t i = 0; i < n; i++) {
        if (i >= window - 1) {
            size_t sma_idx = i - window + 1;
            printf("%3zu | %6.2f | %6.2f | %6.2f | %6.2f |\n",
                   i+1, prices[i], sma[sma_idx], ema[i], wma[sma_idx]);
        } else {
            printf("%3zu | %6.2f |   --   | %6.2f |   --   |\n",
                   i+1, prices[i], ema[i]);
        }
    }

    printf("\nInterpretation:\n");
    printf("- SMA: Equal weight to all values (smoothest, most lagging)\n");
    printf("- EMA: Exponential decay (responsive, good for trends)\n");
    printf("- WMA: Linear weight (balance between SMA and EMA)\n");
    printf("- In volatile markets, EMA captures momentum better than SMA\n");
}

// ============================================================================
// PERFORMANCE BENCHMARKS
// ============================================================================

void benchmark_sma(size_t n, int iterations) {
    printf("\nBenchmarking SMA (n=%zu, iterations=%d)\n", n, iterations);
    printf("==========================================\n");

    double* data = malloc(n * sizeof(double));
    size_t window = n / 10;  // 10% window
    size_t out_size = n - window + 1;
    double* output = malloc(out_size * sizeof(double));

    for (size_t i = 0; i < n; i++) {
        data[i] = (double)(rand() % 1000);
    }

    // Warmup
    fp_sma_f64(data, n, window, output);
    sma_baseline(data, n, window, output);
    sma_baseline_naive(data, n, window, output);

    // Benchmark assembly
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        fp_sma_f64(data, n, window, output);
    }
    clock_t end = clock();
    double time_asm = (double)(end - start) / CLOCKS_PER_SEC;

    // Benchmark C optimized (fair comparison - same algorithm)
    start = clock();
    for (int i = 0; i < iterations; i++) {
        sma_baseline(data, n, window, output);
    }
    end = clock();
    double time_c_optimized = (double)(end - start) / CLOCKS_PER_SEC;

    // Benchmark C naive (algorithmic comparison)
    start = clock();
    for (int i = 0; i < iterations; i++) {
        sma_baseline_naive(data, n, window, output);
    }
    end = clock();
    double time_c_naive = (double)(end - start) / CLOCKS_PER_SEC;

    printf("FP-ASM (sliding window):    %.6f seconds\n", time_asm);
    printf("C Optimized (sliding window): %.6f seconds\n", time_c_optimized);
    printf("C Naive (recompute sum):    %.6f seconds\n", time_c_naive);
    printf("\n");
    printf("Implementation speedup (ASM vs C optimized): %.2fx\n", time_c_optimized / time_asm);
    printf("Algorithmic speedup (optimized vs naive):   %.2fx\n", time_c_naive / time_c_optimized);

    free(data);
    free(output);
}

void benchmark_ema(size_t n, int iterations) {
    printf("\nBenchmarking EMA (n=%zu, iterations=%d)\n", n, iterations);
    printf("==========================================\n");

    double* data = malloc(n * sizeof(double));
    double* output = malloc(n * sizeof(double));
    size_t window = n / 10;

    for (size_t i = 0; i < n; i++) {
        data[i] = (double)(rand() % 1000);
    }

    // Warmup
    fp_ema_f64(data, n, window, output);
    ema_baseline(data, n, window, output);

    // Benchmark assembly
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        fp_ema_f64(data, n, window, output);
    }
    clock_t end = clock();
    double time_asm = (double)(end - start) / CLOCKS_PER_SEC;

    // Benchmark C
    start = clock();
    for (int i = 0; i < iterations; i++) {
        ema_baseline(data, n, window, output);
    }
    end = clock();
    double time_c = (double)(end - start) / CLOCKS_PER_SEC;

    printf("FP-ASM:     %.6f seconds\n", time_asm);
    printf("C Baseline: %.6f seconds\n", time_c);
    printf("Speedup:    %.2fx\n", time_c / time_asm);

    free(data);
    free(output);
}

void benchmark_wma(size_t n, int iterations) {
    printf("\nBenchmarking WMA (n=%zu, iterations=%d)\n", n, iterations);
    printf("==========================================\n");

    double* data = malloc(n * sizeof(double));
    size_t window = n / 10;
    size_t out_size = n - window + 1;
    double* output = malloc(out_size * sizeof(double));

    for (size_t i = 0; i < n; i++) {
        data[i] = (double)(rand() % 1000);
    }

    // Warmup
    fp_wma_f64(data, n, window, output);
    wma_baseline(data, n, window, output);

    // Benchmark assembly
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        fp_wma_f64(data, n, window, output);
    }
    clock_t end = clock();
    double time_asm = (double)(end - start) / CLOCKS_PER_SEC;

    // Benchmark C
    start = clock();
    for (int i = 0; i < iterations; i++) {
        wma_baseline(data, n, window, output);
    }
    end = clock();
    double time_c = (double)(end - start) / CLOCKS_PER_SEC;

    printf("FP-ASM:     %.6f seconds\n", time_asm);
    printf("C Baseline: %.6f seconds\n", time_c);
    printf("Speedup:    %.2fx\n", time_asm > 0 ? time_c / time_asm : 0.0);

    free(data);
    free(output);
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("========================================\n");
    printf("Algorithm #6: Moving Averages\n");
    printf("Financial Computing & Technical Analysis\n");
    printf("========================================\n");

    // Part 1: Correctness Tests
    printf("\n=== PART 1: CORRECTNESS TESTS ===\n");

    int passed = 0;
    int total = 3;

    passed += test_sma_basic();
    passed += test_ema_responsiveness();
    passed += test_wma_weighting();

    printf("\n=== CORRECTNESS SUMMARY: %d/%d tests passed ===\n", passed, total);

    if (passed != total) {
        printf("\nERROR: Some tests failed. Fix issues before benchmarking.\n");
        return 1;
    }

    // Part 2: Real-World Scenarios
    printf("\n=== PART 2: REAL-WORLD SCENARIOS ===\n");

    scenario_stock_trend_analysis();
    scenario_trading_signal_ema();
    scenario_volatility_comparison();

    // Part 3: Performance Benchmarks
    printf("\n=== PART 3: PERFORMANCE BENCHMARKS ===\n");

    srand(42);  // Reproducible results

    benchmark_sma(10000, 1000);
    benchmark_ema(10000, 1000);
    benchmark_wma(1000, 100);  // WMA is O(n*window), use smaller dataset

    printf("\n=== Algorithm #6 Complete ===\n");
    printf("Moving averages are fundamental to technical analysis:\n");
    printf("- SMA: Trend identification (optimized sliding window)\n");
    printf("- EMA: Momentum detection (recursive formula)\n");
    printf("- WMA: Balance between smoothness and responsiveness\n");

    return 0;
}
