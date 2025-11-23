// FP-ASM Benchmark: Conditional Reduction
//
// Benchmarks fp_reduce_add_f64_where() - masked summation primitive
//
// Test scenarios:
//   1. Correctness verification against baseline C
//   2. Performance comparison: ASM vs C implementation
//   3. Mask density analysis: sparse, dense, alternating patterns
//   4. Array size scaling: 1K to 10M elements
//   5. Comparison with fp_reduce_add_f64 (overhead measurement)
//
// PLATFORM: Windows only (uses QueryPerformanceCounter for high-precision timing)
// This benchmark requires Windows x64. It is not intended for cross-platform use.
// The FP-ASM library currently targets Windows x64 with NASM and MinGW-w64 GCC.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef _WIN32
#include <windows.h>
#else
#error "This benchmark requires Windows (uses QueryPerformanceCounter). The FP-ASM library is Windows-only."
#endif

#include <math.h>
#include "../include/fp_core.h"

// ============================================================================
// Timer Infrastructure
// ============================================================================

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

// ============================================================================
// Memory Allocation Helper
// ============================================================================

static void* xmalloc(size_t bytes) {
    void* p = malloc(bytes);
    if (!p) {
        fprintf(stderr, "Out of memory requesting %zu bytes\n", bytes);
        exit(EXIT_FAILURE);
    }
    return p;
}

// ============================================================================
// Baseline C Implementation
// ============================================================================

static double c_reduce_add_f64_where(const double* x, const int* mask, size_t n) {
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        if (mask[i] != 0) {
            sum += x[i];
        }
    }
    return sum;
}

// ============================================================================
// Mask Generation Utilities
// ============================================================================

typedef enum {
    MASK_ALL_ONES,      // 100% density - all elements included
    MASK_ALL_ZEROS,     // 0% density - no elements included
    MASK_ALTERNATING,   // 50% density - every other element
    MASK_SPARSE_10,     // 10% density - every 10th element
    MASK_SPARSE_1,      // 1% density - every 100th element
    MASK_RANDOM_50,     // ~50% density - random pattern
    MASK_RANDOM_25,     // ~25% density - random pattern
    MASK_FIRST_HALF,    // First half included
    MASK_LAST_HALF      // Last half included
} MaskPattern;

static const char* mask_pattern_name(MaskPattern p) {
    switch (p) {
        case MASK_ALL_ONES:     return "100% (all)";
        case MASK_ALL_ZEROS:    return "0% (none)";
        case MASK_ALTERNATING:  return "50% (alt)";
        case MASK_SPARSE_10:    return "10% (sparse)";
        case MASK_SPARSE_1:     return "1% (v.sparse)";
        case MASK_RANDOM_50:    return "~50% (rand)";
        case MASK_RANDOM_25:    return "~25% (rand)";
        case MASK_FIRST_HALF:   return "50% (first)";
        case MASK_LAST_HALF:    return "50% (last)";
        default:                return "unknown";
    }
}

static void generate_mask(int* mask, size_t n, MaskPattern pattern, unsigned int seed) {
    srand(seed);
    switch (pattern) {
        case MASK_ALL_ONES:
            for (size_t i = 0; i < n; ++i) mask[i] = 1;
            break;
        case MASK_ALL_ZEROS:
            for (size_t i = 0; i < n; ++i) mask[i] = 0;
            break;
        case MASK_ALTERNATING:
            for (size_t i = 0; i < n; ++i) mask[i] = (i % 2 == 0) ? 1 : 0;
            break;
        case MASK_SPARSE_10:
            for (size_t i = 0; i < n; ++i) mask[i] = (i % 10 == 0) ? 1 : 0;
            break;
        case MASK_SPARSE_1:
            for (size_t i = 0; i < n; ++i) mask[i] = (i % 100 == 0) ? 1 : 0;
            break;
        case MASK_RANDOM_50:
            for (size_t i = 0; i < n; ++i) mask[i] = (rand() % 2);
            break;
        case MASK_RANDOM_25:
            for (size_t i = 0; i < n; ++i) mask[i] = (rand() % 4 == 0) ? 1 : 0;
            break;
        case MASK_FIRST_HALF:
            for (size_t i = 0; i < n; ++i) mask[i] = (i < n / 2) ? 1 : 0;
            break;
        case MASK_LAST_HALF:
            for (size_t i = 0; i < n; ++i) mask[i] = (i >= n / 2) ? 1 : 0;
            break;
    }
}

