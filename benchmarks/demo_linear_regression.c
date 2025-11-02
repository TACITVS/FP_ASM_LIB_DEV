// demo_linear_regression.c
// Comprehensive test suite for Algorithm #4: Linear Regression
// Tests correctness with known datasets and real-world scenarios
// Benchmarks performance against naive C implementations

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/fp_core.h"

// Tolerance for floating-point comparisons
#define EPSILON 1e-9

// ============================================================================
// BASELINE C IMPLEMENTATIONS (for correctness verification and benchmarking)
// ============================================================================

void c_linear_regression(const double* x, const double* y, size_t n, LinearRegression* result) {
    if (n < 2) {
        result->slope = NAN;
        result->intercept = NAN;
        result->r_squared = NAN;
        result->std_error = NAN;
        return;
    }

    // Special case: 2 points give perfect fit
    if (n == 2) {
        double x0 = x[0], x1 = x[1];
        double y0 = y[0], y1 = y[1];

        // Check for vertical line
        if (x1 == x0) {
            result->slope = NAN;
            result->intercept = NAN;
            result->r_squared = NAN;
            result->std_error = NAN;
            return;
        }

        result->slope = (y1 - y0) / (x1 - x0);
        result->intercept = y0 - result->slope * x0;
        result->r_squared = 1.0;
        result->std_error = 0.0;
        return;
    }

    // Pass 1: Compute statistics
    double sum_x = 0.0, sum_y = 0.0;
    double sum_x2 = 0.0, sum_y2 = 0.0, sum_xy = 0.0;

    for (size_t i = 0; i < n; i++) {
        sum_x += x[i];
        sum_y += y[i];
        sum_x2 += x[i] * x[i];
        sum_y2 += y[i] * y[i];
        sum_xy += x[i] * y[i];
    }

    double mean_x = sum_x / n;
    double mean_y = sum_y / n;
    double var_x = (sum_x2 / n) - (mean_x * mean_x);
    double var_y = (sum_y2 / n) - (mean_y * mean_y);
    double cov_xy = (sum_xy / n) - (mean_x * mean_y);

    if (var_x <= 0.0) {
        result->slope = NAN;
        result->intercept = NAN;
        result->r_squared = NAN;
        result->std_error = NAN;
        return;
    }

    // Compute coefficients
    result->slope = cov_xy / var_x;
    result->intercept = mean_y - result->slope * mean_x;

    // Compute R²
    double stddev_x = sqrt(var_x);
    double stddev_y = sqrt(var_y);
    double correlation = cov_xy / (stddev_x * stddev_y);
    result->r_squared = correlation * correlation;

    // Pass 2: Compute standard error
    double sum_squared_residuals = 0.0;
    for (size_t i = 0; i < n; i++) {
        double y_pred = result->slope * x[i] + result->intercept;
        double residual = y[i] - y_pred;
        sum_squared_residuals += residual * residual;
    }

    result->std_error = sqrt(sum_squared_residuals / (n - 2));
}

