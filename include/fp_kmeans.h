/**
 * fp_kmeans.h - K-Means Clustering Algorithm API
 *
 * Provides K-Means clustering with both standard and safe (Maybe monad) interfaces.
 * Uses L0 ASM primitives internally for SIMD-accelerated distance computation.
 *
 * Features:
 * - K-Means++ initialization for better convergence
 * - Deterministic results via seed parameter
 * - Safe variant returning Maybe monad for error handling
 *
 * Example (standard):
 *   KMeansResult result = fp_kmeans_f64(data, n, d, k, 100, 1e-4, 42);
 *   fp_kmeans_print(&result, k, d);
 *   fp_kmeans_free(&result);
 *
 * Example (safe/monadic):
 *   Maybe result = fp_kmeans_f64_safe(data, n, d, k, 100, 1e-4, 42);
 *   if (fp_is_just(result)) {
 *       KMeansResult* r = (KMeansResult*)fp_from_just_ptr(result);
 *       // Use result...
 *       fp_kmeans_free(r);  // Free internal arrays
 *       free(r);            // Free the result struct itself
 *   }
 */

#ifndef FP_KMEANS_H
#define FP_KMEANS_H

#include <stdint.h>
#include "fp_monads.h"

/* ============================================================================
 * K-MEANS RESULT STRUCTURE
 * ============================================================================ */

typedef struct {
    double* centroids;        // k × d matrix (row-major)
    int* assignments;         // n-element array (cluster ID per point)
    int* cluster_sizes;       // k-element array (points per cluster)
    int iterations;           // Number of iterations to convergence
    double inertia;           // Sum of squared distances to centroids
    int converged;            // 1 if converged, 0 if max_iter reached
} KMeansResult;

/* ============================================================================
 * STANDARD K-MEANS API
 * ============================================================================ */

/**
 * fp_kmeans_f64 - Run K-Means clustering on double-precision data
 *
 * @param data      n × d data matrix (row-major, contiguous)
 * @param n         Number of data points
 * @param d         Dimensionality of each point
 * @param k         Number of clusters
 * @param max_iter  Maximum iterations before stopping
 * @param tol       Convergence tolerance (unused internally, reserved)
 * @param seed      RNG seed for deterministic k-means++ initialization
 *
 * @return KMeansResult struct with allocated arrays (caller must free via fp_kmeans_free)
 *
 * NOTE: Does NOT validate inputs. For safe version with validation, use fp_kmeans_f64_safe().
 */
KMeansResult fp_kmeans_f64(
    const double* data,
    int n,
    int d,
    int k,
    int max_iter,
    double tol,
    uint64_t seed
);

/**
 * fp_kmeans_free - Free internal arrays of KMeansResult
 *
 * @param result    Pointer to KMeansResult to free
 *
 * NOTE: This frees result->centroids, result->assignments, result->cluster_sizes.
 *       If the KMeansResult was heap-allocated (e.g., from fp_kmeans_f64_safe),
 *       you must also call free(result) separately, or use fp_kmeans_free_safe().
 */
void fp_kmeans_free(KMeansResult* result);

/**
 * fp_kmeans_free_safe - Convenience function for freeing heap-allocated KMeansResult
 *
 * @param result    Pointer to heap-allocated KMeansResult (from fp_kmeans_f64_safe)
 *
 * This function combines both cleanup steps required for results from fp_kmeans_f64_safe:
 *   1. Frees internal arrays (centroids, assignments, cluster_sizes)
 *   2. Frees the KMeansResult struct itself
 *
 * Safe to call with NULL (no-op).
 *
 * Example:
 *   Maybe result = fp_kmeans_f64_safe(data, n, d, k, 100, 1e-4, 42);
 *   if (fp_is_just(result)) {
 *       KMeansResult* r = (KMeansResult*)fp_from_just_ptr(result);
 *       // Use result...
 *       fp_kmeans_free_safe(r);  // Single call for complete cleanup
 *   }
 */
void fp_kmeans_free_safe(KMeansResult* result);

/**
 * fp_kmeans_print - Print K-Means result summary
 *
 * @param result    Pointer to KMeansResult to print
 * @param k         Number of clusters
 * @param d         Dimensionality
 */
void fp_kmeans_print(const KMeansResult* result, int k, int d);

/* ============================================================================
 * SAFE K-MEANS API (TIER 4: Maybe Monad)
 * ============================================================================ */

/**
 * fp_kmeans_f64_safe - Safe K-Means with Maybe monad error handling
 *
 * Returns Nothing for invalid inputs:
 * - NULL data pointer
 * - n <= 0 (no data points)
 * - d <= 0 (invalid dimensionality)
 * - k <= 0 (no clusters)
 * - k > n (more clusters than data points)
 * - max_iter <= 0 (invalid iteration count)
 * - tol < 0.0 (negative tolerance)
 * - Memory allocation failure
 *
 * Returns Just(result_ptr) on success with heap-allocated KMeansResult*.
 *
 * @param data      n × d data matrix (row-major, contiguous)
 * @param n         Number of data points
 * @param d         Dimensionality of each point
 * @param k         Number of clusters
 * @param max_iter  Maximum iterations before stopping
 * @param tol       Convergence tolerance (must be >= 0)
 * @param seed      RNG seed for deterministic k-means++ initialization
 *
 * @return Maybe containing KMeansResult* on success, or Nothing on error
 *
 * MEMORY MANAGEMENT:
 * If fp_is_just(result), caller must perform TWO cleanup steps:
 *   1. fp_kmeans_free(result_ptr)  - Free internal arrays (centroids, assignments, cluster_sizes)
 *   2. free(result_ptr)            - Free the KMeansResult struct itself
 *
 * Example:
 *   Maybe result = fp_kmeans_f64_safe(data, n, d, k, 100, 1e-4, 42);
 *   if (fp_is_just(result)) {
 *       KMeansResult* r = (KMeansResult*)fp_from_just_ptr(result);
 *       // Use result...
 *       fp_kmeans_free(r);  // Step 1: Free internal arrays
 *       free(r);            // Step 2: Free the struct
 *   } else {
 *       // Handle error - invalid parameters or allocation failure
 *   }
 */
Maybe fp_kmeans_f64_safe(
    const double* data,
    int n,
    int d,
    int k,
    int max_iter,
    double tol,
    uint64_t seed
);

#endif /* FP_KMEANS_H */