// ============================================================================
// Benchmark Configuration
// ============================================================================

#define N_DEFAULT      10000000
#define ITERS_DEFAULT  10
#define EPSILON        1e-9

// ============================================================================
// Main Benchmark
// ============================================================================

int main(int argc, char** argv) {
    size_t n = (argc > 1) ? (size_t)strtoull(argv[1], NULL, 10) : N_DEFAULT;
    int iters = (argc > 2) ? (int)strtol(argv[2], NULL, 10) : ITERS_DEFAULT;

    if (n == 0) n = N_DEFAULT;
    if (iters == 0) iters = ITERS_DEFAULT;

    printf("========================================================================\n");
    printf("  FP-ASM Benchmark: Conditional Reduction (fp_reduce_add_f64_where)\n");
    printf("========================================================================\n");
    printf("Array Size: n=%" PRIu64 " elements (%.1f MB)\n",
           (uint64_t)n, (double)(n * sizeof(double)) / (1024.0 * 1024.0));
    printf("Iterations: %d per test\n", iters);
    printf("------------------------------------------------------------------------\n\n");

    // Allocate buffers
    double* data = (double*)xmalloc(n * sizeof(double));
    int* mask = (int*)xmalloc(n * sizeof(int));

    // Initialize data with varied values
    for (size_t i = 0; i < n; ++i) {
        data[i] = (double)(i % 1000) * 0.001 - 0.5;  // Range: [-0.5, 0.499]
    }

    // Volatile sink to prevent dead-code elimination
    volatile double sink = 0.0;

    // ========================================================================
    // SECTION 1: Correctness Verification
    // ========================================================================
    printf("=== Section 1: Correctness Verification ===\n\n");

    MaskPattern test_patterns[] = {
        MASK_ALL_ONES, MASK_ALL_ZEROS, MASK_ALTERNATING,
        MASK_SPARSE_10, MASK_RANDOM_50
    };
    int num_patterns = sizeof(test_patterns) / sizeof(test_patterns[0]);
    int all_correct = 1;

    for (int p = 0; p < num_patterns; ++p) {
        generate_mask(mask, n, test_patterns[p], 42);

        double c_result = c_reduce_add_f64_where(data, mask, n);
        double asm_result = fp_reduce_add_f64_where(data, mask, n);

        double diff = fabs(c_result - asm_result);
        // Use relative error for large values
        double rel_err = (fabs(c_result) > 1.0) ? diff / fabs(c_result) : diff;

        if (rel_err < EPSILON) {
            printf("[PASS] Pattern %-14s: C=%.6f, ASM=%.6f\n",
                   mask_pattern_name(test_patterns[p]), c_result, asm_result);
        } else {
            printf("[FAIL] Pattern %-14s: C=%.6f, ASM=%.6f (diff=%.2e)\n",
                   mask_pattern_name(test_patterns[p]), c_result, asm_result, diff);
            all_correct = 0;
        }
    }

    if (!all_correct) {
        printf("\nCorrectness checks FAILED. Halting.\n");
        free(data);
        free(mask);
        return EXIT_FAILURE;
    }
    printf("\nAll correctness checks passed.\n\n");

    // ========================================================================
    // SECTION 2: Performance Benchmarks by Mask Pattern
    // ========================================================================
    printf("=== Section 2: Performance by Mask Pattern ===\n\n");
    printf("%-16s %12s %12s %10s\n", "Pattern", "C (ms)", "ASM (ms)", "Speedup");
    printf("%-16s %12s %12s %10s\n", "-------", "------", "--------", "-------");

    MaskPattern bench_patterns[] = {
        MASK_ALL_ONES, MASK_ALTERNATING, MASK_SPARSE_10,
        MASK_SPARSE_1, MASK_RANDOM_50, MASK_RANDOM_25
    };
    int num_bench_patterns = sizeof(bench_patterns) / sizeof(bench_patterns[0]);

    for (int p = 0; p < num_bench_patterns; ++p) {
        generate_mask(mask, n, bench_patterns[p], 42);

        // Benchmark C implementation
        double c_ms = 0.0;
        for (int k = 0; k < iters; ++k) {
            hi_timer_t t = timer_start();
            sink += c_reduce_add_f64_where(data, mask, n);
            c_ms += timer_ms_since(&t);
        }
        c_ms /= iters;

        // Benchmark ASM implementation
        double asm_ms = 0.0;
        for (int k = 0; k < iters; ++k) {
            hi_timer_t t = timer_start();
            sink += fp_reduce_add_f64_where(data, mask, n);
            asm_ms += timer_ms_since(&t);
        }
        asm_ms /= iters;

        double speedup = c_ms / asm_ms;
        printf("%-16s %12.3f %12.3f %9.2fx\n",
               mask_pattern_name(bench_patterns[p]), c_ms, asm_ms, speedup);
    }

    // ========================================================================
    // SECTION 3: Comparison with Unconditional Reduction
    // ========================================================================
    printf("\n=== Section 3: Overhead vs Unconditional Reduction ===\n\n");

    // Generate all-ones mask for fair comparison
    generate_mask(mask, n, MASK_ALL_ONES, 42);

    // Benchmark unconditional fp_reduce_add_f64
    double uncond_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        hi_timer_t t = timer_start();
        sink += fp_reduce_add_f64(data, n);
        uncond_ms += timer_ms_since(&t);
    }
    uncond_ms /= iters;

    // Benchmark conditional with all-ones mask
    double cond_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        hi_timer_t t = timer_start();
        sink += fp_reduce_add_f64_where(data, mask, n);
        cond_ms += timer_ms_since(&t);
    }
    cond_ms /= iters;

    printf("fp_reduce_add_f64 (unconditional): %8.3f ms\n", uncond_ms);
    printf("fp_reduce_add_f64_where (all=1):   %8.3f ms\n", cond_ms);
    printf("Overhead ratio:                    %8.2fx\n", cond_ms / uncond_ms);
    printf("\nNote: Overhead expected due to mask checking. Future AVX2 optimization\n");
    printf("with masked gather/blend can reduce this overhead significantly.\n");

    // ========================================================================
    // SECTION 4: Scaling Analysis
    // ========================================================================
    printf("\n=== Section 4: Scaling Analysis ===\n\n");
    printf("%-12s %12s %12s %12s\n", "Size", "C (ms)", "ASM (ms)", "Throughput");
    printf("%-12s %12s %12s %12s\n", "----", "------", "--------", "----------");

    size_t sizes[] = {1000, 10000, 100000, 1000000, n};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    generate_mask(mask, n, MASK_ALTERNATING, 42);  // 50% density

    for (int s = 0; s < num_sizes; ++s) {
        size_t test_n = sizes[s];
        if (test_n > n) continue;

        // C timing
        double c_ms = 0.0;
        for (int k = 0; k < iters; ++k) {
            hi_timer_t t = timer_start();
            sink += c_reduce_add_f64_where(data, mask, test_n);
            c_ms += timer_ms_since(&t);
        }
        c_ms /= iters;

        // ASM timing
        double asm_ms = 0.0;
        for (int k = 0; k < iters; ++k) {
            hi_timer_t t = timer_start();
            sink += fp_reduce_add_f64_where(data, mask, test_n);
            asm_ms += timer_ms_since(&t);
        }
        asm_ms /= iters;

        // Calculate throughput (elements per millisecond)
        double throughput = (double)test_n / asm_ms;
        const char* unit = "elem/ms";
        if (throughput > 1e6) {
            throughput /= 1e6;
            unit = "M elem/ms";
        } else if (throughput > 1e3) {
            throughput /= 1e3;
            unit = "K elem/ms";
        }

        char size_str[32];
        if (test_n >= 1000000) {
            snprintf(size_str, sizeof(size_str), "%.0fM", (double)test_n / 1e6);
        } else if (test_n >= 1000) {
            snprintf(size_str, sizeof(size_str), "%.0fK", (double)test_n / 1e3);
        } else {
            snprintf(size_str, sizeof(size_str), "%zu", test_n);
        }

        printf("%-12s %12.3f %12.3f %8.1f %s\n", size_str, c_ms, asm_ms, throughput, unit);
    }

    // ========================================================================
    // Summary
    // ========================================================================
    printf("\n========================================================================\n");
    printf("  Benchmark Complete\n");
    printf("========================================================================\n");
    printf("\nKey Observations:\n");
    printf("  - Current implementation: Scalar loop with conditional branch\n");
    printf("  - Performance varies with mask density (branch prediction)\n");
    printf("  - Sparse masks may cause branch misprediction overhead\n");
    printf("  - Future optimization: AVX2 masked gather/blend for SIMD speedup\n");
    printf("\n");

    // Cleanup
    free(data);
    free(mask);

    // Prevent sink optimization
    if (sink == 0.0) printf("Sink: %.1f\n", sink);

    return EXIT_SUCCESS;
}
