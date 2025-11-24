// fp_kmeans.c
//
// K-Means Clustering Algorithm - Refactored with L0 ASM Primitives
// Demonstrates composition of FP-ASM primitives into a complete ML algorithm
//
// REFACTORED: Now uses L0 ASM primitives for SIMD acceleration:
//   - fp_reduce_add_f64() - Mean computation, distance summation
//   - fp_fold_dotp_f64() - Euclidean distance via dot product identity
//   - fp_rng - Deterministic initialization (no rand()!)
//
// FP Primitives Used:
//   - fp_reduce_add_f64: mean computation (sum / n)
//   - fp_fold_dotp_f64: ||a-b||² = ||a||² + ||b||² - 2(a·b)
//   - fp_rng_*: deterministic random initialization
//
// Algorithm:
// 1. Initialize k centroids (k-means++ for better convergence)
// 2. Assign each point to nearest centroid
// 3. Recompute centroids as mean of assigned points
// 4. Repeat until convergence or max iterations

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fp_core.h"
#include "fp_rng.h"
#include "fp_monads.h"  // TIER 4: Maybe monad for safe error handling
#include "fp_kmeans.h"  // Public API declarations

// Pattern 1 helpers (lightweight inline versions to avoid dependency issues)
// These follow the Pattern 1 style from fp_stats.h but are self-contained

static inline double fp_mean_inline(const double* data, size_t n) {
    if (!data || n == 0) return 0.0;
    // L0 ASM: SIMD reduction
    double sum = fp_reduce_add_f64(data, n);
    return sum / (double)n;
}

// KMeansResult is defined in fp_kmeans.h

// Compute squared Euclidean distance between two d-dimensional points
// REFACTORED: Uses L0 ASM primitives with identity: ||a-b||² = ||a||² + ||b||² - 2(a·b)
// Returns squared distance to avoid sqrt overhead (K-means only needs relative distances)
static inline double euclidean_distance_squared(const double* a, const double* b, int d) {
    if (!a || !b || d == 0) return 0.0;
    // L0 ASM: Use dot product identity instead of loop
    // ||a - b||² = a·a + b·b - 2*(a·b)
    double aa = fp_fold_dotp_f64(a, a, (size_t)d);  // ||a||²
    double bb = fp_fold_dotp_f64(b, b, (size_t)d);  // ||b||²
    double ab = fp_fold_dotp_f64(a, b, (size_t)d);  // a·b
    return aa + bb - 2.0 * ab;
}

// Initialize centroids using k-means++ algorithm
// REFACTORED: Uses fp_rng for deterministic, reproducible initialization
// Better than random initialization - ensures well-spread initial centroids
static fp_rng_t kmeans_plus_plus_init(
    const double* data,     // n × d matrix
    int n,                  // number of points
    int d,                  // dimensionality
    int k,                  // number of clusters
    double* centroids,      // k × d output matrix
    fp_rng_t rng            // RNG state (deterministic)
) {
    // Choose first centroid uniformly at random
    int64_t first_idx;
    rng = fp_rng_next_i64_range(rng, 0, n - 1, &first_idx);
    memcpy(centroids, &data[first_idx * d], d * sizeof(double));

    double* distances = (double*)malloc(n * sizeof(double));

    // Choose remaining k-1 centroids
    for (int c = 1; c < k; c++) {
        // Compute distance from each point to nearest existing centroid
        for (int i = 0; i < n; i++) {
            double min_dist = INFINITY;
            for (int j = 0; j < c; j++) {
                double dist = euclidean_distance_squared(
                    &data[i * d],
                    &centroids[j * d],
                    d
                );
                if (dist < min_dist) min_dist = dist;
            }
            distances[i] = min_dist;
        }
        // L0 ASM: Sum distances using SIMD reduction
        double total_dist = fp_reduce_add_f64(distances, (size_t)n);

        // Choose next centroid with probability proportional to distance^2
        double r_val;
        rng = fp_rng_next_f64(rng, &r_val);
        double r = r_val * total_dist;
        double cumsum = 0.0;
        for (int i = 0; i < n; i++) {
            cumsum += distances[i];
            if (cumsum >= r) {
                memcpy(&centroids[c * d], &data[i * d], d * sizeof(double));
                break;
            }
        }
    }

    free(distances);
    return rng;
}

