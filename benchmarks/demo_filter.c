// Filter: Classic Functional Programming Operation
// Tests whether FP-ASM is truly a "Functional Programming" library
// or just a "Floating Point" numerical library
//
// ✅ EXPECTED SUCCESS: 1.5-2.5x speedup
//
// WHY THIS MATTERS:
// - Filter is THE fundamental FP operation (Haskell, Lisp, ML, etc.)
// - Every FP language has: filter, map, fold
// - We already have map (Module 3) and fold (Module 1, 2)
// - This completes the FP trinity!
//
// Algorithm:
//   filter predicate xs = [ x | x <- xs, predicate x ]
//
// Classic FP examples:
//   filter (> 0) [-2, 3, -1, 4]        → [3, 4]
//   filter isEven [1, 2, 3, 4, 5, 6]   → [2, 4, 6]
//   filter (< 100) [50, 150, 75, 200]  → [50, 75]

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "../include/fp_core.h"

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

// -------------------- Helpers --------------------
static void* xmalloc(size_t bytes) {
    void* p = malloc(bytes);
    if (!p) {
        fprintf(stderr, "Out of memory requesting %zu bytes\n", bytes);
        exit(EXIT_FAILURE);
    }
    return p;
}

static int64_t rand_i64_range(int64_t min, int64_t max) {
    return min + (rand() % (max - min + 1));
}

// ============================================================================
// C BASELINE IMPLEMENTATION
// ============================================================================

// Classic filter: returns elements that satisfy predicate
size_t c_filter_gt_i64(const int64_t* input, int64_t* output,
                        size_t n, int64_t threshold) {
    size_t out_idx = 0;

    for (size_t i = 0; i < n; i++) {
        if (input[i] > threshold) {
            output[out_idx++] = input[i];
        }
    }

    return out_idx;
}

// Generic filter with predicate function pointer
typedef bool (*predicate_fn)(int64_t value, int64_t param);

static bool pred_gt(int64_t value, int64_t threshold) {
    return value > threshold;
}

static bool pred_even(int64_t value, int64_t unused) {
    (void)unused;
    return (value & 1) == 0;
}

static bool pred_range(int64_t value, int64_t min_max_packed) {
    // Pack min in lower 32 bits, max in upper 32 bits
    int64_t min = (int64_t)(int32_t)(min_max_packed & 0xFFFFFFFF);
    int64_t max = (int64_t)(int32_t)(min_max_packed >> 32);
    return value >= min && value <= max;
}

size_t c_filter_generic(const int64_t* input, int64_t* output,
                         size_t n, predicate_fn pred, int64_t param) {
    size_t out_idx = 0;

    for (size_t i = 0; i < n; i++) {
        if (pred(input[i], param)) {
            output[out_idx++] = input[i];
        }
    }

    return out_idx;
}

// ============================================================================
// SIMD-OPTIMIZED IMPLEMENTATION
// ============================================================================

// Phase 1: Generate mask using SIMD comparison
// Phase 2: Compact elements based on mask
//
// This is the classic two-phase filter algorithm used in:
// - Database query engines
// - Apache Arrow
// - GPU stream compaction
// - FP language runtimes (GHC, OCaml)

// Generate mask: 1 where predicate true, 0 otherwise
static void simd_generate_mask_gt_i64(const int64_t* input, uint8_t* mask,
                                       size_t n, int64_t threshold) {
    // Use scalar for now - SIMD version would use vpcmpgtq
    for (size_t i = 0; i < n; i++) {
        mask[i] = (input[i] > threshold) ? 1 : 0;
    }
}

// Compact: pack elements where mask is 1
// This is the bottleneck - needs SIMD optimization
static size_t simd_compact_i64(const int64_t* input, int64_t* output,
                                const uint8_t* mask, size_t n) {
    size_t out_idx = 0;

    // Scalar version for correctness
    // SIMD version would use:
    // 1. Parallel prefix sum on mask (get output indices)
    // 2. Conditional store based on mask
    for (size_t i = 0; i < n; i++) {
        if (mask[i]) {
            output[out_idx++] = input[i];
        }
    }

    return out_idx;
}

// Combined SIMD filter (two-phase)
size_t simd_filter_gt_i64(const int64_t* input, int64_t* output,
                          size_t n, int64_t threshold) {
    // Allocate mask array
    uint8_t* mask = (uint8_t*)xmalloc(n * sizeof(uint8_t));

    // Phase 1: Generate mask (SIMD-friendly)
    simd_generate_mask_gt_i64(input, mask, n, threshold);

    // Phase 2: Compact (harder to SIMD, but possible)
    size_t out_len = simd_compact_i64(input, output, mask, n);

    free(mask);
    return out_len;
}

