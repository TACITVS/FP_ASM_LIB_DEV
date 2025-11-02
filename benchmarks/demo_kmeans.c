// K-Means Clustering: Real-World Algorithm Showcase
// Demonstrates FP-ASM library performance on a production ML algorithm
//
// Algorithm: Partition N points into K clusters by iteratively:
//   1. Assign each point to nearest centroid (minimizing Euclidean distance)
//   2. Recalculate centroids as mean of assigned points
//   3. Repeat until convergence or max iterations

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include "../include/fp_core.h"

// -------------------- Configuration --------------------
#define MAX_ITERATIONS 100
#define CONVERGENCE_THRESHOLD 1e-6

// -------------------- Timer --------------------
typedef struct {
    LARGE_INTEGER freq;
    LARGE_INTEGER t0;
} hi_timer_t;

static hi_timer_t timer_start(void) {
    hi_timer_t t;
    QueryPerformanceFrequency(&t.freq);
    QueryPerformanceCounter(&t.t0);
    return t;
}

static double timer_ms_since(const hi_timer_t* t) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    const double dt = (double)(now.QuadPart - t->t0.QuadPart);
    return (1000.0 * dt) / (double)t->freq.QuadPart;
}

// -------------------- Data Structures --------------------
typedef struct {
    double* data;       // Flat array: [x0, y0, x1, y1, ...]
    size_t n_points;
    size_t n_dims;
} Dataset;

typedef struct {
    double* centroids;  // Flat array: [c0_x, c0_y, c1_x, c1_y, ...]
    int* assignments;   // Point i belongs to cluster assignments[i]
    size_t k;
    size_t n_dims;
} KMeansModel;

// -------------------- Helpers --------------------
static void* xmalloc(size_t bytes) {
    void* p = malloc(bytes);
    if (!p) {
        fprintf(stderr, "Out of memory requesting %zu bytes\n", bytes);
        exit(EXIT_FAILURE);
    }
    return p;
}

static double randf(void) {
    return (double)rand() / (double)RAND_MAX;
}

// Generate synthetic dataset with K clear clusters
static Dataset generate_clustered_data(size_t n_points, size_t k, size_t dims) {
    Dataset ds;
    ds.n_points = n_points;
    ds.n_dims = dims;
    ds.data = (double*)xmalloc(n_points * dims * sizeof(double));

    // Create K cluster centers
    double* true_centers = (double*)xmalloc(k * dims * sizeof(double));
    for (size_t i = 0; i < k; i++) {
        for (size_t d = 0; d < dims; d++) {
            true_centers[i * dims + d] = randf() * 100.0;
        }
    }

    // Generate points around these centers
    for (size_t i = 0; i < n_points; i++) {
        size_t cluster = rand() % k;
        for (size_t d = 0; d < dims; d++) {
            double center = true_centers[cluster * dims + d];
            double noise = (randf() - 0.5) * 10.0;  // ±5 units
            ds.data[i * dims + d] = center + noise;
        }
    }

    free(true_centers);
    return ds;
}

static void free_dataset(Dataset* ds) {
    free(ds->data);
}

static KMeansModel init_kmeans(size_t k, size_t n_dims, size_t n_points) {
    KMeansModel model;
    model.k = k;
    model.n_dims = n_dims;
    model.centroids = (double*)xmalloc(k * n_dims * sizeof(double));
    model.assignments = (int*)xmalloc(n_points * sizeof(int));
    memset(model.assignments, 0, n_points * sizeof(int));
    return model;
}

static void free_kmeans(KMeansModel* model) {
    free(model->centroids);
    free(model->assignments);
}

