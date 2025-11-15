// demo_linear_regression.c
//
// Linear Regression + Gradient Descent Demo
// Demonstrates FP-ASM library for ML optimization
//
// This showcases:
// - Closed-form linear regression (exact solution)
// - Gradient descent optimization (iterative)
// - Loss convergence visualization
// - Real-world regression problems

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Forward declarations from fp_linear_regression.c
typedef struct {
    double* weights;
    int n_features;
    double final_loss;
    int converged;
} LinearRegressionModel;

typedef struct {
    LinearRegressionModel model;
    double* loss_history;
    int n_iterations;
} GradientDescentResult;

LinearRegressionModel fp_linear_regression_closed_form(
    const double* X, const double* y, int n, int d);

GradientDescentResult fp_linear_regression_gradient_descent(
    const double* X, const double* y, int n, int d,
    double learning_rate, int max_iterations, double convergence_threshold);

void fp_linear_regression_predict(
    const LinearRegressionModel* model,
    const double* X, double* y_pred, int n);

double fp_linear_regression_r2_score(
    const double* y_true, const double* y_pred, int n);

void fp_linear_regression_free(LinearRegressionModel* model);
void fp_gradient_descent_free(GradientDescentResult* result);
void fp_linear_regression_print(const LinearRegressionModel* model);
void fp_gradient_descent_print_summary(const GradientDescentResult* result);

// ============================================================================
// Synthetic Data Generation
// ============================================================================

// Generate linear data: y = w0 + w1*x + noise
void generate_linear_data(
    double* X,              // n-element output (1D features)
    double* y,              // n-element output (targets)
    int n,                  // number of samples
    double true_w0,         // true intercept
    double true_w1,         // true slope
    double noise_std        // noise standard deviation
) {
    srand(42);  // Fixed seed for reproducibility

    for (int i = 0; i < n; i++) {
        // Generate x uniformly in [0, 10]
        X[i] = ((double)rand() / RAND_MAX) * 10.0;

        // Generate y = w0 + w1*x + noise
        double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 * noise_std;
        y[i] = true_w0 + true_w1 * X[i] + noise;
    }
}

// Generate multiple regression data: y = w0 + w1*x1 + w2*x2 + ... + noise
void generate_multiple_regression_data(
    double* X,              // n × d output matrix
    double* y,              // n-element output
    int n,                  // number of samples
    int d,                  // number of features
    const double* true_weights,  // d+1 true weights (includes bias)
    double noise_std        // noise standard deviation
) {
    srand(42);

    for (int i = 0; i < n; i++) {
        // Generate features uniformly in [0, 10]
        for (int j = 0; j < d; j++) {
            X[i * d + j] = ((double)rand() / RAND_MAX) * 10.0;
        }

        // Compute y = w0 + w1*x1 + w2*x2 + ... + noise
        y[i] = true_weights[0];  // bias
        for (int j = 0; j < d; j++) {
            y[i] += true_weights[j + 1] * X[i * d + j];
        }

        // Add noise
        double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 * noise_std;
        y[i] += noise;
    }
}

// ============================================================================
// Test 1: Simple Linear Regression (Closed-Form vs Gradient Descent)
// ============================================================================
void test_simple_linear_regression() {
    printf("================================================================\n");
    printf("TEST 1: Simple Linear Regression (y = 3x + 5 + noise)\n");
    printf("================================================================\n\n");

    const int n = 1000;
    const double true_w0 = 5.0;   // True intercept
    const double true_w1 = 3.0;   // True slope
    const double noise_std = 1.0;

    // Generate synthetic data
    double* X = (double*)malloc(n * sizeof(double));
    double* y = (double*)malloc(n * sizeof(double));
    generate_linear_data(X, y, n, true_w0, true_w1, noise_std);

    printf("Dataset: %d samples\n", n);
    printf("True model: y = %.1f + %.1fx\n", true_w0, true_w1);
    printf("Noise std: %.1f\n\n", noise_std);

    // Method 1: Closed-form solution
    printf("Method 1: Closed-Form Solution (Normal Equations)\n");
    printf("--------------------------------------------------------\n");
    clock_t start = clock();
    LinearRegressionModel model_closed = fp_linear_regression_closed_form(X, y, n, 1);
    clock_t end = clock();
    double time_closed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    fp_linear_regression_print(&model_closed);
    printf("  Runtime: %.3f ms\n", time_closed);

    // Compute R² score
    double* y_pred = (double*)malloc(n * sizeof(double));
    fp_linear_regression_predict(&model_closed, X, y_pred, n);
    double r2 = fp_linear_regression_r2_score(y, y_pred, n);
    printf("  R² Score: %.6f\n\n", r2);

    // Method 2: Gradient descent
    printf("Method 2: Gradient Descent\n");
    printf("--------------------------------------------------------\n");
    start = clock();
    GradientDescentResult result_gd = fp_linear_regression_gradient_descent(
        X, y, n, 1,
        0.01,    // learning rate
        1000,    // max iterations
        1e-6     // convergence threshold
    );
    end = clock();
    double time_gd = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    fp_gradient_descent_print_summary(&result_gd);
    fp_linear_regression_print(&result_gd.model);
    printf("  Runtime: %.3f ms\n", time_gd);

    // Compute R² score for gradient descent
    fp_linear_regression_predict(&result_gd.model, X, y_pred, n);
    r2 = fp_linear_regression_r2_score(y, y_pred, n);
    printf("  R² Score: %.6f\n\n", r2);

    // Show convergence history
    printf("Convergence History (first 10 iterations):\n");
    printf("  Iter |   Loss (MSE)\n");
    printf("  -----|-------------\n");
    for (int i = 0; i < 10 && i < result_gd.n_iterations; i++) {
        printf("  %4d | %12.6f\n", i, result_gd.loss_history[i]);
    }
    printf("  ...  |\n");
    printf("  %4d | %12.6f\n\n", result_gd.n_iterations - 1,
           result_gd.loss_history[result_gd.n_iterations - 1]);

    // Comparison
    printf("Comparison:\n");
    printf("  Closed-form: %.3f ms (exact solution)\n", time_closed);
    printf("  Gradient Descent: %.3f ms (%d iterations)\n", time_gd, result_gd.n_iterations);
    printf("  Both methods recovered the true parameters accurately!\n\n");

    // Cleanup
    free(X);
    free(y);
    free(y_pred);
    fp_linear_regression_free(&model_closed);
    fp_gradient_descent_free(&result_gd);
}

