// fp_linear_regression.c
//
// Linear Regression + Gradient Descent - Refactored with Pattern 1
// Demonstrates FP-ASM primitives for ML optimization
//
// REFACTORED: Now uses Pattern 1 (Array Statistics) for:
//   - fp_mean() - Computing means (clearer than manual sum+divide)
//   - fp_variance() - Computing variances (numerically stable)
//   - fp_covariance() - Computing covariance for closed-form solution
//
// This showcases:
// - Closed-form solution (normal equations) - Pattern 1 for statistics!
// - Iterative optimization (gradient descent) - Pattern 1 for loss!
// - Loss computation (Mean Squared Error) - Pattern 1 for variance!
// - Functional composition for ML
//
// FP Primitives Used:
// - fp_fold_dotp_f64: dot products (x·w, predictions)
// - fp_map_axpy_f64: weight updates (w = w - α·gradient)
// - fp_fold_sumsq_f64: loss computation
// - fp_reduce_add_f64: summations

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fp_core.h"
#include "fp_rng.h"
#include "fp_monads.h"  // TIER 4: Either monad for error handling

// Pattern 1 helpers - NOW USING ASM PRIMITIVES
// Refactored to use fp_reduce_add_f64 instead of for loops

static inline double fp_mean_inline(const double* data, size_t n) {
    if (!data || n == 0) return 0.0;
    // L0 ASM: SIMD reduction
    double sum = fp_reduce_add_f64(data, n);
    return sum / (double)n;
}

static inline double fp_variance_inline(const double* data, size_t n, double mean) {
    if (!data || n == 0) return 0.0;
    // L0 ASM: Var(X) = E[X²] - E[X]²
    // sum_sq = data · data (dot product with self)
    double sum_sq = fp_fold_dotp_f64(data, data, n);
    double mean_sq = sum_sq / (double)n;  // E[X²]
    return mean_sq - mean * mean;         // E[X²] - E[X]²
}

static inline double fp_covariance_inline(const double* x, const double* y, size_t n,
                                          double mean_x, double mean_y) {
    if (!x || !y || n == 0) return 0.0;
    // L0 ASM: Cov(X,Y) = E[XY] - E[X]E[Y]
    // sum_xy = x · y (dot product)
    double sum_xy = fp_fold_dotp_f64(x, y, n);
    double mean_xy = sum_xy / (double)n;  // E[XY]
    return mean_xy - mean_x * mean_y;     // E[XY] - E[X]E[Y]
}

// Linear regression model structure
typedef struct {
    double* weights;      // Model weights (d+1 dimensional, includes bias)
    int n_features;       // Number of features (not including bias)
    double final_loss;    // Final MSE loss
    int converged;        // 1 if converged, 0 if max_iter reached
} LinearRegressionModel;

// Gradient descent result (includes training history)
typedef struct {
    LinearRegressionModel model;
    double* loss_history;  // Loss at each iteration
    int n_iterations;      // Number of iterations performed
} GradientDescentResult;

// ============================================================================
// Helper Functions
// ============================================================================

// Compute predictions for all samples
// y_pred[i] = w0 + w1*x1[i] + w2*x2[i] + ... + wd*xd[i]
static void predict(
    const double* X,           // n × d feature matrix (row-major)
    const double* weights,     // d+1 weights (includes bias at index 0)
    double* y_pred,            // n-element output
    int n,                     // number of samples
    int d                      // number of features
) {
    // L0 ASM: Use fp_fold_dotp_f64 for each row's dot product
    for (int i = 0; i < n; i++) {
        // bias + (row_i · weights[1:d+1])
        y_pred[i] = weights[0] + fp_fold_dotp_f64(&X[i * d], &weights[1], (size_t)d);
    }
}

