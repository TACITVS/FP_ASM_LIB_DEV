// demo_pca.c - Principal Component Analysis Demo
// Demonstrates dimensionality reduction and eigenvalue decomposition

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declarations
typedef struct {
    int n_features;
    int n_components;
    double* mean;
    double* components;
    double* eigenvalues;
    double total_variance;
    double* explained_variance_ratio;
    double* cumulative_variance_ratio;
} PCAModel;

typedef struct {
    PCAModel model;
    int iterations_used;
    int converged;
} PCAResult;

PCAResult fp_pca_fit(const double* X, int n, int d, int n_components, int max_iterations, double tolerance);
void fp_pca_transform(const PCAModel* model, const double* X, double* X_transformed, int n);
void fp_pca_inverse_transform(const PCAModel* model, const double* X_pca, double* X_reconstructed, int n);
double fp_pca_reconstruction_error(const PCAModel* model, const double* X, int n);
void fp_pca_generate_ellipse_data(double* X, int n, double major, double minor, double angle, double noise, unsigned int seed);
void fp_pca_generate_low_rank_data(double* X, int n, int d, int intrinsic_dim, double noise, unsigned int seed);
void fp_pca_print_model_summary(const PCAModel* model);
void fp_pca_print_components(const PCAModel* model, int max_features);
void fp_pca_free_model(PCAModel* model);

// ============================================================================
// TEST 1: 2D Ellipse - Finding Principal Axes
// ============================================================================
void test_2d_ellipse() {
    printf("================================================================\n");
    printf("TEST 1: 2D Ellipse - Finding Principal Axes\n");
    printf("================================================================\n\n");

    int n = 200;
    double major_axis = 3.0;
    double minor_axis = 1.0;
    double rotation_angle = M_PI / 4;  // 45 degrees

    printf("Dataset: Rotated ellipse\n");
    printf("  Samples: %d\n", n);
    printf("  Major axis: %.1f\n", major_axis);
    printf("  Minor axis: %.1f\n", minor_axis);
    printf("  Rotation: %.1f degrees\n", rotation_angle * 180.0 / M_PI);
    printf("  Noise level: 0.1\n\n");

    // Generate data
    double* X = (double*)malloc(n * 2 * sizeof(double));
    fp_pca_generate_ellipse_data(X, n, major_axis, minor_axis, rotation_angle, 0.1, 42);

    // Fit PCA
    printf("Fitting PCA (2 components)...\n\n");
    PCAResult result = fp_pca_fit(X, n, 2, 2, 1000, 1e-6);

    // Print results
    fp_pca_print_model_summary(&result.model);

    printf("Principal Components (should align with ellipse axes):\n");
    printf("  PC1: [%.4f, %.4f]\n",
           result.model.components[0], result.model.components[1]);
    printf("  PC2: [%.4f, %.4f]\n",
           result.model.components[2], result.model.components[3]);

    // Compare with expected rotation
    double expected_pc1_x = cos(rotation_angle);
    double expected_pc1_y = sin(rotation_angle);
    printf("\nExpected PC1 (along major axis): [%.4f, %.4f]\n",
           expected_pc1_x, expected_pc1_y);

    // Check alignment (allow for sign flip)
    double dot_product = result.model.components[0] * expected_pc1_x +
                        result.model.components[1] * expected_pc1_y;
    double alignment = fabs(dot_product);
    printf("Alignment with expected: %.4f (1.0 = perfect)\n", alignment);

    if (alignment > 0.95) {
        printf("Status: PASS - PCA correctly found principal axes!\n\n");
    } else {
        printf("Status: FAIL - Poor alignment\n\n");
    }

    fp_pca_free_model(&result.model);
    free(X);
}