// Assign each point to nearest centroid
// Returns number of points that changed assignment
static int assign_clusters(
    const double* data,       // n × d matrix
    int n,                    // number of points
    int d,                    // dimensionality
    const double* centroids,  // k × d matrix
    int k,                    // number of clusters
    int* assignments          // n-element output array
) {
    int changed = 0;

    for (int i = 0; i < n; i++) {
        double min_dist = INFINITY;
        int nearest = 0;

        // Find nearest centroid using Pattern 1 style distance computation
        for (int c = 0; c < k; c++) {
            double dist = euclidean_distance_squared(
                &data[i * d],
                &centroids[c * d],
                d
            );
            if (dist < min_dist) {
                min_dist = dist;
                nearest = c;
            }
        }

        if (assignments[i] != nearest) {
            changed++;
            assignments[i] = nearest;
        }
    }

    return changed;
}

// Recompute centroids as mean of assigned points
// REFACTORED: Uses Pattern 1's fp_mean() for clearer, safer computation
static void update_centroids(
    const double* data,       // n × d matrix
    int n,                    // number of points
    int d,                    // dimensionality
    const int* assignments,   // n-element array
    int k,                    // number of clusters
    double* centroids,        // k × d output matrix
    int* cluster_sizes        // k-element output array
) {
    // Zero out centroids and cluster sizes
    memset(centroids, 0, k * d * sizeof(double));
    memset(cluster_sizes, 0, k * sizeof(int));

    // Allocate temporary storage for cluster points (one dimension at a time)
    double* cluster_points = (double*)malloc(n * sizeof(double));

    // Compute centroid for each cluster, dimension by dimension
    for (int c = 0; c < k; c++) {
        // Count points in this cluster
        int count = 0;
        for (int i = 0; i < n; i++) {
            if (assignments[i] == c) {
                count++;
            }
        }
        cluster_sizes[c] = count;

        if (count == 0) continue;  // Empty cluster - leave at zero

        // For each dimension, extract cluster points and compute mean
        for (int j = 0; j < d; j++) {
            // Extract j-th dimension of all points in cluster c
            count = 0;
            for (int i = 0; i < n; i++) {
                if (assignments[i] == c) {
                    cluster_points[count++] = data[i * d + j];
                }
            }

            // Pattern 1: Use fp_mean_inline() instead of manual sum+divide
            // Benefits: Clearer intent, no division-by-zero risk, single-pass
            centroids[c * d + j] = fp_mean_inline(cluster_points, count);
        }
    }

    free(cluster_points);
}

// Compute inertia (sum of squared distances to assigned centroids)
// REFACTORED: Uses Pattern 1 style distance computation
static double compute_inertia(
    const double* data,
    int n,
    int d,
    const double* centroids,
    const int* assignments
) {
    double inertia = 0.0;
    for (int i = 0; i < n; i++) {
        double dist = euclidean_distance_squared(
            &data[i * d],
            &centroids[assignments[i] * d],
            d
        );
        inertia += dist;  // Already squared distance
    }
    return inertia;
}