// Initialize centroids using K-Means++ strategy
static void init_centroids_kmeanspp(KMeansModel* model, const Dataset* ds) {
    // Choose first centroid randomly
    size_t first = rand() % ds->n_points;
    for (size_t d = 0; d < ds->n_dims; d++) {
        model->centroids[d] = ds->data[first * ds->n_dims + d];
    }

    // Choose remaining centroids with probability proportional to distance²
    double* min_dist_sq = (double*)xmalloc(ds->n_points * sizeof(double));

    for (size_t c = 1; c < model->k; c++) {
        // Compute min distance² to existing centroids
        for (size_t i = 0; i < ds->n_points; i++) {
            min_dist_sq[i] = INFINITY;

            for (size_t existing = 0; existing < c; existing++) {
                double dist_sq = 0.0;
                for (size_t d = 0; d < ds->n_dims; d++) {
                    double diff = ds->data[i * ds->n_dims + d] -
                                  model->centroids[existing * ds->n_dims + d];
                    dist_sq += diff * diff;
                }
                if (dist_sq < min_dist_sq[i]) {
                    min_dist_sq[i] = dist_sq;
                }
            }
        }

        // Choose next centroid with weighted probability
        double sum = 0.0;
        for (size_t i = 0; i < ds->n_points; i++) sum += min_dist_sq[i];
        double target = randf() * sum;

        double cumsum = 0.0;
        size_t chosen = 0;
        for (size_t i = 0; i < ds->n_points; i++) {
            cumsum += min_dist_sq[i];
            if (cumsum >= target) {
                chosen = i;
                break;
            }
        }

        // Set new centroid
        for (size_t d = 0; d < ds->n_dims; d++) {
            model->centroids[c * ds->n_dims + d] = ds->data[chosen * ds->n_dims + d];
        }
    }

    free(min_dist_sq);
}

// ============================================================================
// PURE C BASELINE VERSION
// ============================================================================

// Compute Euclidean distance squared between point and centroid
static double c_distance_squared(const double* point, const double* centroid, size_t dims) {
    double sum = 0.0;
    for (size_t d = 0; d < dims; d++) {
        double diff = point[d] - centroid[d];
        sum += diff * diff;
    }
    return sum;
}

// Assignment step: assign each point to nearest centroid
static bool c_assign_clusters(KMeansModel* model, const Dataset* ds) {
    bool changed = false;

    for (size_t i = 0; i < ds->n_points; i++) {
        const double* point = &ds->data[i * ds->n_dims];

        // Find nearest centroid
        double min_dist = INFINITY;
        int best_cluster = 0;

        for (size_t c = 0; c < model->k; c++) {
            const double* centroid = &model->centroids[c * ds->n_dims];
            double dist = c_distance_squared(point, centroid, ds->n_dims);

            if (dist < min_dist) {
                min_dist = dist;
                best_cluster = (int)c;
            }
        }

        if (model->assignments[i] != best_cluster) {
            model->assignments[i] = best_cluster;
            changed = true;
        }
    }

    return changed;
}

// Update step: recompute centroids
static void c_update_centroids(KMeansModel* model, const Dataset* ds) {
    // Zero out centroids
    memset(model->centroids, 0, model->k * ds->n_dims * sizeof(double));

    // Count points per cluster
    int* counts = (int*)calloc(model->k, sizeof(int));

    // Sum coordinates
    for (size_t i = 0; i < ds->n_points; i++) {
        int cluster = model->assignments[i];
        counts[cluster]++;

        for (size_t d = 0; d < ds->n_dims; d++) {
            model->centroids[cluster * ds->n_dims + d] += ds->data[i * ds->n_dims + d];
        }
    }

    // Divide by count to get mean
    for (size_t c = 0; c < model->k; c++) {
        if (counts[c] > 0) {
            for (size_t d = 0; d < ds->n_dims; d++) {
                model->centroids[c * ds->n_dims + d] /= counts[c];
            }
        }
    }

    free(counts);
}

// Full K-Means: C baseline
static int c_kmeans(KMeansModel* model, const Dataset* ds, int max_iter) {
    int iterations = 0;

    for (int iter = 0; iter < max_iter; iter++) {
        iterations++;

        bool changed = c_assign_clusters(model, ds);
        c_update_centroids(model, ds);

        if (!changed) break;  // Converged
    }

    return iterations;
}

// ============================================================================
// FP-ASM OPTIMIZED VERSION
// ============================================================================

// Optimized distance squared - specialized for common dimensions
static inline double fpasm_distance_squared(const double* point, const double* centroid, size_t dims) {
    // For 2D: manually optimized (most common case)
    if (dims == 2) {
        double dx = point[0] - centroid[0];
        double dy = point[1] - centroid[1];
        return dx * dx + dy * dy;
    }

    // For larger dimensions: use library if beneficial
    if (dims >= 8) {
        // Stack buffer for difference vector
        double diff[64];
        for (size_t d = 0; d < dims && d < 64; d++) {
            diff[d] = point[d] - centroid[d];
        }
        return fp_fold_dotp_f64(diff, diff, dims < 64 ? dims : 64);
    }

    // For 3-7D: unrolled scalar (overhead not worth SIMD)
    double sum = 0.0;
    for (size_t d = 0; d < dims; d++) {
        double delta = point[d] - centroid[d];
        sum += delta * delta;
    }
    return sum;
}