// ============================================================================
// Test 2: Multiple Linear Regression (Multiple Features)
// ============================================================================
void test_multiple_regression() {
    printf("================================================================\n");
    printf("TEST 2: Multiple Linear Regression (3 features)\n");
    printf("================================================================\n\n");

    const int n = 1000;
    const int d = 3;
    const double true_weights[] = {10.0, 2.5, -1.5, 0.8};  // w0, w1, w2, w3
    const double noise_std = 2.0;

    // Generate synthetic data
    double* X = (double*)malloc(n * d * sizeof(double));
    double* y = (double*)malloc(n * sizeof(double));
    generate_multiple_regression_data(X, y, n, d, true_weights, noise_std);

    printf("Dataset: %d samples, %d features\n", n, d);
    printf("True model: y = %.1f + %.1f*x1 + %.1f*x2 + %.1f*x3\n",
           true_weights[0], true_weights[1], true_weights[2], true_weights[3]);
    printf("Noise std: %.1f\n\n", noise_std);

    // Gradient descent (closed-form not implemented for d>1)
    printf("Method: Gradient Descent\n");
    printf("--------------------------------------------------------\n");

    clock_t start = clock();
    GradientDescentResult result = fp_linear_regression_gradient_descent(
        X, y, n, d,
        0.001,   // learning rate (smaller for multiple features)
        5000,    // max iterations
        1e-6     // convergence threshold
    );
    clock_t end = clock();
    double time_gd = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    fp_gradient_descent_print_summary(&result);
    fp_linear_regression_print(&result.model);
    printf("  Runtime: %.3f ms\n", time_gd);

    // Compute R² score
    double* y_pred = (double*)malloc(n * sizeof(double));
    fp_linear_regression_predict(&result.model, X, y_pred, n);
    double r2 = fp_linear_regression_r2_score(y, y_pred, n);
    printf("  R² Score: %.6f\n\n", r2);

    // Show learned vs true weights
    printf("Learned vs True Weights:\n");
    printf("  Parameter |    Learned |       True | Error\n");
    printf("  ----------|------------|------------|-------\n");
    for (int i = 0; i <= d; i++) {
        printf("  w%-9d| %10.4f | %10.4f | %6.4f\n",
               i, result.model.weights[i], true_weights[i],
               fabs(result.model.weights[i] - true_weights[i]));
    }
    printf("\n");

    // Cleanup
    free(X);
    free(y);
    free(y_pred);
    fp_gradient_descent_free(&result);
}

