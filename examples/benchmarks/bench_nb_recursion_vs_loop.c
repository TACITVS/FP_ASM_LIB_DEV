/**
 * bench_nb_recursion_vs_loop.c
 *
 * Benchmark: FP Purist Tail Recursion vs Imperative For-Loop
 *
 * Compares performance of:
 * - Tail-recursive batch prediction (FP purist - ZERO for-loops!)
 * - Imperative for-loop batch prediction (kept for comparison)
 *
 * Tests Naive Bayes batch prediction on varying sample sizes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include "fp_naive_bayes.h"
#include "fp_rng.h"

// Expose imperative versions for benchmarking
// These are declared static in fp_naive_bayes.c, so we duplicate them here

// IMPERATIVE VERSION - For-loop (NOT FP PURIST!)
static void fp_gaussian_nb_predict_batch_imperative(
    const GaussianNBModel* model,
    const double* X,
    int n,
    int* predictions
) {
    for (int i = 0; i < n; i++) {
        const double* x = &X[i * model->n_features];
        NBPrediction pred;
        pred = fp_gaussian_nb_predict(model, x);
        predictions[i] = pred.predicted_class;
        free(pred.probabilities);
    }
}

static void fp_multinomial_nb_predict_batch_imperative(
    const MultinomialNBModel* model,
    const double* X,
    int n,
    int* predictions
) {
    for (int i = 0; i < n; i++) {
        const double* x = &X[i * model->n_features];
        NBPrediction pred;
        pred = fp_multinomial_nb_predict(model, x);
        predictions[i] = pred.predicted_class;
        free(pred.probabilities);
    }
}

// ============================================================================
// Timing Utilities (Windows-compatible)
// ============================================================================

double get_time_ms() {
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000.0) / frequency.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
#endif
}

// ============================================================================
// Benchmark: Gaussian Naive Bayes
// ============================================================================

void bench_gaussian_nb(int n_samples, int n_features, int n_iterations) {
    printf("\n--- Gaussian NB: n=%d, d=%d, iterations=%d ---\n", n_samples, n_features, n_iterations);

    // Generate synthetic data
    int n_classes = 3;
    double* X = (double*)malloc(n_samples * n_features * sizeof(double));
    int* y = (int*)malloc(n_samples * sizeof(int));

    uint64_t seed = 42;
    fp_rng_t rng = fp_rng_create(seed);

    for (int i = 0; i < n_samples; i++) {
        y[i] = i % n_classes;
        for (int j = 0; j < n_features; j++) {
            double val;
            rng = fp_rng_next_f64(rng, &val);
            X[i * n_features + j] = val * 10.0 + y[i] * 5.0;
        }
    }

    // Train model
    GaussianNBModel model = fp_gaussian_nb_train(X, y, n_samples, n_features, n_classes);

    // Allocate prediction arrays
    int* predictions_recursive = (int*)malloc(n_samples * sizeof(int));
    int* predictions_imperative = (int*)malloc(n_samples * sizeof(int));

    // Warmup
    fp_gaussian_nb_predict_batch(&model, X, n_samples, predictions_recursive);
    fp_gaussian_nb_predict_batch_imperative(&model, X, n_samples, predictions_imperative);

    // ========================================
    // Benchmark: FP Purist (Tail Recursion)
    // ========================================
    double start = get_time_ms();
    for (int iter = 0; iter < n_iterations; iter++) {
        fp_gaussian_nb_predict_batch(&model, X, n_samples, predictions_recursive);
    }
    double end = get_time_ms();
    double time_recursive = end - start;

    // ========================================
    // Benchmark: Imperative (For-Loop)
    // ========================================
    start = get_time_ms();
    for (int iter = 0; iter < n_iterations; iter++) {
        fp_gaussian_nb_predict_batch_imperative(&model, X, n_samples, predictions_imperative);
    }
    end = get_time_ms();
    double time_imperative = end - start;

    // Verify results match
    int mismatches = 0;
    for (int i = 0; i < n_samples; i++) {
        if (predictions_recursive[i] != predictions_imperative[i]) {
            mismatches++;
        }
    }

    // Report results
    double speedup = time_imperative / time_recursive;
    printf("FP Purist (Recursion):  %.2f ms  (%.2f μs/sample)\n",
           time_recursive, (time_recursive * 1000.0) / (n_samples * n_iterations));
    printf("Imperative (For-Loop):  %.2f ms  (%.2f μs/sample)\n",
           time_imperative, (time_imperative * 1000.0) / (n_samples * n_iterations));
    printf("Speedup: %.3fx %s\n", speedup,
           speedup > 1.0 ? "(FP Purist FASTER)" : "(Imperative FASTER)");
    printf("Correctness: %s\n", mismatches == 0 ? "PASS (results identical)" : "FAIL (mismatches!)");

    // Cleanup
    free(X);
    free(y);
    free(predictions_recursive);
    free(predictions_imperative);
    fp_nb_free_gaussian_model(&model);
}

// ============================================================================
// Benchmark: Multinomial Naive Bayes
// ============================================================================

void bench_multinomial_nb(int n_samples, int n_features, int n_iterations) {
    printf("\n--- Multinomial NB: n=%d, d=%d, iterations=%d ---\n", n_samples, n_features, n_iterations);

    // Generate synthetic count data
    int n_classes = 3;
    double* X = (double*)malloc(n_samples * n_features * sizeof(double));
    int* y = (int*)malloc(n_samples * sizeof(int));

    uint64_t seed = 42;
    fp_rng_t rng = fp_rng_create(seed);

    for (int i = 0; i < n_samples; i++) {
        y[i] = i % n_classes;
        for (int j = 0; j < n_features; j++) {
            double val;
            rng = fp_rng_next_f64(rng, &val);
            // Count data (integers >= 0)
            X[i * n_features + j] = (int)(val * 20.0);
        }
    }

    // Train model
    MultinomialNBModel model = fp_multinomial_nb_train(X, y, n_samples, n_features, n_classes, 1.0);

    // Allocate prediction arrays
    int* predictions_recursive = (int*)malloc(n_samples * sizeof(int));
    int* predictions_imperative = (int*)malloc(n_samples * sizeof(int));

    // Warmup
    fp_multinomial_nb_predict_batch(&model, X, n_samples, predictions_recursive);
    fp_multinomial_nb_predict_batch_imperative(&model, X, n_samples, predictions_imperative);

    // ========================================
    // Benchmark: FP Purist (Tail Recursion)
    // ========================================
    double start = get_time_ms();
    for (int iter = 0; iter < n_iterations; iter++) {
        fp_multinomial_nb_predict_batch(&model, X, n_samples, predictions_recursive);
    }
    double end = get_time_ms();
    double time_recursive = end - start;

    // ========================================
    // Benchmark: Imperative (For-Loop)
    // ========================================
    start = get_time_ms();
    for (int iter = 0; iter < n_iterations; iter++) {
        fp_multinomial_nb_predict_batch_imperative(&model, X, n_samples, predictions_imperative);
    }
    end = get_time_ms();
    double time_imperative = end - start;

    // Verify results match
    int mismatches = 0;
    for (int i = 0; i < n_samples; i++) {
        if (predictions_recursive[i] != predictions_imperative[i]) {
            mismatches++;
        }
    }

    // Report results
    double speedup = time_imperative / time_recursive;
    printf("FP Purist (Recursion):  %.2f ms  (%.2f μs/sample)\n",
           time_recursive, (time_recursive * 1000.0) / (n_samples * n_iterations));
    printf("Imperative (For-Loop):  %.2f ms  (%.2f μs/sample)\n",
           time_imperative, (time_imperative * 1000.0) / (n_samples * n_iterations));
    printf("Speedup: %.3fx %s\n", speedup,
           speedup > 1.0 ? "(FP Purist FASTER)" : "(Imperative FASTER)");
    printf("Correctness: %s\n", mismatches == 0 ? "PASS (results identical)" : "FAIL (mismatches!)");

    // Cleanup
    free(X);
    free(y);
    free(predictions_recursive);
    free(predictions_imperative);
    fp_nb_free_multinomial_model(&model);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("========================================\n");
    printf("FP Purist vs Imperative Benchmark\n");
    printf("Naive Bayes Batch Prediction\n");
    printf("========================================\n");

    // Test different sample sizes
    int iterations = 100;  // Repeat each benchmark 100 times

    printf("\n=== SMALL: 100 samples ===\n");
    bench_gaussian_nb(100, 10, iterations);
    bench_multinomial_nb(100, 10, iterations);

    printf("\n=== MEDIUM: 1,000 samples ===\n");
    bench_gaussian_nb(1000, 20, iterations);
    bench_multinomial_nb(1000, 20, iterations);

    printf("\n=== LARGE: 10,000 samples ===\n");
    bench_gaussian_nb(10000, 50, iterations);
    bench_multinomial_nb(10000, 50, iterations);

    printf("\n=== VERY LARGE: 100,000 samples ===\n");
    bench_gaussian_nb(100000, 100, 10);  // Fewer iterations for large data
    bench_multinomial_nb(100000, 100, 10);

    printf("\n========================================\n");
    printf("Benchmark Complete\n");
    printf("========================================\n");

    printf("\nConclusion:\n");
    printf("- If FP Purist (recursion) is faster: compiler tail-call optimization works!\n");
    printf("- If Imperative (loop) is faster: consider using GCC -O3 -foptimize-sibling-calls\n");
    printf("- In most cases, they should be identical (tail recursion → loop conversion)\n");

    return 0;
}