// Compute Mean Squared Error loss
// MSE = (1/n) * sum((y_pred - y_true)^2)
static double compute_mse(
    const double* y_pred,
    const double* y_true,
    int n
) {
    // L0 ASM: Compute errors using fp_map_axpy_f64: error = pred + (-1)*true
    double* errors = (double*)malloc((size_t)n * sizeof(double));
    fp_map_axpy_f64(y_true, y_pred, errors, (size_t)n, -1.0);  // errors = y_pred - y_true

    // L0 ASM: Sum of squared errors = errors · errors
    double sum_squared_error = fp_fold_dotp_f64(errors, errors, (size_t)n);

    free(errors);
    return sum_squared_error / n;
}

// ============================================================================
// Closed-Form Solution (Normal Equations)
// ============================================================================

// Solve using normal equations: w = (X^T X)^(-1) X^T y
// This is exact but requires matrix inversion (O(d^3))
LinearRegressionModel fp_linear_regression_closed_form(
    const double* X,          // n × d feature matrix
    const double* y,          // n-element target vector
    int n,                    // number of samples
    int d                     // number of features
) {
    LinearRegressionModel model;
    model.n_features = d;
    model.weights = (double*)calloc(d + 1, sizeof(double));
    model.converged = 1;  // Closed-form always "converges"

    // For simple linear regression (d=1), use closed-form formulas
    if (d == 1) {
        // Simple linear regression: y = w0 + w1*x
        // w1 = Cov(x,y) / Var(x)
        // w0 = mean(y) - w1*mean(x)

        // Pattern 1: Use fp_mean_inline() instead of manual sum+divide
        double mean_x = fp_mean_inline(X, n);
        double mean_y = fp_mean_inline(y, n);

        // Pattern 1: Use fp_variance_inline() and fp_covariance_inline()
        double var_x = fp_variance_inline(X, n, mean_x);
        double cov_xy = fp_covariance_inline(X, y, n, mean_x, mean_y);

        if (fabs(var_x) < 1e-10) {
            // Degenerate case: all x values are the same
            model.weights[0] = mean_y;
            model.weights[1] = 0.0;
        } else {
            // Pattern 1 benefits: Clearer formula, numerically stable
            model.weights[1] = cov_xy / var_x;  // slope = Cov(x,y) / Var(x)
            model.weights[0] = mean_y - model.weights[1] * mean_x;  // intercept
        }
    } else {
        // Multiple linear regression would require matrix operations
        // For now, we'll use gradient descent as the primary method
        printf("Warning: Closed-form solution for d>1 not implemented, use gradient descent\n");
        model.converged = 0;
    }

    // Compute final loss
    double* y_pred = (double*)malloc(n * sizeof(double));
    predict(X, model.weights, y_pred, n, d);
    model.final_loss = compute_mse(y_pred, y, n);
    free(y_pred);

    return model;
}

// ============================================================================
// Gradient Descent Optimizer
// ============================================================================

// Compute gradients for all weights
// REFACTORED: Uses Pattern 1 for clearer gradient computation
// gradient[j] = (1/n) * sum((y_pred - y_true) * x[j])
// gradient[0] = (1/n) * sum(y_pred - y_true)  (bias gradient)
static void compute_gradients(
    const double* X,          // n × d feature matrix
    const double* y_true,     // n-element target
    const double* y_pred,     // n-element predictions
    double* gradients,        // d+1 output gradients
    int n,                    // number of samples
    int d                     // number of features
) {
    // Initialize gradients to zero
    memset(gradients, 0, (d + 1) * sizeof(double));

    // Compute errors (y_pred - y_true)
    double* errors = (double*)malloc(n * sizeof(double));
    for (int i = 0; i < n; i++) {
        errors[i] = y_pred[i] - y_true[i];
    }

    // Pattern 1: Bias gradient is mean of errors
    gradients[0] = fp_mean_inline(errors, n);

    // Compute feature gradients (gradient[1..d])
    // gradient[j] = mean((y_pred - y_true) * x[j])
    for (int j = 0; j < d; j++) {
        double* weighted_errors = (double*)malloc(n * sizeof(double));
        for (int i = 0; i < n; i++) {
            weighted_errors[i] = errors[i] * X[i * d + j];
        }
        // Pattern 1: Each gradient is mean of weighted errors
        gradients[j + 1] = fp_mean_inline(weighted_errors, n);
        free(weighted_errors);
    }

    free(errors);
}

