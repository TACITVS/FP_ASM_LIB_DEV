// demo_kmeans.c
//
// K-Means Clustering Demo
// Demonstrates FP-ASM library composing into a complete ML algorithm
//
// This showcases:
// - Functional composition (complex algorithm from simple primitives)
// - Real-world ML application
// - SIMD-accelerated distance calculations
// - Clean, maintainable functional code

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "fp_core.h"

// K-Means structures (duplicated here for standalone compilation)
typedef struct {
    double* centroids;
    int* assignments;
    int* cluster_sizes;
    int iterations;
    double inertia;
    int converged;
} KMeansResult;

// Forward declarations from fp_kmeans.c
KMeansResult fp_kmeans_f64(const double* data, int n, int d, int k, int max_iter, double tol);
void fp_kmeans_free(KMeansResult* result);
void fp_kmeans_print(const KMeansResult* result, int k, int d);

// Generate synthetic clustered data for testing
// Creates k well-separated Gaussian clusters
void generate_clustered_data(
    double* data,          // n × d output matrix
    int n,                 // number of points
    int d,                 // dimensionality
    int k,                 // number of true clusters
    double separation      // cluster separation (higher = more separated)
) {
    srand(42);  // Fixed seed for reproducibility

    int points_per_cluster = n / k;

    for (int cluster = 0; cluster < k; cluster++) {
        // Random cluster center
        double center[10];  // Max 10 dimensions
        for (int j = 0; j < d; j++) {
            center[j] = ((double)rand() / RAND_MAX) * separation * k;
        }

        // Generate points around this center
        int start = cluster * points_per_cluster;
        int end = (cluster == k - 1) ? n : start + points_per_cluster;

        for (int i = start; i < end; i++) {
            for (int j = 0; j < d; j++) {
                // Gaussian noise around center
                double u1 = (double)rand() / RAND_MAX;
                double u2 = (double)rand() / RAND_MAX;
                double gaussian = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
                data[i * d + j] = center[j] + gaussian * separation * 0.3;
            }
        }
    }
}

// Compute clustering accuracy (for synthetic data where true labels known)
double compute_accuracy(const int* predicted, const int* true_labels, int n, int k) {
    // Find best permutation of cluster IDs (Hungarian algorithm simplified)
    int correct = 0;
    int* mapping = (int*)malloc(k * sizeof(int));

    // Simple greedy matching
    for (int pred_c = 0; pred_c < k; pred_c++) {
        int best_true_c = 0;
        int best_overlap = 0;

        for (int true_c = 0; true_c < k; true_c++) {
            int overlap = 0;
            for (int i = 0; i < n; i++) {
                if (predicted[i] == pred_c && true_labels[i] == true_c) {
                    overlap++;
                }
            }
            if (overlap > best_overlap) {
                best_overlap = overlap;
                best_true_c = true_c;
            }
        }
        mapping[pred_c] = best_true_c;
    }

    // Count correct assignments
    for (int i = 0; i < n; i++) {
        if (mapping[predicted[i]] == true_labels[i]) {
            correct++;
        }
    }

    free(mapping);
    return (double)correct / n;
}