// ============================================================================
// TEST 2: Dimensionality Reduction (100D → 5D)
// ============================================================================
void test_dimensionality_reduction() {
    printf("================================================================\n");
    printf("TEST 2: Dimensionality Reduction (100D → 5D)\n");
    printf("================================================================\n\n");

    int n = 500;          // Samples
    int d = 100;          // Original dimensions
    int intrinsic = 5;    // True underlying dimensionality
    int k = 10;           // Components to extract

    printf("Dataset: High-dimensional with low intrinsic rank\n");
    printf("  Samples: %d\n", n);
    printf("  Original dimensions: %d\n", d);
    printf("  True intrinsic dimensions: %d\n", intrinsic);
    printf("  Components to extract: %d\n", k);
    printf("  Noise level: 0.1\n\n");

    // Generate low-rank data
    double* X = (double*)malloc(n * d * sizeof(double));
    fp_pca_generate_low_rank_data(X, n, d, intrinsic, 0.1, 42);

    printf("Fitting PCA...\n\n");
    clock_t start = clock();
    PCAResult result = fp_pca_fit(X, n, d, k, 1000, 1e-6);
    clock_t end = clock();
    double time_ms = 1000.0 * (end - start) / CLOCKS_PER_SEC;

    printf("Training completed in %.2f ms\n", time_ms);
    printf("Iterations used: %d\n\n", result.iterations_used);

    fp_pca_print_model_summary(&result.model);

    // Compute reconstruction error
    printf("Testing reconstruction quality...\n");
    double mse = fp_pca_reconstruction_error(&result.model, X, n);
    printf("  Mean Squared Error: %.6e\n", mse);

    // Expected: First 'intrinsic' components should explain most variance
    printf("\nVariance Analysis:\n");
    printf("  First %d components explain: %.2f%%\n",
           intrinsic,
           result.model.cumulative_variance_ratio[intrinsic - 1] * 100.0);
    printf("  First %d components explain: %.2f%%\n",
           k,
           result.model.cumulative_variance_ratio[k - 1] * 100.0);

    if (result.model.cumulative_variance_ratio[intrinsic - 1] > 0.90) {
        printf("\nStatus: PASS - First %d components capture >90%% variance!\n", intrinsic);
        printf("This confirms the data has intrinsic dimensionality ~%d\n\n", intrinsic);
    } else {
        printf("\nStatus: Variance not concentrated as expected\n\n");
    }

    fp_pca_free_model(&result.model);
    free(X);
}

// ============================================================================
// TEST 3: Compression and Reconstruction
// ============================================================================
void test_compression() {
    printf("================================================================\n");
    printf("TEST 3: Data Compression via PCA\n");
    printf("================================================================\n\n");

    int n = 1000;
    int d = 50;
    int intrinsic = 3;

    printf("Compression Challenge:\n");
    printf("  Original data: %d samples × %d features = %d values\n",
           n, d, n * d);
    printf("  Intrinsic dimensionality: %d\n\n", intrinsic);

    // Generate data
    double* X = (double*)malloc(n * d * sizeof(double));
    fp_pca_generate_low_rank_data(X, n, d, intrinsic, 0.05, 42);

    printf("Testing different compression levels:\n");
    printf("  k  | Variance | Compression | Reconstruction Error\n");
    printf("  ---|----------|-------------|---------------------\n");

    int k_values[] = {1, 2, 3, 5, 10, 20};
    int n_k = sizeof(k_values) / sizeof(int);

    for (int i = 0; i < n_k; i++) {
        int k = k_values[i];

        PCAResult result = fp_pca_fit(X, n, d, k, 1000, 1e-6);

        // Compression ratio
        double original_size = n * d;
        double compressed_size = n * k + k * d + d;  // transformed + components + mean
        double compression = original_size / compressed_size;

        // Reconstruction error
        double mse = fp_pca_reconstruction_error(&result.model, X, n);

        printf("  %2d | %7.2f%% | %10.1fx | %.6e\n",
               k,
               result.model.cumulative_variance_ratio[k - 1] * 100.0,
               compression,
               mse);

        fp_pca_free_model(&result.model);
    }

    printf("\nKey Insight: With k=%d components:\n", intrinsic);
    PCAResult best = fp_pca_fit(X, n, d, intrinsic, 1000, 1e-6);
    double best_compression = (n * d) / (double)(n * intrinsic + intrinsic * d + d);
    double best_variance = best.model.cumulative_variance_ratio[intrinsic - 1];

    printf("  - %.1fx compression\n", best_compression);
    printf("  - %.2f%% variance preserved\n", best_variance * 100.0);
    printf("  - Lossy but extremely efficient!\n\n");

    fp_pca_free_model(&best.model);
    free(X);
}

