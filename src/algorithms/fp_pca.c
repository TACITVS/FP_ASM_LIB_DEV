// fp_pca.c
//
// Principal Component Analysis (PCA)
// Demonstrates dimensionality reduction and eigenvalue decomposition
//
// This showcases:
// - Covariance matrix computation
// - Eigenvalue decomposition (power iteration + deflation)
// - Dimensionality reduction
// - Variance explanation analysis
// - Data projection and reconstruction
//
// FP Primitives Used:
// - Matrix operations (multiply, transpose)
// - Statistical computations (mean, variance, covariance)
// - Iterative algorithms (power method)
//
// Applications:
// - Feature extraction (compress 1000 features → 10)
// - Data visualization (3D → 2D projection)
// - Noise reduction (keep high-variance components)
// - Image compression (eigenfaces)
// - Genomics (gene expression analysis)

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

// PCA Model: stores principal components and statistics
typedef struct {
    int n_features;          // Original number of features (d)
    int n_components;        // Number of principal components kept (k)

    double* mean;            // Feature means (d × 1)
    double* components;      // Principal components (k × d, row-major)
    double* eigenvalues;     // Eigenvalues (k × 1, sorted descending)

    double total_variance;   // Sum of all eigenvalues
    double* explained_variance_ratio;  // eigenvalue[i] / total_variance
    double* cumulative_variance_ratio; // Cumulative sum of explained_variance_ratio
} PCAModel;

// PCA Training Result
typedef struct {
    PCAModel model;
    int iterations_used;     // Total power iterations
    int converged;           // Whether eigenvalue extraction converged
} PCAResult;

// ============================================================================
// Matrix Utilities
// ============================================================================

// Matrix-vector multiply: y = A * x
// A: m × n (row-major), x: n × 1, y: m × 1
static void matrix_vector_multiply(
    const double* A, const double* x, double* y,
    int m, int n
) {
    for (int i = 0; i < m; i++) {
        y[i] = 0.0;
        for (int j = 0; j < n; j++) {
            y[i] += A[i * n + j] * x[j];
        }
    }
}

// Vector dot product: result = x · y
static double vector_dot(const double* x, const double* y, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += x[i] * y[i];
    }
    return sum;
}

// Vector norm: ||x||₂
static double vector_norm(const double* x, int n) {
    return sqrt(vector_dot(x, x, n));
}

// Vector normalize: x = x / ||x||
static void vector_normalize(double* x, int n) {
    double norm = vector_norm(x, n);
    if (norm > 1e-10) {
        for (int i = 0; i < n; i++) {
            x[i] /= norm;
        }
    }
}

// Vector scale: y = alpha * x
static void vector_scale(const double* x, double alpha, double* y, int n) {
    for (int i = 0; i < n; i++) {
        y[i] = alpha * x[i];
    }
}

// Vector subtract: z = x - y
static void vector_subtract(const double* x, const double* y, double* z, int n) {
    for (int i = 0; i < n; i++) {
        z[i] = x[i] - y[i];
    }
}

// ============================================================================
// Statistical Computations
// ============================================================================

// Compute feature means: mean[j] = (1/n) * Σ X[i,j]
static void compute_means(const double* X, int n, int d, double* mean) {
    for (int j = 0; j < d; j++) {
        mean[j] = 0.0;
        for (int i = 0; i < n; i++) {
            mean[j] += X[i * d + j];
        }
        mean[j] /= n;
    }
}

// Center data: X_centered = X - mean (in-place)
static void center_data(double* X, int n, int d, const double* mean) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < d; j++) {
            X[i * d + j] -= mean[j];
        }
    }
}

// Compute covariance matrix: C = (1/n) * X^T * X
// X: n × d (centered data), C: d × d (symmetric)
static void compute_covariance_matrix(
    const double* X, int n, int d, double* C
) {
    // C[i,j] = (1/n) * Σ(k=0..n-1) X[k,i] * X[k,j]
    for (int i = 0; i < d; i++) {
        for (int j = 0; j < d; j++) {
            C[i * d + j] = 0.0;
            for (int k = 0; k < n; k++) {
                C[i * d + j] += X[k * d + i] * X[k * d + j];
            }
            C[i * d + j] /= n;
        }
    }
}