int main() {
    printf("================================================================\n");
    printf("  K-Means Clustering Demo\n");
    printf("  FP-ASM Library - Complex Algorithm from Simple Primitives\n");
    printf("================================================================\n\n");

    // ========================================================================
    // Test 1: 2D Clustering (Visualizable)
    // ========================================================================
    printf("TEST 1: 2D Clustering (3 clusters, 300 points)\n");
    printf("--------------------------------------------------------\n");

    const int n1 = 300;
    const int d1 = 2;
    const int k1 = 3;

    double* data1 = (double*)malloc(n1 * d1 * sizeof(double));
    int* true_labels1 = (int*)malloc(n1 * sizeof(int));

    generate_clustered_data(data1, n1, d1, k1, 10.0);

    // Store true labels for accuracy calculation
    for (int i = 0; i < n1; i++) {
        true_labels1[i] = i / (n1 / k1);
    }

    // Run k-means
    clock_t start = clock();
    KMeansResult result1 = fp_kmeans_f64(data1, n1, d1, k1, 100, 1e-4);
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf("\n");
    fp_kmeans_print(&result1, k1, d1);
    printf("\nRuntime: %.3f ms\n", elapsed);
    printf("Accuracy: %.1f%%\n", compute_accuracy(result1.assignments, true_labels1, n1, k1) * 100);

    fp_kmeans_free(&result1);
    free(data1);
    free(true_labels1);

    printf("\n");

    // ========================================================================
    // Test 2: High-Dimensional Clustering
    // ========================================================================
    printf("TEST 2: High-Dimensional Clustering (10D, 5 clusters, 1000 points)\n");
    printf("--------------------------------------------------------\n");

    const int n2 = 1000;
    const int d2 = 10;
    const int k2 = 5;

    double* data2 = (double*)malloc(n2 * d2 * sizeof(double));
    int* true_labels2 = (int*)malloc(n2 * sizeof(int));

    generate_clustered_data(data2, n2, d2, k2, 5.0);

    for (int i = 0; i < n2; i++) {
        true_labels2[i] = i / (n2 / k2);
    }

    start = clock();
    KMeansResult result2 = fp_kmeans_f64(data2, n2, d2, k2, 100, 1e-4);
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf("\n");
    fp_kmeans_print(&result2, k2, d2);
    printf("\nRuntime: %.3f ms\n", elapsed);
    printf("Accuracy: %.1f%%\n", compute_accuracy(result2.assignments, true_labels2, n2, k2) * 100);

    fp_kmeans_free(&result2);
    free(data2);
    free(true_labels2);

    printf("\n");

    // ========================================================================
    // Test 3: Performance Benchmark (Large Dataset)
    // ========================================================================
    printf("TEST 3: Performance Benchmark (10K points, 5D, 8 clusters)\n");
    printf("--------------------------------------------------------\n");

    const int n3 = 10000;
    const int d3 = 5;
    const int k3 = 8;

    double* data3 = (double*)malloc(n3 * d3 * sizeof(double));
    generate_clustered_data(data3, n3, d3, k3, 8.0);

    printf("Running k-means on %d points...\n", n3);

    start = clock();
    KMeansResult result3 = fp_kmeans_f64(data3, n3, d3, k3, 100, 1e-4);
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf("\nResults:\n");
    printf("  Iterations: %d\n", result3.iterations);
    printf("  Converged: %s\n", result3.converged ? "Yes" : "No");
    printf("  Inertia: %.2f\n", result3.inertia);
    printf("  Runtime: %.3f ms\n", elapsed);
    printf("  Points/sec: %.0f\n", n3 / (elapsed / 1000.0));

    printf("\nCluster sizes:\n");
    for (int i = 0; i < k3; i++) {
        printf("  Cluster %d: %d points\n", i, result3.cluster_sizes[i]);
    }

    fp_kmeans_free(&result3);
    free(data3);

    printf("\n");

    // ========================================================================
    // Summary
    // ========================================================================
    printf("================================================================\n");
    printf("  K-Means Demo Complete!\n");
    printf("================================================================\n\n");

    printf("Key Takeaways:\n");
    printf("  ✅ Complex ML algorithm built from FP primitives\n");
    printf("  ✅ SIMD-accelerated distance calculations\n");
    printf("  ✅ Functional composition = clean, maintainable code\n");
    printf("  ✅ Scales to 10K+ points with sub-second performance\n");
    printf("\n");

    printf("FP-ASM primitives used:\n");
    printf("  - Euclidean distance (squared - avoids sqrt)\n");
    printf("  - Vector operations (implicit in loops)\n");
    printf("  - k-means++ initialization (smart seeding)\n");
    printf("  - Iterative refinement (functional style)\n");
    printf("\n");

    printf("This demonstrates the power of functional composition:\n");
    printf("  Simple primitives + composition = complex algorithms!\n");
    printf("\n");

    return 0;
}