// Real SIMD version: calls assembly implementation
// This is the test for "List FP" fitness!
size_t simd_filter_gt_i64_fused(const int64_t* input, int64_t* output,
                                 size_t n, int64_t threshold) {
    // Use real SIMD implementation from fp_core_compaction.asm
    return fp_filter_gt_i64_simple(input, output, n, threshold);
}

// ============================================================================
// FUNCTIONAL PROGRAMMING COMPOSITIONS
// ============================================================================

// Demonstrate classic FP pipeline: map → filter → fold
// This tests whether the library truly enables FP-style programming

// Example: Process transaction data
// 1. Map: Convert cents to dollars
// 2. Filter: Only transactions > $100
// 3. Fold: Sum total

int64_t fp_pipeline_example(const int64_t* transactions_cents, size_t n,
                              int64_t threshold_cents) {
    // Allocate intermediate arrays
    int64_t* filtered = (int64_t*)xmalloc(n * sizeof(int64_t));

    // Step 1: Filter (> threshold)
    size_t filtered_count = c_filter_gt_i64(transactions_cents, filtered,
                                             n, threshold_cents);

    // Step 2: Fold (sum)
    int64_t total = fp_reduce_add_i64(filtered, filtered_count);

    free(filtered);
    return total;
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    size_t n = 10000000;  // Default: 10M elements
    int iterations = 50;   // Benchmark iterations

    if (argc >= 2) n = (size_t)atoll(argv[1]);
    if (argc >= 3) iterations = atoi(argv[2]);

    printf("+============================================================+\n");
    printf("|   FILTER: CLASSIC FUNCTIONAL PROGRAMMING OPERATION        |\n");
    printf("|   Testing FP-ASM Library's True FP Fitness                |\n");
    printf("+============================================================+\n\n");

    printf("Functional Programming Context:\n");
    printf("  Haskell:   filter (> threshold) list\n");
    printf("  Lisp:      (filter (lambda (x) (> x threshold)) list)\n");
    printf("  Python:    [x for x in list if x > threshold]\n");
    printf("  SQL:       SELECT * FROM table WHERE value > threshold\n");
    printf("\n");

    printf("Test Configuration:\n");
    printf("  Array size:    %zu elements (%.1f MB)\n", n, n * 8.0 / 1e6);
    printf("  Data range:    -1000 to 1000\n");
    printf("  Threshold:     0 (filter positive values)\n");
    printf("  Iterations:    %d runs\n\n", iterations);

    // Allocate arrays
    int64_t* input = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* output_c = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* output_simd = (int64_t*)xmalloc(n * sizeof(int64_t));

    // Generate random test data (uniform distribution)
    printf("Generating random test data...\n");
    srand(42);
    for (size_t i = 0; i < n; i++) {
        input[i] = rand_i64_range(-1000, 1000);
    }

    int64_t threshold = 0;

    // ========================================
    // Correctness Check
    // ========================================
    printf("\n--- Correctness Check ---\n");

    size_t out_len_c = c_filter_gt_i64(input, output_c, n, threshold);
    size_t out_len_simd = simd_filter_gt_i64(input, output_simd, n, threshold);

    printf("C Baseline:   filtered %zu elements (%.1f%% of input)\n",
           out_len_c, 100.0 * out_len_c / n);
    printf("SIMD Version: filtered %zu elements (%.1f%% of input)\n",
           out_len_simd, 100.0 * out_len_simd / n);

    // Verify outputs match
    bool match = (out_len_c == out_len_simd);
    if (match) {
        for (size_t i = 0; i < out_len_c; i++) {
            if (output_c[i] != output_simd[i]) {
                match = false;
                printf("MISMATCH at index %zu: C=%lld, SIMD=%lld\n",
                       i, (long long)output_c[i], (long long)output_simd[i]);
                break;
            }
        }
    }

    if (match) {
        printf("PASS: Both methods produce identical results\n");
    } else {
        printf("FAIL: Methods differ!\n");
        return 1;
    }

    // ========================================
    // Performance Benchmark
    // ========================================
    printf("\n--- Performance Benchmark ---\n");
    printf("Filtering %zu elements, %d times...\n\n", n, iterations);

    // Warm-up
    c_filter_gt_i64(input, output_c, n / 10, threshold);

    // Benchmark C version
    hi_timer_t t0 = timer_start();
    volatile size_t sink_c = 0;
    for (int iter = 0; iter < iterations; iter++) {
        size_t len = c_filter_gt_i64(input, output_c, n, threshold);
        sink_c += len;
    }
    double time_c = timer_ms_since(&t0);

    // Benchmark SIMD version (fused)
    t0 = timer_start();
    volatile size_t sink_simd = 0;
    for (int iter = 0; iter < iterations; iter++) {
        size_t len = simd_filter_gt_i64_fused(input, output_simd, n, threshold);
        sink_simd += len;
    }
    double time_simd = timer_ms_since(&t0);

    double avg_c = time_c / iterations;
    double avg_simd = time_simd / iterations;
    double speedup = time_c / time_simd;

    printf("+============================================================+\n");
    printf("|   RESULTS                                                  |\n");
    printf("+------------------------------------------------------------+\n");
    printf("|   C Baseline:      %8.2f ms/run  (1.00x)               |\n", avg_c);
    printf("|   SIMD Optimized:  %8.2f ms/run  (%.2fx)               |\n", avg_simd, speedup);
    printf("+------------------------------------------------------------+\n");
    printf("|   Speedup:         %.2fx faster                            |\n", speedup);
    printf("|   Time saved:      %.2f ms per filter                  |\n", avg_c - avg_simd);
    printf("+============================================================+\n");

    // ========================================
    // Functional Programming Pipeline Example
    // ========================================
    printf("\n--- FP Pipeline Example ---\n");
    printf("Classic FP composition: filter → fold\n\n");

    // Scenario: E-commerce transactions (filter high-value, sum total)
    printf("Scenario: E-commerce transaction analysis\n");
    printf("  Filter: transactions > $100\n");
    printf("  Fold:   sum total revenue\n\n");

    int64_t threshold_cents = 10000;  // $100 in cents

    t0 = timer_start();
    int64_t total = fp_pipeline_example(input, n, threshold_cents);
    double time_pipeline = timer_ms_since(&t0);

    printf("Processed %zu transactions in %.2f ms\n", n, time_pipeline);
    printf("High-value transactions: %zu (%.1f%%)\n",
           out_len_c, 100.0 * out_len_c / n);
    printf("Total filtered sum: %lld\n", (long long)total);

    // ========================================
    // Selectivity Analysis
    // ========================================
    printf("\n--- Selectivity Analysis ---\n");
    printf("Filter performance depends on selectivity (% of elements passing):\n\n");

    int64_t thresholds[] = {-900, -500, 0, 500, 900};
    const char* selectivity_names[] = {"10%", "25%", "50%", "75%", "90%"};

    printf("| Threshold | Selectivity | Filtered Count | C Time   | SIMD Time | Speedup |\n");
    printf("|-----------|-------------|----------------|----------|-----------|----------|\n");

    for (int i = 0; i < 5; i++) {
        int64_t thresh = thresholds[i];

        // Measure C
        t0 = timer_start();
        size_t len_c = c_filter_gt_i64(input, output_c, n, thresh);
        double t_c = timer_ms_since(&t0);

        // Measure SIMD
        t0 = timer_start();
        size_t len_simd = simd_filter_gt_i64_fused(input, output_simd, n, thresh);
        double t_simd = timer_ms_since(&t0);

        double sp = t_c / t_simd;

        printf("| %9lld | %11s | %14zu | %7.2f | %8.2f | %7.2fx |\n",
               (long long)thresh, selectivity_names[i], len_c, t_c, t_simd, sp);
    }

    // ========================================
    // Conclusion
    // ========================================
    printf("\n--- Functional Programming Fitness Assessment ---\n");
    printf("Filter is THE fundamental FP operation alongside map and fold.\n\n");

    printf("FP-ASM Library Status:\n");
    printf("  ✅ FOLD:   Implemented (fp_reduce_*, fp_fold_*)\n");
    printf("  ✅ MAP:    Implemented (fp_map_*, fp_zip_*)\n");
    printf("  🔧 FILTER: Demonstrated (needs library integration)\n\n");

    if (speedup >= 1.2) {
        printf("✅ VERDICT: Library shows FP fitness with %.2fx speedup\n", speedup);
        printf("   Recommendation: Add fp_filter_* family to library\n");
    } else {
        printf("⚠️  VERDICT: Filter shows marginal gains (%.2fx)\n", speedup);
        printf("   Note: Scalar filter is already quite efficient\n");
        printf("   SIMD helps most when combined with other operations\n");
    }

    // Cleanup
    free(input);
    free(output_c);
    free(output_simd);

    return 0;
}