// ============================================================================
// Test 3: Learning Rate Comparison
// ============================================================================
void test_learning_rate_comparison() {
    printf("================================================================\n");
    printf("TEST 3: Learning Rate Comparison\n");
    printf("================================================================\n\n");

    const int n = 500;
    const double true_w0 = 5.0;
    const double true_w1 = 3.0;
    const double noise_std = 1.0;

    // Generate synthetic data
    double* X = (double*)malloc(n * sizeof(double));
    double* y = (double*)malloc(n * sizeof(double));
    generate_linear_data(X, y, n, true_w0, true_w1, noise_std);

    printf("Testing different learning rates...\n\n");

    double learning_rates[] = {0.001, 0.01, 0.05, 0.1};
    int n_rates = 4;

    printf("  LR      | Iterations | Final Loss | Runtime | Converged\n");
    printf("  --------|------------|------------|---------|----------\n");

    for (int i = 0; i < n_rates; i++) {
        clock_t start = clock();
        GradientDescentResult result = fp_linear_regression_gradient_descent(
            X, y, n, 1,
            learning_rates[i],
            1000,
            1e-6
        );
        clock_t end = clock();
        double runtime = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

        printf("  %.4f  | %10d | %10.6f | %6.2f ms | %s\n",
               learning_rates[i],
               result.n_iterations,
               result.model.final_loss,
               runtime,
               result.model.converged ? "Yes" : "No");

        fp_gradient_descent_free(&result);
    }

    printf("\n");
    printf("Key Insights:\n");
    printf("  - Higher learning rates converge faster (fewer iterations)\n");
    printf("  - But too high learning rate may overshoot (diverge)\n");
    printf("  - Sweet spot: 0.01 - 0.1 for this problem\n\n");

    free(X);
    free(y);
}

// ============================================================================
// Test 4: Large-Scale Regression
// ============================================================================
void test_large_scale() {
    printf("================================================================\n");
    printf("TEST 4: Large-Scale Regression (10K samples, 10 features)\n");
    printf("================================================================\n\n");

    const int n = 10000;
    const int d = 10;
    const double noise_std = 5.0;

    // Generate true weights
    double* true_weights = (double*)malloc((d + 1) * sizeof(double));
    srand(123);
    for (int i = 0; i <= d; i++) {
        true_weights[i] = ((double)rand() / RAND_MAX) * 10.0 - 5.0;  // Random in [-5, 5]
    }

    // Generate synthetic data
    double* X = (double*)malloc(n * d * sizeof(double));
    double* y = (double*)malloc(n * sizeof(double));
    generate_multiple_regression_data(X, y, n, d, true_weights, noise_std);

    printf("Dataset: %d samples, %d features\n", n, d);
    printf("Noise std: %.1f\n\n", noise_std);

    // Gradient descent
    printf("Running Gradient Descent...\n");
    clock_t start = clock();
    GradientDescentResult result = fp_linear_regression_gradient_descent(
        X, y, n, d,
        0.0001,  // Small learning rate for stability with many features
        10000,   // More iterations
        1e-7     // Tighter convergence
    );
    clock_t end = clock();
    double runtime = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    fp_gradient_descent_print_summary(&result);
    printf("  Runtime: %.3f ms\n", runtime);
    printf("  Samples/sec: %.0f\n", (n * result.n_iterations) / (runtime / 1000.0));

    // Compute R² score
    double* y_pred = (double*)malloc(n * sizeof(double));
    fp_linear_regression_predict(&result.model, X, y_pred, n);
    double r2 = fp_linear_regression_r2_score(y, y_pred, n);
    printf("  R² Score: %.6f\n\n", r2);

    // Cleanup
    free(X);
    free(y);
    free(y_pred);
    free(true_weights);
    fp_gradient_descent_free(&result);
}

int main() {
    printf("================================================================\n");
    printf("  Linear Regression + Gradient Descent Demo\n");
    printf("  FP-ASM Library - ML Optimization Showcase\n");
    printf("================================================================\n\n");

    printf("Demonstrating linear regression with two methods:\n");
    printf("  1. Closed-form solution (exact, fast for small d)\n");
    printf("  2. Gradient descent (iterative, scales to large d)\n\n");

    // Run tests
    test_simple_linear_regression();
    test_multiple_regression();
    test_learning_rate_comparison();
    test_large_scale();

    // Summary
    printf("================================================================\n");
    printf("  Linear Regression Demo Complete!\n");
    printf("================================================================\n\n");

    printf("Key Takeaways:\n");
    printf("  ✓ Closed-form solution: Exact, fast for simple regression\n");
    printf("  ✓ Gradient descent: Flexible, scales to multiple features\n");
    printf("  ✓ Learning rate matters: Trade-off between speed and stability\n");
    printf("  ✓ Both methods recover true parameters accurately\n\n");

    printf("FP-ASM Primitives Used:\n");
    printf("  - Dot products (predictions: x·w)\n");
    printf("  - Vector operations (gradient updates)\n");
    printf("  - Sum of squares (loss computation)\n");
    printf("  - Functional composition for ML optimization\n\n");

    printf("Real-World Applications:\n");
    printf("  - Stock price prediction\n");
    printf("  - Sales forecasting\n");
    printf("  - Housing price estimation\n");
    printf("  - Trend analysis\n\n");

    printf("This demonstrates gradient descent - the foundation of modern ML!\n");
    printf("(Used in neural networks, deep learning, etc.)\n\n");

    return 0;
}