// ============================================================================
// Eigenvalue Decomposition (Power Iteration Method)
// ============================================================================

// Power iteration: find dominant eigenvector/eigenvalue of symmetric matrix A
// Returns eigenvalue, stores eigenvector in v (must be pre-initialized)
static double power_iteration(
    const double* A,        // d × d symmetric matrix
    double* v,              // d × 1 eigenvector (in/out)
    int d,                  // Dimension
    int max_iterations,     // Maximum iterations
    double tolerance        // Convergence threshold
) {
    double* Av = (double*)malloc(d * sizeof(double));
    double eigenvalue = 0.0;
    double prev_eigenvalue = 0.0;

    // Initialize v to random unit vector if all zeros
    double v_norm = vector_norm(v, d);
    if (v_norm < 1e-10) {
        for (int i = 0; i < d; i++) {
            v[i] = (double)rand() / RAND_MAX;
        }
        vector_normalize(v, d);
    }

    for (int iter = 0; iter < max_iterations; iter++) {
        // v_new = A * v
        matrix_vector_multiply(A, v, Av, d, d);

        // eigenvalue = v^T * A * v
        eigenvalue = vector_dot(v, Av, d);

        // Normalize v_new
        vector_normalize(Av, d);
        memcpy(v, Av, d * sizeof(double));

        // Check convergence
        if (iter > 0 && fabs(eigenvalue - prev_eigenvalue) < tolerance) {
            break;
        }
        prev_eigenvalue = eigenvalue;
    }

    free(Av);
    return eigenvalue;
}

// Deflation: A_new = A - λ * v * v^T
// Removes the found eigenpair from matrix
static void deflate_matrix(
    double* A,              // d × d matrix (in/out)
    const double* v,        // Eigenvector to remove
    double eigenvalue,      // Corresponding eigenvalue
    int d                   // Dimension
) {
    for (int i = 0; i < d; i++) {
        for (int j = 0; j < d; j++) {
            A[i * d + j] -= eigenvalue * v[i] * v[j];
        }
    }
}

// Extract top k eigenvectors/eigenvalues using power iteration + deflation
static int extract_eigenpairs(
    const double* C_original,   // d × d covariance matrix (input, not modified)
    double* eigenvectors,       // k × d output (row-major)
    double* eigenvalues,        // k × 1 output
    int d,                      // Dimension
    int k,                      // Number of components to extract
    int max_iterations,         // Max iterations per eigenpair
    double tolerance            // Convergence threshold
) {
    // Work on a copy of covariance matrix (will be modified by deflation)
    double* C = (double*)malloc(d * d * sizeof(double));
    memcpy(C, C_original, d * d * sizeof(double));

    double* v = (double*)malloc(d * sizeof(double));
    int total_iterations = 0;

    for (int i = 0; i < k; i++) {
        // Initialize eigenvector guess
        memset(v, 0, d * sizeof(double));

        // Find dominant eigenpair of current matrix
        eigenvalues[i] = power_iteration(C, v, d, max_iterations, tolerance);

        // Store eigenvector (as row in eigenvectors matrix)
        memcpy(&eigenvectors[i * d], v, d * sizeof(double));

        // Deflate matrix to find next eigenpair
        deflate_matrix(C, v, eigenvalues[i], d);

        total_iterations += max_iterations;  // Approximate
    }

    free(C);
    free(v);
    return total_iterations;
}

// ============================================================================
// PCA Core Functions
// ============================================================================

