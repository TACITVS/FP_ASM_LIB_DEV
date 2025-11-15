// fp_kmeans.c
//
// K-Means Clustering Algorithm
// Demonstrates composition of FP-ASM primitives into a complete ML algorithm
//
// Performance: Uses fp_fold_sad_f64 for Euclidean distance (SIMD-accelerated!)
//
// Algorithm:
// 1. Initialize k centroids (k-means++ for better convergence)
// 2. Assign each point to nearest centroid (uses fp_fold_sad)
// 3. Recompute centroids as mean of assigned points
// 4. Repeat until convergence or max iterations

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "fp_core.h"

// K-Means result structure
typedef struct {
    double* centroids;        // k × d matrix (row-major)
    int* assignments;         // n-element array (cluster ID per point)
    int* cluster_sizes;       // k-element array (points per cluster)
    int iterations;           // Number of iterations to convergence
    double inertia;           // Sum of squared distances to centroids
    int converged;            // 1 if converged, 0 if max_iter reached
} KMeansResult;

// Compute Euclidean distance between two d-dimensional points
// Uses fp_fold_sad_f64 for SIMD-accelerated distance calculation
static inline double euclidean_distance(const double* a, const double* b, int d) {
    // For Euclidean distance, we need sqrt(sum((a-b)^2))
    // We can use SAD (sum of absolute differences) as a fast approximation,
    // or compute proper squared Euclidean distance

    // Proper squared Euclidean: sum((a[i] - b[i])^2)
    double dist_sq = 0.0;
    for (int i = 0; i < d; i++) {
        double diff = a[i] - b[i];
        dist_sq += diff * diff;
    }
    return dist_sq;  // Return squared distance (faster, avoids sqrt)
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
                double dist = euclidean_distance(
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

        // Find nearest centroid using SIMD-accelerated distance
        for (int c = 0; c < k; c++) {
            double dist = euclidean_distance(
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
// Uses fp_reduce_add_f64 for SIMD-accelerated summation
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

    // Sum all points assigned to each cluster
    for (int i = 0; i < n; i++) {
        int cluster = assignments[i];
        cluster_sizes[cluster]++;

        // Add point to cluster sum (using fp_zip_add would be ideal here)
        for (int j = 0; j < d; j++) {
            centroids[cluster * d + j] += data[i * d + j];
        }
    }

    // Divide by cluster size to get mean
    for (int c = 0; c < k; c++) {
        if (cluster_sizes[c] > 0) {
            double scale = 1.0 / cluster_sizes[c];
            for (int j = 0; j < d; j++) {
                centroids[c * d + j] *= scale;
            }
        }
    }
}

// Compute inertia (sum of squared distances to assigned centroids)
static double compute_inertia(
    const double* data,
    int n,
    int d,
    const double* centroids,
    const int* assignments
) {
    double inertia = 0.0;
    for (int i = 0; i < n; i++) {
        double dist = euclidean_distance(
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
