/**
 * fp_linear_regression.h - Linear Regression Algorithm API
 *
 * Provides linear regression with both standard and safe (Either monad) interfaces.
 * Uses L0 ASM primitives internally for SIMD-accelerated computation.
 *
 * Features:
 * - Closed-form solution (normal equations)
 * - Gradient descent with configurable learning rate
 * - Safe variant returning Either monad for error handling
 * - R-squared scoring
 *
 * Example (standard - closed-form):
 *   LinearRegressionModel model = fp_linear_regression_closed_form(x, y, n);
 *   double prediction = model.weights[0] + model.weights[1] * new_x;
 *   fp_linear_regression_free(&model);
 *
 * Example (standard - gradient descent):
 *   GradientDescentResult result = fp_linear_regression_gradient_descent(
 *       X, y, n, d, 0.01, 1000, 1e-6, 42);
 *   fp_gradient_descent_print_summary(&result);
 *   fp_gradient_descent_free(&result);
 *
 * Example (safe/monadic):
 *   Either result = fp_linear_regression_gradient_descent_safe(
 *       X, y, n, d, 0.01, 1000, 1e-6, 42);
 *   if (fp_is_right(result)) {
 *       GradientDescentResult* r = (GradientDescentResult*)fp_from_right_ptr(result);
 *       // Use result...
 *       fp_gradient_descent_free(r);  // Free internal arrays
 *       free(r);                      // Free the result struct itself
 *   } else {
 *       printf("Error: %s (code %d)\n",
 *              fp_from_left_msg(result), fp_from_left_code(result));
 *   }
 */

#ifndef FP_LINEAR_REGRESSION_H
#define FP_LINEAR_REGRESSION_H

#include <stdint.h>
#include <stddef.h>
#include "fp_monads.h"

/* ============================================================================
 * LINEAR REGRESSION STRUCTURES
 * ============================================================================ */

/**
 * LinearRegressionModel - Trained linear regression model
 */
typedef struct {
    double* weights;      // Model weights (d+1 dimensional, includes bias at index 0)
    int n_features;       // Number of features (not including bias)
    double final_loss;    // Final MSE loss
    int converged;        // 1 if converged, 0 if max_iter reached
} LinearRegressionModel;

/**
 * GradientDescentResult - Result of gradient descent training
 */
typedef struct {
    LinearRegressionModel model;
    double* loss_history;  // Loss at each iteration
    int n_iterations;      // Number of iterations performed
} GradientDescentResult;

/* ============================================================================
 * STANDARD LINEAR REGRESSION API
 * ============================================================================ */

/**
 * fp_linear_regression_closed_form - Closed-form regression (normal equations)
 *
 * @param X         n x d feature matrix (row-major)
 * @param y         Target values (n elements)
 * @param n         Number of samples
 * @param d         Number of features (d=1 for simple linear regression)
 *
 * @return LinearRegressionModel with weights[0]=bias, weights[1..d]=coefficients
 */
LinearRegressionModel fp_linear_regression_closed_form(
    const double* X,
    const double* y,
    int n,
    int d
);

/**
 * fp_linear_regression_gradient_descent - Multi-feature gradient descent
 *
 * @param X             n x d feature matrix (row-major, contiguous)
 * @param y             Target values (n elements)
 * @param n             Number of samples
 * @param d             Number of features
 * @param learning_rate Learning rate (alpha)
 * @param max_iterations Maximum iterations
 * @param convergence_threshold Threshold for early stopping
 * @param seed          RNG seed for weight initialization
 *
 * @return GradientDescentResult with model and training history
 *
 * NOTE: Does NOT validate inputs. For safe version with validation,
 *       use fp_linear_regression_gradient_descent_safe().
 */
GradientDescentResult fp_linear_regression_gradient_descent(
    const double* X,
    const double* y,
    int n,
    int d,
    double learning_rate,
    int max_iterations,
    double convergence_threshold,
    uint64_t seed
);

/**
 * fp_linear_regression_predict - Make predictions using trained model
 *
 * @param model     Trained LinearRegressionModel
 * @param X         n x d feature matrix (d = model->n_features)
 * @param y_pred    Output predictions (n elements, must be pre-allocated)
 * @param n         Number of samples
 */
void fp_linear_regression_predict(
    const LinearRegressionModel* model,
    const double* X,
    double* y_pred,
    int n
);

/**
 * fp_linear_regression_r2_score - Compute R-squared coefficient
 *
 * @param y_true    Actual target values
 * @param y_pred    Predicted values
 * @param n         Number of samples
 *
 * @return R-squared score (1.0 = perfect fit, 0.0 = as bad as mean prediction)
 */
double fp_linear_regression_r2_score(
    const double* y_true,
    const double* y_pred,
    int n
);

/* ============================================================================
 * MEMORY MANAGEMENT
 * ============================================================================ */

/**
 * fp_linear_regression_free - Free internal arrays of LinearRegressionModel
 *
 * @param model     Pointer to model to free
 */
void fp_linear_regression_free(LinearRegressionModel* model);

/**
 * fp_gradient_descent_free - Free internal arrays of GradientDescentResult
 *
 * @param result    Pointer to result to free
 *
 * NOTE: Frees result->model.weights and result->loss_history
 */
void fp_gradient_descent_free(GradientDescentResult* result);

/* ============================================================================
 * PRINTING / VISUALIZATION
 * ============================================================================ */

/**
 * fp_linear_regression_print - Print model weights
 */
void fp_linear_regression_print(const LinearRegressionModel* model);

/**
 * fp_gradient_descent_print_summary - Print training summary
 */
void fp_gradient_descent_print_summary(const GradientDescentResult* result);

/* ============================================================================
 * SAFE API (Either Monad)
 * ============================================================================ */

/**
 * fp_linear_regression_gradient_descent_safe - Safe gradient descent with validation
 *
 * Returns Either monad for explicit error handling:
 * - Right(GradientDescentResult*) on success
 * - Left(error_msg, error_code) on failure
 *
 * Error codes:
 *   1 = NULL input data (X or y)
 *   2 = Invalid parameters (n<=0, d<=0, learning_rate<=0, max_iter<=0, threshold<0)
 *   3 = Memory allocation failed
 *
 * @param X             n x d feature matrix (row-major)
 * @param y             Target values (n elements)
 * @param n             Number of samples
 * @param d             Number of features
 * @param learning_rate Learning rate (must be > 0)
 * @param max_iterations Maximum iterations (must be > 0)
 * @param convergence_threshold Threshold for early stopping (must be >= 0)
 * @param seed          RNG seed for weight initialization
 *
 * @return Either containing Right(GradientDescentResult*) or Left(error_msg, code)
 *
 * MEMORY: On success, caller must:
 *   1. fp_gradient_descent_free(result) - Free internal arrays
 *   2. free(result) - Free the heap-allocated struct
 */
Either fp_linear_regression_gradient_descent_safe(
    const double* X,
    const double* y,
    int n,
    int d,
    double learning_rate,
    int max_iterations,
    double convergence_threshold,
    uint64_t seed
);

#endif /* FP_LINEAR_REGRESSION_H */