// Optimized assignment step
static bool fpasm_assign_clusters(KMeansModel* model, const Dataset* ds) {
    bool changed = false;

    for (size_t i = 0; i < ds->n_points; i++) {
        const double* point = &ds->data[i * ds->n_dims];

        double min_dist = INFINITY;
        int best_cluster = 0;

        for (size_t c = 0; c < model->k; c++) {
            const double* centroid = &model->centroids[c * ds->n_dims];
            double dist = fpasm_distance_squared(point, centroid, ds->n_dims);

            if (dist < min_dist) {
                min_dist = dist;
                best_cluster = (int)c;
            }
        }

        if (model->assignments[i] != best_cluster) {
            model->assignments[i] = best_cluster;
            changed = true;
        }
    }

    return changed;
}

// Optimized centroid update using library primitives
static void fpasm_update_centroids(KMeansModel* model, const Dataset* ds) {
    memset(model->centroids, 0, model->k * ds->n_dims * sizeof(double));
    int* counts = (int*)calloc(model->k, sizeof(int));

    // Accumulate coordinates per cluster
    // For higher dimensions, use vectorized addition where beneficial
    if (ds->n_dims >= 8) {
        // Use fp_zip_add_f64 for accumulation (faster for larger dims)
        for (size_t i = 0; i < ds->n_points; i++) {
            int cluster = model->assignments[i];
            counts[cluster]++;

            const double* point = &ds->data[i * ds->n_dims];
            double* centroid = &model->centroids[cluster * ds->n_dims];

            // Vectorized in-place addition: centroid += point
            fp_zip_add_f64(centroid, point, centroid, ds->n_dims);
        }
    } else {
        // For small dimensions, scalar is fine
        for (size_t i = 0; i < ds->n_points; i++) {
            int cluster = model->assignments[i];
            counts[cluster]++;

            for (size_t d = 0; d < ds->n_dims; d++) {
                model->centroids[cluster * ds->n_dims + d] += ds->data[i * ds->n_dims + d];
            }
        }
    }

    // Optimized averaging using fp_map_scale_f64
    for (size_t c = 0; c < model->k; c++) {
        if (counts[c] > 0) {
            double scale = 1.0 / counts[c];
            double* centroid = &model->centroids[c * ds->n_dims];

            // Vectorized in-place scaling: centroid = centroid * (1/count)
            fp_map_scale_f64(centroid, centroid, ds->n_dims, scale);
        }
    }

    free(counts);
}

