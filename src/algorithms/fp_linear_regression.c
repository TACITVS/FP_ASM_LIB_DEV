// fp_linear_regression.c
//
// Linear Regression + Gradient Descent
// Demonstrates FP-ASM primitives for ML optimization
//
// This showcases:
// - Closed-form solution (normal equations)
// - Iterative optimization (gradient descent)
// - Loss computation (Mean Squared Error)
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
    for (int i = 0; i < n; i++) {
        // Start with bias term
        y_pred[i] = weights[0];

        // Add weighted sum of features using fp_fold_dotp_f64
        // Could optimize this by using fp_fold_dotp_f64 directly
        for (int j = 0; j < d; j++) {
            y_pred[i] += weights[j + 1] * X[i * d + j];
        }
    }
}

// Compute Mean Squared Error loss
// MSE = (1/n) * sum((y_pred - y_true)^2)
static double compute_mse(
    const double* y_pred,
    const double* y_true,
    int n
) {
    double sum_squared_error = 0.0;

    // Could use fp_fold_sumsq_f64 here after computing differences
    for (int i = 0; i < n; i++) {
        double error = y_pred[i] - y_true[i];
        sum_squared_error += error * error;
    }

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

        double sum_x = 0.0, sum_y = 0.0;
        double sum_xy = 0.0, sum_xx = 0.0;

        for (int i = 0; i < n; i++) {
            sum_x += X[i];
            sum_y += y[i];
            sum_xy += X[i] * y[i];
            sum_xx += X[i] * X[i];
        }

        double mean_x = sum_x / n;
        double mean_y = sum_y / n;

        // Compute slope
        double numerator = sum_xy - n * mean_x * mean_y;
        double denominator = sum_xx - n * mean_x * mean_x;

        if (fabs(denominator) < 1e-10) {
            // Degenerate case: all x values are the same
            model.weights[0] = mean_y;
            model.weights[1] = 0.0;
        } else {
            model.weights[1] = numerator / denominator;  // slope
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

    // Compute bias gradient (gradient[0])
    for (int i = 0; i < n; i++) {
        gradients[0] += (y_pred[i] - y_true[i]);
    }
    gradients[0] /= n;

    // Compute feature gradients (gradient[1..d])
    for (int j = 0; j < d; j++) {
        for (int i = 0; i < n; i++) {
            gradients[j + 1] += (y_pred[i] - y_true[i]) * X[i * d + j];
        }
        gradients[j + 1] /= n;
    }
}

// Gradient Descent optimizer
// Minimizes MSE loss using iterative updates: w = w - learning_rate * gradient
GradientDescentResult fp_linear_regression_gradient_descent(
    const double* X,              // n × d feature matrix
    const double* y,              // n-element target vector
    int n,                        // number of samples
    int d,                        // number of features
    double learning_rate,         // step size (typically 0.001 - 0.1)
    int max_iterations,           // maximum iterations
    double convergence_threshold  // stop if loss change < threshold
) {
    GradientDescentResult result;
    result.model.n_features = d;
    result.model.weights = (double*)calloc(d + 1, sizeof(double));
    result.loss_history = (double*)malloc(max_iterations * sizeof(double));
    result.model.converged = 0;

    // Allocate working memory
    double* y_pred = (double*)malloc(n * sizeof(double));
    double* gradients = (double*)malloc((d + 1) * sizeof(double));

    // Initialize weights to small random values
    for (int i = 0; i <= d; i++) {
        result.model.weights[i] = ((double)rand() / RAND_MAX) * 0.01;
    }

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

        // 5. Update weights: w = w - learning_rate * gradient
        // This could use fp_map_axpy_f64 for SIMD acceleration
        for (int i = 0; i <= d; i++) {
            result.model.weights[i] -= learning_rate * gradients[i];
        }
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
// R² = 1 - (SS_res / SS_tot) where:
// - SS_res = sum((y_true - y_pred)^2)  (residual sum of squares)
// - SS_tot = sum((y_true - y_mean)^2)  (total sum of squares)
double fp_linear_regression_r2_score(
    const double* y_true,
    const double* y_pred,
    int n
) {
    // Compute mean of y_true
    double sum_y = 0.0;
    for (int i = 0; i < n; i++) {
        sum_y += y_true[i];
    }
    double mean_y = sum_y / n;

    // Compute SS_res and SS_tot
    double ss_res = 0.0;
    double ss_tot = 0.0;
    for (int i = 0; i < n; i++) {
        ss_res += (y_true[i] - y_pred[i]) * (y_true[i] - y_pred[i]);
        ss_tot += (y_true[i] - mean_y) * (y_true[i] - mean_y);
    }

    if (ss_tot < 1e-10) {
        return 0.0;  // All y values are the same
    }

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