// ============================================================================
// TEST 4: Scalability Test
// ============================================================================
void test_scalability() {
    printf("================================================================\n");
    printf("TEST 4: Scalability Analysis\n");
    printf("================================================================\n\n");

    printf("Testing PCA performance on various dataset sizes:\n");
    printf("  n     | d   | k  | Time (ms) | Throughput\n");
    printf("  ------|-----|----|-----------|-----------\n");

    int configs[][3] = {
        {100, 10, 3},
        {500, 20, 5},
        {1000, 50, 10},
        {2000, 100, 20}
    };
    int n_configs = sizeof(configs) / sizeof(configs[0]);

    for (int i = 0; i < n_configs; i++) {
        int n = configs[i][0];
        int d = configs[i][1];
        int k = configs[i][2];

        double* X = (double*)malloc(n * d * sizeof(double));
        fp_pca_generate_low_rank_data(X, n, d, k, 0.1, 42);

        clock_t start = clock();
        PCAResult result = fp_pca_fit(X, n, d, k, 500, 1e-6);
        clock_t end = clock();

        double time_ms = 1000.0 * (end - start) / CLOCKS_PER_SEC;
        double throughput = (n * d) / (time_ms / 1000.0) / 1e6;  // Million elements/sec

        printf("  %5d | %3d | %2d | %9.2f | %.2f M elem/s\n",
               n, d, k, time_ms, throughput);

        fp_pca_free_model(&result.model);
        free(X);
    }

    printf("\nNote: Complexity is dominated by covariance matrix computation\n");
    printf("and eigenvalue extraction (power iteration × k components)\n\n");
}

// ============================================================================
// Main
// ============================================================================
int main() {
    printf("================================================================\n");
    printf("  Principal Component Analysis (PCA) Demo\n");
    printf("  FP-ASM Library - Dimensionality Reduction\n");
    printf("================================================================\n\n");

    printf("PCA Applications Demonstrated:\n");
    printf("  1. Finding Principal Axes - Geometric interpretation\n");
    printf("  2. Dimensionality Reduction - 100D → 5D compression\n");
    printf("  3. Data Compression - Lossy but efficient\n");
    printf("  4. Scalability - Performance on various sizes\n\n");

    printf("Algorithm: Covariance Method + Power Iteration\n");
    printf("  - Compute covariance matrix C = (1/n)·X^T·X\n");
    printf("  - Extract eigenpairs via power iteration\n");
    printf("  - Use deflation to find multiple components\n");
    printf("  - Project data onto principal components\n\n");

    test_2d_ellipse();
    test_dimensionality_reduction();
    test_compression();
    test_scalability();

    printf("================================================================\n");
    printf("  PCA Demo Complete!\n");
    printf("================================================================\n\n");

    printf("Key Takeaways:\n");
    printf("  ✓ PCA finds directions of maximum variance\n");
    printf("  ✓ Reduces dimensionality while preserving information\n");
    printf("  ✓ Enables data compression (lossy but efficient)\n");
    printf("  ✓ Foundation of many ML preprocessing pipelines\n");
    printf("  ✓ Interpretable: principal components have geometric meaning\n\n");

    printf("Real-World Applications:\n");
    printf("  - Face Recognition: Eigenfaces (compress face images)\n");
    printf("  - Genomics: Analyze gene expression (1000s of genes → 3 PCs)\n");
    printf("  - Image Compression: Reduce storage while preserving quality\n");
    printf("  - Feature Engineering: Remove correlated features\n");
    printf("  - Data Visualization: Project high-D data to 2D/3D for plotting\n");
    printf("  - Noise Reduction: Keep high-variance components, discard noise\n\n");

    printf("Why PCA is Fundamental:\n");
    printf("  - Unsupervised: No labels needed\n");
    printf("  - Optimal: Minimizes reconstruction error (L2 sense)\n");
    printf("  - Fast: O(d²n + d³) for eigenvalue decomposition\n");
    printf("  - Interpretable: Components have clear geometric meaning\n");
    printf("  - Versatile: Preprocessing for classification, regression, clustering\n\n");

    printf("Mathematical Foundation:\n");
    printf("  - Covariance Matrix: C = (1/n)·X^T·X (captures correlations)\n");
    printf("  - Eigendecomposition: C·v = λ·v (principal directions)\n");
    printf("  - Variance Maximization: PC1 has max variance, PC2 orthogonal to PC1, etc.\n");
    printf("  - Reconstruction: X ≈ X̄ + Σ(score[i]·component[i])\n\n");

    return 0;
}