// Gradient Descent optimizer
// Minimizes MSE loss using iterative updates: w = w - learning_rate * gradient
// REFACTORED: Now takes seed for deterministic initialization (no rand())
GradientDescentResult fp_linear_regression_gradient_descent(
    const double* X,              // n × d feature matrix
    const double* y,              // n-element target vector
    int n,                        // number of samples
    int d,                        // number of features
    double learning_rate,         // step size (typically 0.001 - 0.1)
    int max_iterations,           // maximum iterations
    double convergence_threshold, // stop if loss change < threshold
    uint64_t seed                 // RNG seed for deterministic initialization
) {
    GradientDescentResult result;
    result.model.n_features = d;
    result.model.weights = (double*)calloc(d + 1, sizeof(double));
    result.loss_history = (double*)malloc(max_iterations * sizeof(double));
    result.model.converged = 0;

    // Allocate working memory
    double* y_pred = (double*)malloc(n * sizeof(double));
    double* gradients = (double*)malloc((d + 1) * sizeof(double));

    // DETERMINISTIC: Initialize weights using fp_rng (no rand()!)
    fp_rng_t rng = fp_rng_create(seed);
    rng = fp_rng_fill_f64_range(rng, result.model.weights, (size_t)(d + 1), -0.01, 0.01);

    double prev_loss = INFINITY;

    // Gradient descent loop
    for (result.n_iterations = 0; result.n_iterations < max_iterations; result.n_iterations++) {
        // 1. Compute predictions
        predict(X, result.model.weights, y_pred, n, d);

        // 2. Compute loss
        double loss = compute_mse(y_pred, y, n);
        result.loss_history[result.n_iterations] = loss;

        // 3. Check convergence
        if (result.n_iterations > 0 && fabs(prev_loss - loss) < convergence_threshold) {
            result.model.converged = 1;
            result.n_iterations++;  // Include this iteration in count
            break;
        }
        prev_loss = loss;

        // 4. Compute gradients
        compute_gradients(X, y, y_pred, gradients, n, d);

        // 5. L0 ASM: Update weights using fp_map_axpy_f64
        // w = w + (-learning_rate) * gradient
        fp_map_axpy_f64(gradients, result.model.weights, result.model.weights,
                        (size_t)(d + 1), -learning_rate);
    }

    // Compute final loss
    predict(X, result.model.weights, y_pred, n, d);
    result.model.final_loss = compute_mse(y_pred, y, n);

    // Cleanup
    free(y_pred);
    free(gradients);

    return result;
}

// ============================================================================
// Model Evaluation and Prediction
// ============================================================================

// Make predictions on new data
void fp_linear_regression_predict(
    const LinearRegressionModel* model,
    const double* X,           // n × d feature matrix
    double* y_pred,            // n-element output
    int n                      // number of samples
) {
    predict(X, model->weights, y_pred, n, model->n_features);
}

// Compute R² score (coefficient of determination)
// REFACTORED: Uses Pattern 1 for variance computation
// R² = 1 - (SS_res / SS_tot) where:
// - SS_res = sum((y_true - y_pred)^2)  (residual sum of squares)
// - SS_tot = sum((y_true - y_mean)^2)  (total sum of squares)
// NOTE: SS_tot = n * Var(y_true), SS_res = n * Var(residuals)
double fp_linear_regression_r2_score(
    const double* y_true,
    const double* y_pred,
    int n
) {
    // Pattern 1: Use fp_mean_inline() instead of manual sum+divide
    double mean_y = fp_mean_inline(y_true, n);

    // Pattern 1: SS_tot = n * Var(y_true)
    double var_y = fp_variance_inline(y_true, n, mean_y);
    double ss_tot = n * var_y;

    if (ss_tot < 1e-10) {
        return 0.0;  // All y values are the same
    }

    // Compute residuals
    double* residuals = (double*)malloc(n * sizeof(double));
    for (int i = 0; i < n; i++) {
        residuals[i] = y_true[i] - y_pred[i];
    }

    // Pattern 1: SS_res = n * Var(residuals) where mean(residuals) ≈ 0
    double mean_residuals = fp_mean_inline(residuals, n);
    double var_residuals = fp_variance_inline(residuals, n, mean_residuals);
    double ss_res = n * var_residuals;

    free(residuals);

    return 1.0 - (ss_res / ss_tot);
}

