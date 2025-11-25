/**
 * fp_pca.h - Principal Component Analysis (PCA) API
 *
 * Provides PCA with both standard and safe (Either monad) interfaces.
 * Uses L0 ASM primitives internally for SIMD-accelerated computation.
 *
 * Features:
 * - Power iteration eigenvalue decomposition
 * - Dimensionality reduction
 * - Transform/inverse transform operations
 * - Safe variant returning Either monad for error handling
 * - Variance analysis (explained variance ratio)
 *
 * Example (standard):
 *   PCAResult result = fp_pca_fit(X, n, d, k, 100, 1e-6, 42);
 *   fp_pca_transform(&result.model, X, X_pca, n);
 *   fp_pca_free_model(&result.model);
 *
 * Example (safe/monadic):
 *   Either result = fp_pca_fit_safe(X, n, d, k, 100, 1e-6, 42);
 *   if (fp_is_right(result)) {
 *       PCAResult* r = (PCAResult*)fp_from_right_ptr(result);
 *       // Use result...
 *       fp_pca_free_model(&r->model);  // Free internal arrays
 *       free(r);                       // Free the result struct itself
 *   } else {
 *       printf("Error: %s (code %d)\n",
 *              fp_from_left_msg(result), fp_from_left_code(result));
 *   }
 */

#ifndef FP_PCA_H
#define FP_PCA_H

#include <stdint.h>
#include <stddef.h>
#include "fp_monads.h"

/* ============================================================================
 * PCA STRUCTURES
 * ============================================================================ */

/**
 * PCAModel - Trained PCA model
 */
typedef struct {
    int n_features;                        // Original number of features (d)
    int n_components;                      // Number of principal components kept (k)

    double* mean;                          // Feature means (d × 1)
    double* components;                    // Principal components (k × d, row-major)
    double* eigenvalues;                   // Eigenvalues (k × 1, sorted descending)

    double total_variance;                 // Sum of all eigenvalues
    double* explained_variance_ratio;      // eigenvalue[i] / total_variance
    double* cumulative_variance_ratio;     // Cumulative sum of explained_variance_ratio
} PCAModel;

/**
 * PCAResult - Result of PCA training
 */
typedef struct {
    PCAModel model;
    int iterations_used;                   // Total power iterations
    int converged;                         // Whether eigenvalue extraction converged
} PCAResult;

/* ============================================================================
 * STANDARD PCA API
 * ============================================================================ */

/**
 * fp_pca_fit - Fit PCA model to data
 *
 * @param X               n x d data matrix (row-major)
 * @param n               Number of samples
 * @param d               Number of features
 * @param n_components    Number of principal components to extract (must be <= d)
 * @param max_iterations  Max iterations for power iteration
 * @param tolerance       Convergence threshold for eigenvalue extraction
 * @param seed            RNG seed for deterministic initialization
 *
 * @return PCAResult with trained model and convergence info
 *
 * NOTE: Does NOT validate inputs. For safe version with validation,
 *       use fp_pca_fit_safe().
 */
PCAResult fp_pca_fit(
    const double* X,
    int n,
    int d,
    int n_components,
    int max_iterations,
    double tolerance,
    uint64_t seed
);

/**
 * fp_pca_transform - Transform data to PCA space
 *
 * @param model           Trained PCA model
 * @param X               n x d input data (d = model->n_features)
 * @param X_transformed   Output in PCA space (n x k, must be pre-allocated)
 * @param n               Number of samples
 */
void fp_pca_transform(
    const PCAModel* model,
    const double* X,
    double* X_transformed,
    int n
);

/**
 * fp_pca_inverse_transform - Reconstruct data from PCA space
 *
 * @param model           Trained PCA model
 * @param X_pca           n x k data in PCA space (k = model->n_components)
 * @param X_reconstructed Output in original space (n x d, must be pre-allocated)
 * @param n               Number of samples
 */
void fp_pca_inverse_transform(
    const PCAModel* model,
    const double* X_pca,
    double* X_reconstructed,
    int n
);

/* ============================================================================
 * MEMORY MANAGEMENT
 * ============================================================================ */

/**
 * fp_pca_free_model - Free internal arrays of PCAModel
 *
 * @param model           Pointer to model to free
 */
void fp_pca_free_model(PCAModel* model);

/* ============================================================================
 * DATA GENERATION (for testing/demos)
 * ============================================================================ */

/**
 * fp_pca_generate_ellipse_data - Generate 2D ellipse data for PCA demo
 *
 * @param X               Output buffer (n x 2, must be pre-allocated)
 * @param n               Number of samples
 * @param major_axis      Length of major axis
 * @param minor_axis      Length of minor axis
 * @param angle           Rotation angle in radians
 * @param seed            RNG seed
 */
void fp_pca_generate_ellipse_data(
    double* X,
    int n,
    double major_axis,
    double minor_axis,
    double angle,
    uint64_t seed
);

/**
 * fp_pca_generate_low_rank_data - Generate high-dimensional low-rank data
 *
 * @param X               Output buffer (n x d, must be pre-allocated)
 * @param n               Number of samples
 * @param d               Number of features (high-dimensional)
 * @param intrinsic_dim   True intrinsic dimensionality (k << d)
 * @param noise_stddev    Standard deviation of Gaussian noise
 * @param seed            RNG seed
 */
void fp_pca_generate_low_rank_data(
    double* X,
    int n,
    int d,
    int intrinsic_dim,
    double noise_stddev,
    uint64_t seed
);

/* ============================================================================
 * SAFE API (Either Monad)
 * ============================================================================ */

/**
 * fp_pca_fit_safe - Safe PCA fit with validation
 *
 * Returns Either monad for explicit error handling:
 * - Right(PCAResult*) on success
 * - Left(error_msg, error_code) on failure
 *
 * Error codes:
 *   1 = NULL input data (X)
 *   2 = Invalid parameters (n<=0, d<=0, n_components<=0, n_components>d,
 *       max_iterations<=0, tolerance<0, or overflow in d*d, n*d, n_components*d)
 *   3 = Memory allocation failed
 *
 * Overflow protection:
 *   - Checks d * d < INT_MAX (covariance matrix)
 *   - Checks n * d < INT_MAX (data matrix)
 *   - Checks n_components * d < INT_MAX (components matrix)
 *
 * @param X               n x d data matrix (row-major)
 * @param n               Number of samples
 * @param d               Number of features
 * @param n_components    Number of principal components to extract (must be <= d)
 * @param max_iterations  Max iterations for power iteration (must be > 0)
 * @param tolerance       Convergence threshold (must be >= 0)
 * @param seed            RNG seed for deterministic initialization
 *
 * @return Either containing Right(PCAResult*) or Left(error_msg, code)
 *
 * MEMORY: On success, caller must:
 *   1. fp_pca_free_model(&result->model) - Free internal arrays
 *   2. free(result) - Free the heap-allocated struct
 */
Either fp_pca_fit_safe(
    const double* X,
    int n,
    int d,
    int n_components,
    int max_iterations,
    double tolerance,
    uint64_t seed
);

#endif /* FP_PCA_H */