// Main K-Means function
// REFACTORED: Added seed parameter for deterministic, reproducible results
// Uses functional composition: init -> iterate (assign + update) -> converge
KMeansResult fp_kmeans_f64(
    const double* data,       // n × d data matrix (row-major)
    int n,                    // number of data points
    int d,                    // dimensionality
    int k,                    // number of clusters
    int max_iter,             // maximum iterations
    double tol,               // convergence tolerance
    uint64_t seed             // RNG seed for deterministic initialization
) {
    KMeansResult result;

    // Allocate memory
    result.centroids = (double*)malloc(k * d * sizeof(double));
    result.assignments = (int*)malloc(n * sizeof(int));
    result.cluster_sizes = (int*)malloc(k * sizeof(int));

    // Initialize assignments to -1
    memset(result.assignments, -1, n * sizeof(int));

    // DETERMINISTIC: Initialize centroids using k-means++ with seed
    fp_rng_t rng = fp_rng_create(seed);
    rng = kmeans_plus_plus_init(data, n, d, k, result.centroids, rng);
    (void)rng;  // Suppress unused warning

    // Iterate until convergence or max iterations
    result.converged = 0;
    for (result.iterations = 0; result.iterations < max_iter; result.iterations++) {
        // Assign points to nearest centroids
        int changed = assign_clusters(data, n, d, result.centroids, k, result.assignments);

        // Check for convergence
        if (changed == 0) {
            result.converged = 1;
            break;
        }

        // Update centroids
        update_centroids(data, n, d, result.assignments, k, result.centroids, result.cluster_sizes);
    }

    // Compute final inertia
    result.inertia = compute_inertia(data, n, d, result.centroids, result.assignments);

    return result;
}

// Free K-Means result (internal arrays only)
void fp_kmeans_free(KMeansResult* result) {
    free(result->centroids);
    free(result->assignments);
    free(result->cluster_sizes);
}

// Convenience function for freeing heap-allocated KMeansResult (from fp_kmeans_f64_safe)
// Combines both cleanup steps: internal arrays + struct itself
void fp_kmeans_free_safe(KMeansResult* result) {
    if (result) {
        fp_kmeans_free(result);
        free(result);
    }
}

// ============================================================================
// TIER 4: Maybe Monad Wrapper for Safe K-Means
// ============================================================================
// Returns Nothing for invalid inputs, Just(result_ptr) on success
//
// MEMORY MANAGEMENT: If fp_is_just(result), caller must perform TWO cleanup steps:
//   1. fp_kmeans_free(result_ptr)  - Free internal arrays (centroids, assignments, cluster_sizes)
//   2. free(result_ptr)            - Free the heap-allocated KMeansResult struct itself

Maybe fp_kmeans_f64_safe(
    const double* data,
    int n,
    int d,
    int k,
    int max_iter,
    double tol,
    uint64_t seed
) {
    // Validate inputs - return Nothing for edge cases
    if (!data) return fp_nothing();           // NULL data
    if (n <= 0) return fp_nothing();          // No data points
    if (d <= 0) return fp_nothing();          // Invalid dimensionality
    if (k <= 0) return fp_nothing();          // No clusters
    if (k > n) return fp_nothing();           // More clusters than points
    if (max_iter <= 0) return fp_nothing();   // Invalid max iterations
    if (tol < 0.0) return fp_nothing();       // Negative tolerance

    // Allocate result on heap (caller must free)
    KMeansResult* result = (KMeansResult*)malloc(sizeof(KMeansResult));
    if (!result) return fp_nothing();         // Allocation failed

    // Run K-Means
    *result = fp_kmeans_f64(data, n, d, k, max_iter, tol, seed);

    // Check allocation success inside result
    if (!result->centroids || !result->assignments || !result->cluster_sizes) {
        fp_kmeans_free(result);
        free(result);
        return fp_nothing();
    }

    return fp_just_ptr(result);
}

// Print K-Means result
void fp_kmeans_print(const KMeansResult* result, int k, int d) {
    printf("K-Means Result:\n");
    printf("  Iterations: %d\n", result->iterations);
    printf("  Converged: %s\n", result->converged ? "Yes" : "No (max iter)");
    printf("  Inertia: %.4f\n", result->inertia);
    printf("\nCentroids:\n");
    for (int i = 0; i < k; i++) {
        printf("  Cluster %d (n=%d): [", i, result->cluster_sizes[i]);
        for (int j = 0; j < d; j++) {
            printf("%.3f", result->centroids[i * d + j]);
            if (j < d - 1) printf(", ");
        }
        printf("]\n");
    }
}