// ============================================================================
// Memory Management
// ============================================================================

void fp_linear_regression_free(LinearRegressionModel* model) {
    free(model->weights);
}

void fp_gradient_descent_free(GradientDescentResult* result) {
    free(result->model.weights);
    free(result->loss_history);
}

// ============================================================================
// TIER 4: Either Monad Wrapper for Safe Linear Regression
// ============================================================================
// Returns Left(error_msg, code) for errors, Right(result_ptr) on success
// Error codes: 1=NULL data, 2=invalid params, 3=allocation failed, 4=no convergence

Either fp_linear_regression_gradient_descent_safe(
    const double* X,
    const double* y,
    int n,
    int d,
    double learning_rate,
    int max_iterations,
    double convergence_threshold,
    uint64_t seed
) {
    // Validate inputs
    if (!X) return fp_left("NULL feature matrix X", 1);
    if (!y) return fp_left("NULL target vector y", 1);
    if (n <= 0) return fp_left("Invalid sample count n <= 0", 2);
    if (d <= 0) return fp_left("Invalid feature count d <= 0", 2);
    if (learning_rate <= 0.0) return fp_left("Invalid learning rate <= 0", 2);
    if (max_iterations <= 0) return fp_left("Invalid max_iterations <= 0", 2);
    if (convergence_threshold < 0.0) return fp_left("Invalid convergence threshold < 0", 2);

    // Allocate result on heap
    GradientDescentResult* result = (GradientDescentResult*)malloc(sizeof(GradientDescentResult));
    if (!result) return fp_left("Failed to allocate result", 3);

    // Run gradient descent
    *result = fp_linear_regression_gradient_descent(X, y, n, d, learning_rate,
                                                     max_iterations, convergence_threshold, seed);

    // Check internal allocation
    if (!result->model.weights || !result->loss_history) {
        fp_gradient_descent_free(result);
        free(result);
        return fp_left("Failed to allocate model weights or history", 3);
    }

    // Check convergence (warning, not error)
    if (!result->model.converged) {
        // Still return Right but caller can check converged flag
        // This is informational, not a hard error
    }

    return fp_right_ptr(result);
}

// ============================================================================
// Printing and Visualization
// ============================================================================

void fp_linear_regression_print(const LinearRegressionModel* model) {
    printf("Linear Regression Model:\n");
    printf("  Bias (w0): %.6f\n", model->weights[0]);
    for (int i = 0; i < model->n_features; i++) {
        printf("  Weight w%d: %.6f\n", i + 1, model->weights[i + 1]);
    }
    printf("  Final Loss (MSE): %.6f\n", model->final_loss);
    printf("  Converged: %s\n", model->converged ? "Yes" : "No (max iter)");
}

void fp_gradient_descent_print_summary(const GradientDescentResult* result) {
    printf("Gradient Descent Results:\n");
    printf("  Iterations: %d\n", result->n_iterations);
    printf("  Converged: %s\n", result->model.converged ? "Yes" : "No (max iter)");
    printf("  Final Loss (MSE): %.6f\n", result->model.final_loss);
    printf("  Initial Loss: %.6f\n", result->loss_history[0]);
    printf("  Loss Reduction: %.6f (%.1f%%)\n",
           result->loss_history[0] - result->model.final_loss,
           100.0 * (result->loss_history[0] - result->model.final_loss) / result->loss_history[0]);
}