// Train PCA model
PCAResult fp_pca_fit(
    const double* X,            // n × d data matrix (row-major)
    int n,                      // Number of samples
    int d,                      // Number of features
    int n_components,           // Number of principal components to keep
    int max_iterations,         // Max iterations for eigenvalue extraction
    double tolerance            // Convergence threshold
) {
    PCAResult result;
    result.converged = 1;

    // Allocate model
    result.model.n_features = d;
    result.model.n_components = n_components;
    result.model.mean = (double*)malloc(d * sizeof(double));
    result.model.components = (double*)malloc(n_components * d * sizeof(double));
    result.model.eigenvalues = (double*)malloc(n_components * sizeof(double));
    result.model.explained_variance_ratio = (double*)malloc(n_components * sizeof(double));
    result.model.cumulative_variance_ratio = (double*)malloc(n_components * sizeof(double));

    // Step 1: Compute mean and center data
    compute_means(X, n, d, result.model.mean);

    // Create centered copy of X
    double* X_centered = (double*)malloc(n * d * sizeof(double));
    memcpy(X_centered, X, n * d * sizeof(double));
    center_data(X_centered, n, d, result.model.mean);

    // Step 2: Compute covariance matrix C = (1/n) * X^T * X
    double* C = (double*)malloc(d * d * sizeof(double));
    compute_covariance_matrix(X_centered, n, d, C);

    // Step 3: Extract top k eigenvectors/eigenvalues
    result.iterations_used = extract_eigenpairs(
        C,
        result.model.components,
        result.model.eigenvalues,
        d,
        n_components,
        max_iterations,
        tolerance
    );

    // Step 4: Compute variance ratios
    result.model.total_variance = 0.0;
    for (int i = 0; i < n_components; i++) {
        result.model.total_variance += result.model.eigenvalues[i];
    }

    double cumsum = 0.0;
    for (int i = 0; i < n_components; i++) {
        result.model.explained_variance_ratio[i] =
            result.model.eigenvalues[i] / result.model.total_variance;
        cumsum += result.model.explained_variance_ratio[i];
        result.model.cumulative_variance_ratio[i] = cumsum;
    }

    free(X_centered);
    free(C);

    return result;
}

// Transform data to PCA space: X_transformed = (X - mean) * components^T
// X: n × d, X_transformed: n × k
void fp_pca_transform(
    const PCAModel* model,
    const double* X,            // n × d input data
    double* X_transformed,      // n × k output (PCA space)
    int n                       // Number of samples
) {
    int d = model->n_features;
    int k = model->n_components;

    // For each sample
    for (int i = 0; i < n; i++) {
        // Center the sample
        double* x_centered = (double*)malloc(d * sizeof(double));
        for (int j = 0; j < d; j++) {
            x_centered[j] = X[i * d + j] - model->mean[j];
        }

        // Project onto each principal component
        for (int c = 0; c < k; c++) {
            X_transformed[i * k + c] = vector_dot(
                x_centered,
                &model->components[c * d],
                d
            );
        }

        free(x_centered);
    }
}

// Inverse transform: reconstruct original space from PCA space
// X_pca: n × k, X_reconstructed: n × d
void fp_pca_inverse_transform(
    const PCAModel* model,
    const double* X_pca,        // n × k PCA space
    double* X_reconstructed,    // n × d output
    int n                       // Number of samples
) {
    int d = model->n_features;
    int k = model->n_components;

    for (int i = 0; i < n; i++) {
        // X_reconstructed[i] = mean + Σ(c=0..k-1) X_pca[i,c] * component[c]
        for (int j = 0; j < d; j++) {
            X_reconstructed[i * d + j] = model->mean[j];
            for (int c = 0; c < k; c++) {
                X_reconstructed[i * d + j] +=
                    X_pca[i * k + c] * model->components[c * d + j];
            }
        }
    }
}

// Compute reconstruction error: mean squared error between X and reconstructed X
double fp_pca_reconstruction_error(
    const PCAModel* model,
    const double* X,            // n × d original data
    int n                       // Number of samples
) {
    int d = model->n_features;
    int k = model->n_components;

    // Transform to PCA space
    double* X_pca = (double*)malloc(n * k * sizeof(double));
    fp_pca_transform(model, X, X_pca, n);

    // Reconstruct
    double* X_reconstructed = (double*)malloc(n * d * sizeof(double));
    fp_pca_inverse_transform(model, X_pca, X_reconstructed, n);

    // Compute MSE
    double mse = 0.0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < d; j++) {
            double diff = X[i * d + j] - X_reconstructed[i * d + j];
            mse += diff * diff;
        }
    }
    mse /= (n * d);

    free(X_pca);
    free(X_reconstructed);

    return mse;
}

// ============================================================================
// Data Generation (for testing)
// ============================================================================

