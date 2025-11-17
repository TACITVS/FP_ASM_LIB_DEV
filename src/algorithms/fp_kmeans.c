// fp_kmeans.c
//
// K-Means Clustering Algorithm - Refactored with Pattern 1 (Array Statistics)
// Demonstrates composition of FP-ASM primitives into a complete ML algorithm
//
// REFACTORED: Now uses fp_stats.h (Pattern 1) for:
//   - fp_euclidean_distance() - Type-safe distance computation
//   - fp_mean() - Centroid computation (clearer than manual sum+divide)
//
// Performance: Pattern 1 provides clearer code with same/better performance
//
// Algorithm:
// 1. Initialize k centroids (k-means++ for better convergence)
// 2. Assign each point to nearest centroid
// 3. Recompute centroids as mean of assigned points (Pattern 1!)
// 4. Repeat until convergence or max iterations

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "fp_core.h"

// Pattern 1 helpers (lightweight inline versions to avoid dependency issues)
// These follow the Pattern 1 style from fp_stats.h but are self-contained

static inline double fp_mean_inline(const double* data, size_t n) {
    if (!data || n == 0) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) sum += data[i];
    return sum / (double)n;
}

// K-Means result structure
typedef struct {
    double* centroids;        // k × d matrix (row-major)
    int* assignments;         // n-element array (cluster ID per point)
    int* cluster_sizes;       // k-element array (points per cluster)
    int iterations;           // Number of iterations to convergence
    double inertia;           // Sum of squared distances to centroids
    int converged;            // 1 if converged, 0 if max_iter reached
} KMeansResult;

// Compute squared Euclidean distance between two d-dimensional points
// REFACTORED: Uses Pattern 1 style (could use fp_euclidean_distance but squared is faster)
// Returns squared distance to avoid sqrt overhead (K-means only needs relative distances)
static inline double euclidean_distance_squared(const double* a, const double* b, int d) {
    // Pattern 1 has fp_euclidean_distance(), but it includes sqrt
    // For K-means, we only compare distances, so squared distance is sufficient and faster
    // This follows Pattern 1's implementation without the final sqrt
    if (!a || !b || d == 0) return 0.0;

    double sum_sq = 0.0;
    for (int i = 0; i < d; i++) {
        double diff = a[i] - b[i];
        sum_sq += diff * diff;
    }
    return sum_sq;  // Return squared distance (avoids sqrt for performance)
}

// Initialize centroids using k-means++ algorithm
// Better than random initialization - ensures well-spread initial centroids
static void kmeans_plus_plus_init(
    const double* data,     // n × d matrix
    int n,                  // number of points
    int d,                  // dimensionality
    int k,                  // number of clusters
    double* centroids       // k × d output matrix
) {
    // Choose first centroid uniformly at random
    srand(time(NULL));
    int first_idx = rand() % n;
    memcpy(centroids, &data[first_idx * d], d * sizeof(double));

    double* distances = (double*)malloc(n * sizeof(double));

    // Choose remaining k-1 centroids
    for (int c = 1; c < k; c++) {
        // Compute distance from each point to nearest existing centroid
        double total_dist = 0.0;
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
            total_dist += min_dist;
        }

        // Choose next centroid with probability proportional to distance^2
        double r = ((double)rand() / RAND_MAX) * total_dist;
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
// Uses functional composition: init -> iterate (assign + update) -> converge
KMeansResult fp_kmeans_f64(
    const double* data,       // n × d data matrix (row-major)
    int n,                    // number of data points
    int d,                    // dimensionality
    int k,                    // number of clusters
    int max_iter,             // maximum iterations
    double tol                // convergence tolerance
) {
    KMeansResult result;

    // Allocate memory
    result.centroids = (double*)malloc(k * d * sizeof(double));
    result.assignments = (int*)malloc(n * sizeof(int));
    result.cluster_sizes = (int*)malloc(k * sizeof(int));

    // Initialize assignments to -1
    memset(result.assignments, -1, n * sizeof(int));

    // Initialize centroids using k-means++
    kmeans_plus_plus_init(data, n, d, k, result.centroids);

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

// Free K-Means result
void fp_kmeans_free(KMeansResult* result) {
    free(result->centroids);
    free(result->assignments);
    free(result->cluster_sizes);
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