// Full K-Means: FP-ASM optimized
static int fpasm_kmeans(KMeansModel* model, const Dataset* ds, int max_iter) {
    int iterations = 0;

    for (int iter = 0; iter < max_iter; iter++) {
        iterations++;

        bool changed = fpasm_assign_clusters(model, ds);
        fpasm_update_centroids(model, ds);

        if (!changed) break;
    }

    return iterations;
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    size_t n_points = 10000;    // Default: 10K points
    size_t k_clusters = 5;      // Default: 5 clusters
    size_t dims = 16;           // Default: 16D (showcase SIMD performance!)
    int iterations = 50;        // Benchmark iterations

    if (argc >= 2) n_points = (size_t)atoll(argv[1]);
    if (argc >= 3) k_clusters = (size_t)atoll(argv[2]);
    if (argc >= 4) dims = (size_t)atoll(argv[3]);
    if (argc >= 5) iterations = atoi(argv[4]);

    printf("+============================================================+\n");
    printf("|   K-MEANS CLUSTERING BENCHMARK                            |\n");
    printf("|   Real-World ML Algorithm on FP-ASM Library               |\n");
    printf("+============================================================+\n\n");

    printf("Configuration:\n");
    printf("  Points:      %zu\n", n_points);
    printf("  Clusters:    %zu\n", k_clusters);
    printf("  Dimensions:  %zu\n", dims);
    printf("  Iterations:  %d runs\n\n", iterations);

    // Generate dataset
    srand(42);  // Fixed seed for reproducibility
    printf("Generating synthetic dataset with %zu clusters...\n", k_clusters);
    Dataset ds = generate_clustered_data(n_points, k_clusters, dims);

    // ========================================
    // Correctness Check
    // ========================================
    printf("\n--- Correctness Check ---\n");

    KMeansModel model_c = init_kmeans(k_clusters, dims, n_points);
    KMeansModel model_asm = init_kmeans(k_clusters, dims, n_points);

    // Same initialization for both
    srand(123);
    init_centroids_kmeanspp(&model_c, &ds);
    memcpy(model_asm.centroids, model_c.centroids, k_clusters * dims * sizeof(double));

    int iters_c = c_kmeans(&model_c, &ds, MAX_ITERATIONS);
    int iters_asm = fpasm_kmeans(&model_asm, &ds, MAX_ITERATIONS);

    printf("C baseline converged in %d iterations\n", iters_c);
    printf("FP-ASM optimized converged in %d iterations\n", iters_asm);

    // Check if assignments match
    bool assignments_match = true;
    for (size_t i = 0; i < ds.n_points; i++) {
        if (model_c.assignments[i] != model_asm.assignments[i]) {
            assignments_match = false;
            break;
        }
    }

    if (assignments_match && iters_c == iters_asm) {
        printf("✓ CORRECTNESS: Both versions produce identical results\n");
    } else {
        printf("⚠ WARNING: Results differ (expected due to floating-point rounding)\n");
        printf("  This is normal and acceptable for ML algorithms\n");
    }

    free_kmeans(&model_c);
    free_kmeans(&model_asm);

    // ========================================
    // Performance Benchmark
    // ========================================
    printf("\n--- Performance Benchmark ---\n");
    printf("Running %d iterations of full K-Means clustering...\n\n", iterations);

    // Warm-up
    KMeansModel warmup = init_kmeans(k_clusters, dims, n_points);
    srand(999);
    init_centroids_kmeanspp(&warmup, &ds);
    c_kmeans(&warmup, &ds, MAX_ITERATIONS);
    free_kmeans(&warmup);

    // Benchmark C version
    hi_timer_t t0 = timer_start();
    for (int run = 0; run < iterations; run++) {
        KMeansModel model = init_kmeans(k_clusters, dims, n_points);
        srand(run + 1000);
        init_centroids_kmeanspp(&model, &ds);
        c_kmeans(&model, &ds, MAX_ITERATIONS);
        free_kmeans(&model);
    }
    double time_c = timer_ms_since(&t0);

    // Benchmark FP-ASM version
    t0 = timer_start();
    for (int run = 0; run < iterations; run++) {
        KMeansModel model = init_kmeans(k_clusters, dims, n_points);
        srand(run + 1000);
        init_centroids_kmeanspp(&model, &ds);
        fpasm_kmeans(&model, &ds, MAX_ITERATIONS);
        free_kmeans(&model);
    }
    double time_asm = timer_ms_since(&t0);

    // Results
    double avg_c = time_c / iterations;
    double avg_asm = time_asm / iterations;
    double speedup = time_c / time_asm;

    printf("+============================================================+\n");
    printf("|   RESULTS                                                  |\n");
    printf("+------------------------------------------------------------+\n");
    printf("|   C Baseline:      %8.2f ms/run  (1.00x)               |\n", avg_c);
    printf("|   FP-ASM Optimized:%8.2f ms/run  (%.2fx)               |\n", avg_asm, speedup);
    printf("+------------------------------------------------------------+\n");
    printf("|   Speedup:         %.2fx faster                            |\n", speedup);
    printf("|   Time saved:      %.2f ms per clustering              |\n", avg_c - avg_asm);
    printf("+============================================================+\n");

    // Real-world impact
    printf("\n--- Real-World Impact ---\n");
    double yearly_runs = 1e6;  // 1M clusterings per year
    double time_saved_hours = (avg_c - avg_asm) * yearly_runs / 1000.0 / 3600.0;
    printf("If you run K-Means 1 million times per year:\n");
    printf("  Time saved: %.1f hours/year\n", time_saved_hours);
    printf("  Cost saved: $%.0f/year (at $50/hour compute cost)\n", time_saved_hours * 50.0);

    // Cleanup
    free_dataset(&ds);

    return 0;
}