// Generate correlated 2D data (ellipse)
void fp_pca_generate_ellipse_data(
    double* X,                  // n × 2 output
    int n,                      // Number of samples
    double major_axis,          // Length of major axis
    double minor_axis,          // Length of minor axis
    double angle,               // Rotation angle (radians)
    double noise_level,         // Gaussian noise
    unsigned int seed
) {
    srand(seed);

    double cos_a = cos(angle);
    double sin_a = sin(angle);

    for (int i = 0; i < n; i++) {
        // Generate point on axis-aligned ellipse
        double t = 2.0 * M_PI * i / n;
        double x = major_axis * cos(t);
        double y = minor_axis * sin(t);

        // Add noise
        double noise_x = noise_level * (2.0 * (double)rand() / RAND_MAX - 1.0);
        double noise_y = noise_level * (2.0 * (double)rand() / RAND_MAX - 1.0);
        x += noise_x;
        y += noise_y;

        // Rotate
        X[i * 2 + 0] = x * cos_a - y * sin_a;
        X[i * 2 + 1] = x * sin_a + y * cos_a;
    }
}

// Generate high-dimensional data with intrinsic low rank
void fp_pca_generate_low_rank_data(
    double* X,                  // n × d output
    int n,                      // Number of samples
    int d,                      // Number of features (high-dimensional)
    int intrinsic_dim,          // True dimensionality (< d)
    double noise_level,         // Noise to add
    unsigned int seed
) {
    srand(seed);

    // Generate random low-rank basis
    double* basis = (double*)malloc(intrinsic_dim * d * sizeof(double));
    for (int i = 0; i < intrinsic_dim * d; i++) {
        basis[i] = 2.0 * (double)rand() / RAND_MAX - 1.0;
    }

    // Generate samples in low-dimensional space
    for (int i = 0; i < n; i++) {
        // Random coefficients
        double* coeffs = (double*)malloc(intrinsic_dim * sizeof(double));
        for (int j = 0; j < intrinsic_dim; j++) {
            coeffs[j] = 2.0 * (double)rand() / RAND_MAX - 1.0;
        }

        // Project to high-dimensional space: X[i] = basis^T * coeffs
        for (int k = 0; k < d; k++) {
            X[i * d + k] = 0.0;
            for (int j = 0; j < intrinsic_dim; j++) {
                X[i * d + k] += basis[j * d + k] * coeffs[j];
            }
            // Add noise
            X[i * d + k] += noise_level * (2.0 * (double)rand() / RAND_MAX - 1.0);
        }

        free(coeffs);
    }

    free(basis);
}

// ============================================================================
// Printing and Visualization
// ============================================================================

void fp_pca_print_model_summary(const PCAModel* model) {
    printf("PCA Model Summary:\n");
    printf("  Original dimensions: %d\n", model->n_features);
    printf("  Reduced dimensions: %d\n", model->n_components);
    printf("  Total variance: %.6f\n\n", model->total_variance);

    printf("Principal Components:\n");
    printf("  PC | Eigenvalue | Var Explained | Cumulative\n");
    printf("  ---|------------|---------------|------------\n");
    for (int i = 0; i < model->n_components; i++) {
        printf("  %2d | %10.6f | %12.2f%% | %10.2f%%\n",
               i + 1,
               model->eigenvalues[i],
               model->explained_variance_ratio[i] * 100.0,
               model->cumulative_variance_ratio[i] * 100.0);
    }
    printf("\n");
}

void fp_pca_print_components(const PCAModel* model, int max_features) {
    int d = model->n_features;
    int k = model->n_components;
    int show = (max_features < d) ? max_features : d;

    printf("Principal Component Loadings (first %d features):\n", show);
    for (int c = 0; c < k; c++) {
        printf("  PC%d: [", c + 1);
        for (int i = 0; i < show; i++) {
            printf("%7.3f", model->components[c * d + i]);
            if (i < show - 1) printf(", ");
        }
        if (show < d) printf(", ...");
        printf("]\n");
    }
    printf("\n");
}

// Free PCA model
void fp_pca_free_model(PCAModel* model) {
    free(model->mean);
    free(model->components);
    free(model->eigenvalues);
    free(model->explained_variance_ratio);
    free(model->cumulative_variance_ratio);
}