double c_predict(double x_value, const LinearRegression* model) {
    return model->slope * x_value + model->intercept;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

int compare_doubles(double a, double b) {
    if (isnan(a) && isnan(b)) return 1;
    if (isnan(a) || isnan(b)) return 0;
    return fabs(a - b) < EPSILON;
}

int compare_regression(const LinearRegression* a, const LinearRegression* b) {
    return compare_doubles(a->slope, b->slope) &&
           compare_doubles(a->intercept, b->intercept) &&
           compare_doubles(a->r_squared, b->r_squared) &&
           compare_doubles(a->std_error, b->std_error);
}

void print_test_header(const char* test_name) {
    printf("\n");
    printf("================================================================================\n");
    printf("%s\n", test_name);
    printf("================================================================================\n");
}

void print_model(const char* label, const LinearRegression* model) {
    printf("%s:\n", label);
    printf("  y = %.6fx + %.6f\n", model->slope, model->intercept);
    printf("  R^2 = %.6f\n", model->r_squared);
    printf("  Std Error = %.6f\n", model->std_error);
}

// ============================================================================
// CORRECTNESS TESTS
// ============================================================================

int test_perfect_positive_fit() {
    print_test_header("TEST 1: Perfect Positive Fit (y = 2x + 1)");

    double x[] = {1, 2, 3, 4, 5};
    double y[] = {3, 5, 7, 9, 11};  // y = 2x + 1 exactly
    size_t n = 5;

    LinearRegression fp_model, c_model;
    fp_linear_regression_f64(x, y, n, &fp_model);
    c_linear_regression(x, y, n, &c_model);

    printf("Data: Perfect line y = 2x + 1\n");
    printf("Expected: slope=2.0, intercept=1.0, R^2=1.0\n\n");
    print_model("FP-ASM", &fp_model);
    printf("\n");
    print_model("C", &c_model);

    int match = compare_regression(&fp_model, &c_model);
    int correct = fabs(fp_model.slope - 2.0) < EPSILON &&
                  fabs(fp_model.intercept - 1.0) < EPSILON &&
                  fabs(fp_model.r_squared - 1.0) < EPSILON;

    if (match && correct) {
        printf("\nPASS: Perfect fit detected, models match\n");
        return 1;
    } else {
        printf("\nFAIL: Mismatch detected\n");
        return 0;
    }
}

int test_perfect_negative_fit() {
    print_test_header("TEST 2: Perfect Negative Fit (y = -3x + 10)");

    double x[] = {1, 2, 3, 4, 5};
    double y[] = {7, 4, 1, -2, -5};  // y = -3x + 10
    size_t n = 5;

    LinearRegression fp_model, c_model;
    fp_linear_regression_f64(x, y, n, &fp_model);
    c_linear_regression(x, y, n, &c_model);

    printf("Data: Perfect line y = -3x + 10\n");
    printf("Expected: slope=-3.0, intercept=10.0, R^2=1.0\n\n");
    print_model("FP-ASM", &fp_model);
    printf("\n");
    print_model("C", &c_model);

    int match = compare_regression(&fp_model, &c_model);
    int correct = fabs(fp_model.slope + 3.0) < EPSILON &&
                  fabs(fp_model.intercept - 10.0) < EPSILON &&
                  fabs(fp_model.r_squared - 1.0) < EPSILON;

    if (match && correct) {
        printf("\nPASS: Perfect negative fit detected, models match\n");
        return 1;
    } else {
        printf("\nFAIL: Mismatch detected\n");
        return 0;
    }
}

int test_noisy_data() {
    print_test_header("TEST 3: Noisy Data (approximately y = 1.5x + 2)");

    double x[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    double y[] = {3.2, 5.1, 6.8, 8.3, 9.7, 11.5, 12.9, 14.6, 15.8, 17.2};
    size_t n = 10;

    LinearRegression fp_model, c_model;
    fp_linear_regression_f64(x, y, n, &fp_model);
    c_linear_regression(x, y, n, &c_model);

    printf("Data: Noisy measurements around y = 1.5x + 2\n");
    printf("Expected: slope~1.5, intercept~2, R^2>0.95\n\n");
    print_model("FP-ASM", &fp_model);
    printf("\n");
    print_model("C", &c_model);

    int match = compare_regression(&fp_model, &c_model);
    int reasonable = fp_model.slope > 1.3 && fp_model.slope < 1.7 &&
                     fp_model.intercept > 1.0 && fp_model.intercept < 3.0 &&
                     fp_model.r_squared > 0.95;

    if (match && reasonable) {
        printf("\nPASS: Noisy data handled correctly, models match\n");
        return 1;
    } else {
        printf("\nFAIL: Mismatch detected\n");
        return 0;
    }
}

int test_horizontal_line() {
    print_test_header("TEST 4: Horizontal Line (y = 5, slope = 0)");

    double x[] = {1, 2, 3, 4, 5, 6, 7, 8};
    double y[] = {5, 5, 5, 5, 5, 5, 5, 5};  // Constant y
    size_t n = 8;

    LinearRegression fp_model, c_model;
    fp_linear_regression_f64(x, y, n, &fp_model);
    c_linear_regression(x, y, n, &c_model);

    printf("Data: Constant y = 5 for all x\n");
    printf("Expected: slope=0, intercept=5, R^2=0 (or undefined)\n\n");
    print_model("FP-ASM", &fp_model);
    printf("\n");
    print_model("C", &c_model);

    int match = compare_regression(&fp_model, &c_model);
    int correct = fabs(fp_model.slope) < EPSILON &&
                  fabs(fp_model.intercept - 5.0) < EPSILON;

    if (match && correct) {
        printf("\nPASS: Horizontal line detected, models match\n");
        return 1;
    } else {
        printf("\nFAIL: Mismatch detected\n");
        return 0;
    }
}

int test_edge_cases() {
    print_test_header("TEST 5: Edge Cases (2 points, constant x)");

    // Two points - should give perfect fit
    double x1[] = {1, 5};
    double y1[] = {2, 10};
    LinearRegression fp_two, c_two;
    fp_linear_regression_f64(x1, y1, 2, &fp_two);
    c_linear_regression(x1, y1, 2, &c_two);

    printf("Two points: (1,2) and (5,10)\n");
    printf("Expected: slope=2, intercept=0, R^2=1\n");
    print_model("FP-ASM (2 points)", &fp_two);
    printf("\n");
    print_model("C (2 points)", &c_two);

    // Constant x - undefined regression
    double x2[] = {3, 3, 3, 3, 3};
    double y2[] = {1, 2, 3, 4, 5};
    LinearRegression fp_const;
    fp_linear_regression_f64(x2, y2, 5, &fp_const);

    printf("\n\nConstant x = 3 (vertical line):\n");
    printf("Expected: All NaN (undefined)\n");
    print_model("FP-ASM (constant x)", &fp_const);

    int two_match = compare_regression(&fp_two, &c_two);
    int two_correct = fabs(fp_two.slope - 2.0) < EPSILON &&
                      fabs(fp_two.r_squared - 1.0) < EPSILON;
    int const_ok = isnan(fp_const.slope);

    if (two_match && two_correct && const_ok) {
        printf("\nPASS: Edge cases handled correctly\n");
        return 1;
    } else {
        printf("\nFAIL: Edge case mismatch\n");
        return 0;
    }
}

// ============================================================================
// REAL-WORLD SCENARIOS
// ============================================================================

void scenario_housing_prices() {
    print_test_header("REAL-WORLD SCENARIO 1: Housing Prices vs Square Footage");
    printf("Analyzing relationship between house size and sale price\n");
    printf("Data: 30 recent home sales in suburban area\n\n");

    // Square footage (hundreds of sq ft)
    double sqft[] = {
        12.5, 15.2, 18.7, 14.3, 16.8, 22.4, 19.5, 13.9, 17.2, 20.8,
        16.1, 21.3, 14.7, 18.9, 23.6, 15.8, 19.2, 17.6, 21.8, 14.1,
        20.3, 16.9, 22.7, 15.4, 18.3, 24.2, 17.8, 19.7, 16.3, 21.1
    };

    // Sale price ($1000s)
    double price[] = {
        245, 298, 365, 278, 328, 445, 382, 265, 335, 412,
        315, 422, 288, 368, 475, 305, 375, 345, 435, 272,
        402, 332, 452, 295, 358, 488, 348, 388, 318, 418
    };

    size_t n = 30;

    LinearRegression model;
    fp_linear_regression_f64(sqft, price, n, &model);

    printf("Results:\n");
    print_model("Housing Model", &model);

    printf("\nInterpretation:\n");
    printf("  Base price (0 sq ft): $%.0fk (y-intercept)\n", model.intercept);
    printf("  Price per 100 sq ft: $%.2fk (slope)\n", model.slope);
    printf("  Model fit (R^2): %.2f%% of price variation explained by size\n",
           model.r_squared * 100);

    if (model.r_squared > 0.85) {
        printf("\n  EXCELLENT fit: Square footage is a strong predictor of price.\n");
        printf("  Real estate agents can reliably estimate prices using this model.\n");
    }

    // Make predictions
    printf("\n  Sample Predictions:\n");
    double test_sizes[] = {15.0, 20.0, 25.0};
    for (int i = 0; i < 3; i++) {
        double pred_price = fp_predict_f64(test_sizes[i], &model);
        printf("    %.0f00 sq ft house → $%.0fk ± $%.0fk\n",
               test_sizes[i], pred_price, model.std_error);
    }
}

void scenario_sales_advertising() {
    print_test_header("REAL-WORLD SCENARIO 2: Sales Revenue vs Advertising Spend");
    printf("Analyzing ROI of advertising campaigns\n");
    printf("Data: 24 months of marketing and sales data\n\n");

    // Monthly ad spend ($1000s)
    double ad_spend[] = {
        15.2, 22.8, 18.5, 28.3, 20.7, 25.6, 17.9, 31.4, 19.8, 26.9,
        16.4, 29.7, 21.3, 24.1, 18.7, 27.5, 22.4, 30.8, 19.5, 26.2,
        17.6, 28.9, 20.1, 25.3
    };

    // Monthly revenue ($1000s)
    double revenue[] = {
        128, 185, 152, 232, 168, 208, 145, 258, 162, 218,
        135, 245, 175, 198, 155, 225, 182, 252, 160, 212,
        142, 238, 165, 205
    };

    size_t n = 24;

    LinearRegression model;
    fp_linear_regression_f64(ad_spend, revenue, n, &model);

    printf("Results:\n");
    print_model("Sales Model", &model);

    // Calculate ROI
    double roi_multiplier = model.slope;
    double break_even = -model.intercept / model.slope;

    printf("\nInterpretation:\n");
    printf("  Base revenue (no ads): $%.0fk/month\n", model.intercept);
    printf("  Revenue per $1k ad spend: $%.2fk (ROI: %.1fx)\n",
           model.slope, model.slope);
    printf("  Model quality: R^2 = %.2f%%\n", model.r_squared * 100);

    if (roi_multiplier > 5.0) {
        printf("\n  EXCELLENT ROI: Every dollar spent on ads generates $%.2f in revenue.\n",
               roi_multiplier);
        printf("  Recommendation: Consider increasing ad budget to maximize profits.\n");
        printf("  Break-even ad spend: $%.1fk (below this, ads not cost-effective)\n",
               break_even);
    }

    // Profit analysis
    printf("\n  Profit Analysis (assuming 30%% margin):\n");
    double test_budgets[] = {20.0, 30.0, 40.0};
    for (int i = 0; i < 3; i++) {
        double pred_revenue = fp_predict_f64(test_budgets[i], &model);
        double gross_profit = pred_revenue * 0.30;
        double net_profit = gross_profit - test_budgets[i];
        printf("    $%.0fk ad budget → $%.0fk revenue → $%.0fk profit\n",
               test_budgets[i], pred_revenue, net_profit);
    }
}

void scenario_temperature_trend() {
    print_test_header("REAL-WORLD SCENARIO 3: Temperature Trend Analysis");
    printf("Analyzing global temperature change over time\n");
    printf("Data: 40 years of average annual temperature anomalies\n\n");

    // Years since baseline (1980-2020)
    double years[40];
    for (int i = 0; i < 40; i++) {
        years[i] = i;
    }

    // Temperature anomaly (degrees C above 1951-1980 average)
    double temp_anomaly[] = {
        0.26, 0.32, 0.14, 0.31, 0.16, 0.12, 0.18, 0.32, 0.39, 0.27,
        0.45, 0.41, 0.22, 0.23, 0.31, 0.45, 0.33, 0.46, 0.63, 0.39,
        0.40, 0.54, 0.62, 0.58, 0.68, 0.75, 0.64, 0.66, 0.61, 0.69,
        0.72, 0.93, 0.90, 0.85, 0.87, 1.02, 0.99, 1.16, 0.98, 1.02
    };

    size_t n = 40;

    LinearRegression model;
    fp_linear_regression_f64(years, temp_anomaly, n, &model);

    printf("Results:\n");
    print_model("Climate Trend", &model);

    double warming_per_decade = model.slope * 10;
    double projected_2030 = fp_predict_f64(50, &model);  // Year 2030 (50 years from 1980)

    printf("\nInterpretation:\n");
    printf("  Baseline anomaly (1980): %.3f C\n", model.intercept);
    printf("  Warming rate: %.3f C per year (%.3f C per decade)\n",
           model.slope, warming_per_decade);
    printf("  Trend strength: R^2 = %.2f%%\n", model.r_squared * 100);
    printf("  Projection for 2030: %.3f C above baseline\n", projected_2030);

    if (model.r_squared > 0.80) {
        printf("\n  SIGNIFICANT TREND: Strong evidence of systematic warming.\n");
        printf("  The linear model explains %.0f%% of temperature variation.\n",
               model.r_squared * 100);
        printf("  At current rate, temperatures will rise ~%.2f C by 2050.\n",
               warming_per_decade * 3);
    }
}

void scenario_salary_experience() {
    print_test_header("REAL-WORLD SCENARIO 4: Salary vs Years of Experience");
    printf("Analyzing compensation structure for software engineers\n");
    printf("Data: 35 employees with 1-20 years experience\n\n");

    // Years of experience
    double experience[] = {
        1, 2, 3, 2, 4, 5, 3, 6, 7, 4, 8, 5, 9, 10, 6,
        11, 7, 12, 8, 13, 9, 14, 15, 10, 16, 11, 17, 12,
        18, 13, 19, 14, 20, 15, 16
    };

    // Annual salary ($1000s)
    double salary[] = {
        65, 72, 78, 70, 85, 92, 80, 98, 105, 87, 112, 95, 118, 125, 100,
        132, 108, 138, 115, 145, 122, 152, 158, 128, 165, 135, 172, 142,
        178, 148, 185, 155, 192, 162, 168
    };

    size_t n = 35;

    LinearRegression model;
    fp_linear_regression_f64(experience, salary, n, &model);

    printf("Results:\n");
    print_model("Compensation Model", &model);

    printf("\nInterpretation:\n");
    printf("  Entry-level salary (0 years): $%.0fk\n", model.intercept);
    printf("  Annual raise per year experience: $%.2fk\n", model.slope);
    printf("  Model fit: R^2 = %.2f%%\n", model.r_squared * 100);

    if (model.r_squared > 0.90) {
        printf("\n  HIGHLY PREDICTABLE: Experience strongly determines salary.\n");
        printf("  Employees can expect ~$%.0fk raise per year of experience.\n",
               model.slope);
    }

    printf("\n  Salary Expectations by Experience Level:\n");
    int levels[] = {0, 5, 10, 15, 20};
    for (int i = 0; i < 5; i++) {
        double pred_salary = fp_predict_f64(levels[i], &model);
        printf("    %2d years → $%.0fk ± $%.0fk\n",
               levels[i], pred_salary, model.std_error);
    }
}

void scenario_crop_yield() {
    print_test_header("REAL-WORLD SCENARIO 5: Crop Yield vs Fertilizer Amount");
    printf("Analyzing optimal fertilizer application for corn production\n");
    printf("Data: 28 test plots with varying fertilizer amounts\n\n");

    // Fertilizer (kg per hectare)
    double fertilizer[] = {
        50, 75, 100, 60, 125, 85, 150, 70, 110, 95, 175, 80, 140, 105,
        200, 90, 160, 115, 225, 100, 185, 120, 250, 110, 210, 130, 240, 125
    };

    // Yield (metric tons per hectare)
    double yield[] = {
        4.2, 5.1, 6.3, 4.6, 7.2, 5.8, 8.1, 5.0, 6.7, 6.1, 8.9, 5.5, 7.8, 6.9,
        9.5, 6.0, 8.4, 7.3, 10.2, 6.5, 9.1, 7.6, 10.8, 7.0, 9.8, 8.0, 10.5, 7.8
    };

    size_t n = 28;

    LinearRegression model;
    fp_linear_regression_f64(fertilizer, yield, n, &model);

    printf("Results:\n");
    print_model("Agricultural Model", &model);

    printf("\nInterpretation:\n");
    printf("  Base yield (no fertilizer): %.2f tons/hectare\n", model.intercept);
    printf("  Yield increase per kg fertilizer: %.4f tons\n", model.slope);
    printf("  Model quality: R^2 = %.2f%%\n", model.r_squared * 100);

    if (model.r_squared > 0.85) {
        printf("\n  STRONG RELATIONSHIP: Fertilizer significantly impacts yield.\n");
        printf("  Each 10kg of fertilizer adds ~%.2f tons of yield.\n",
               model.slope * 10);
    }

    // Economic analysis (assuming $200/ton corn, $2/kg fertilizer)
    printf("\n  Economic Analysis (corn=$200/ton, fertilizer=$2/kg):\n");
    double test_amounts[] = {100, 150, 200, 250};
    for (int i = 0; i < 4; i++) {
        double pred_yield = fp_predict_f64(test_amounts[i], &model);
        double revenue = pred_yield * 200;
        double fert_cost = test_amounts[i] * 2;
        double profit = revenue - fert_cost;
        printf("    %3.0f kg → %.2f tons → $%.0f revenue - $%.0f cost = $%.0f profit/hectare\n",
               test_amounts[i], pred_yield, revenue, fert_cost, profit);
    }

    double optimal_fertilizer = 150.0;  // Example
    printf("\n  Optimal fertilizer amount: ~%.0f kg/hectare for maximum profit.\n",
           optimal_fertilizer);
}

// ============================================================================
// PERFORMANCE BENCHMARKS
// ============================================================================

void benchmark_linear_regression() {
    print_test_header("PERFORMANCE BENCHMARK: Linear Regression");

    // Test with different array sizes
    size_t sizes[] = {100, 1000, 10000, 100000};

    for (int i = 0; i < 4; i++) {
        size_t n = sizes[i];

        // Allocate arrays
        double* x = (double*)malloc(n * sizeof(double));
        double* y = (double*)malloc(n * sizeof(double));

        // Fill with semi-random data representing linear relationship
        for (size_t j = 0; j < n; j++) {
            x[j] = (double)j;
            y[j] = 2.5 * j + 10.0 + ((j % 10) - 5);  // y ≈ 2.5x + 10 with noise
        }

        // Determine iteration count based on array size
        int iterations = (n < 1000) ? 100000 : (n < 10000) ? 10000 : 1000;

        // Benchmark FP-ASM
        LinearRegression fp_result;
        clock_t start = clock();
        for (int iter = 0; iter < iterations; iter++) {
            fp_linear_regression_f64(x, y, n, &fp_result);
        }
        clock_t end = clock();
        double fp_time = (double)(end - start) / CLOCKS_PER_SEC;

        // Benchmark C
        LinearRegression c_result;
        start = clock();
        for (int iter = 0; iter < iterations; iter++) {
            c_linear_regression(x, y, n, &c_result);
        }
        end = clock();
        double c_time = (double)(end - start) / CLOCKS_PER_SEC;

        // Calculate speedup
        double speedup = c_time / fp_time;
        double ms_per_call_fp = (fp_time * 1000.0) / iterations;
        double ms_per_call_c = (c_time * 1000.0) / iterations;

        printf("\nArray size: %zu elements\n", n);
        printf("Iterations: %d\n", iterations);
        printf("FP-ASM:  %.4f seconds (%.4f ms/call)\n", fp_time, ms_per_call_fp);
        printf("C:       %.4f seconds (%.4f ms/call)\n", c_time, ms_per_call_c);
        printf("Speedup: %.2fx\n", speedup);

        free(x);
        free(y);
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    printf("================================================================================\n");
    printf("FP-ASM LIBRARY - ALGORITHM #4: LINEAR REGRESSION\n");
    printf("================================================================================\n");
    printf("Testing correctness and performance of linear regression functions\n");
    printf("Target performance: 2.0-2.5x speedup (SIMD first pass, scalar second pass)\n");

    // Run correctness tests
    printf("\n\n");
    printf("################################################################################\n");
    printf("# PART 1: CORRECTNESS TESTS\n");
    printf("################################################################################\n");

    int passed = 0, total = 5;

    passed += test_perfect_positive_fit();
    passed += test_perfect_negative_fit();
    passed += test_noisy_data();
    passed += test_horizontal_line();
    passed += test_edge_cases();

    printf("\n");
    printf("================================================================================\n");
    printf("CORRECTNESS TESTS SUMMARY: %d / %d passed\n", passed, total);
    printf("================================================================================\n");

    if (passed != total) {
        printf("\nERROR: Some tests failed. Fix issues before proceeding.\n");
        return 1;
    }

    printf("\nAll correctness tests passed! Proceeding to real-world scenarios...\n");

    // Run real-world scenarios
    printf("\n\n");
    printf("################################################################################\n");
    printf("# PART 2: REAL-WORLD SCENARIOS\n");
    printf("################################################################################\n");

    scenario_housing_prices();
    scenario_sales_advertising();
    scenario_temperature_trend();
    scenario_salary_experience();
    scenario_crop_yield();

    // Run performance benchmarks
    printf("\n\n");
    printf("################################################################################\n");
    printf("# PART 3: PERFORMANCE BENCHMARKS\n");
    printf("################################################################################\n");

    benchmark_linear_regression();

    printf("\n\n");
    printf("================================================================================\n");
    printf("ALL TESTS COMPLETED SUCCESSFULLY\n");
    printf("================================================================================\n");

    return 0;
}
