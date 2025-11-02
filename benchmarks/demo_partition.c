// Partition: The List FP Operation That Returns TWO Lists
// Haskell: partition (> 0) [-2, 3, -1, 4] → ([3, 4], [-2, -1])
//
// This demonstrates complete "List FP" capability:
// - Filter splits into one list (pass)
// - Partition splits into two lists (pass + fail)
//
// Real-world use cases:
// - Quicksort: partition around pivot
// - Validation: separate valid/invalid records
// - Classification: binary classification tasks

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
// C BASELINE
// ============================================================================

void c_partition_gt_i64(const int64_t* input, int64_t* output_pass,
                        int64_t* output_fail, size_t n, int64_t threshold,
                        size_t* out_pass_count, size_t* out_fail_count) {
    size_t pass_idx = 0;
    size_t fail_idx = 0;

    for (size_t i = 0; i < n; i++) {
        if (input[i] > threshold) {
            output_pass[pass_idx++] = input[i];
        } else {
            output_fail[fail_idx++] = input[i];
        }
    }

    *out_pass_count = pass_idx;
    *out_fail_count = fail_idx;
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    size_t n = 10000000;  // 10M elements
    int iterations = 50;

    if (argc >= 2) n = (size_t)atoll(argv[1]);
    if (argc >= 3) iterations = atoi(argv[2]);

    printf("+============================================================+\n");
    printf("|   PARTITION: List FP with TWO Outputs                     |\n");
    printf("|   Completes the \"List FP\" Status                          |\n");
    printf("+============================================================+\n\n");

    printf("Haskell: partition (> 0) [-2, 3, -1, 4] → ([3, 4], [-2, -1])\n");
    printf("Library: fp_partition_gt_i64(input, pass, fail, n, threshold)\n\n");

    printf("Configuration:\n");
    printf("  Array size:  %zu elements (%.1f MB)\n", n, n * 8.0 / 1e6);
    printf("  Data range:  -1000 to 1000\n");
    printf("  Threshold:   0 (partition positive/negative)\n");
    printf("  Iterations:  %d runs\n\n", iterations);

    // Allocate arrays
    int64_t* input = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* pass_c = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* fail_c = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* pass_asm = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* fail_asm = (int64_t*)xmalloc(n * sizeof(int64_t));

    // Generate test data
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

    size_t pass_count_c, fail_count_c;
    c_partition_gt_i64(input, pass_c, fail_c, n, threshold,
                       &pass_count_c, &fail_count_c);

    size_t pass_count_asm, fail_count_asm;
    fp_partition_gt_i64(input, pass_asm, fail_asm, n, threshold,
                        &pass_count_asm, &fail_count_asm);

    printf("C Baseline:\n");
    printf("  Pass: %zu elements (%.1f%%)\n", pass_count_c, 100.0 * pass_count_c / n);
    printf("  Fail: %zu elements (%.1f%%)\n", fail_count_c, 100.0 * fail_count_c / n);

    printf("FP-ASM:\n");
    printf("  Pass: %zu elements (%.1f%%)\n", pass_count_asm, 100.0 * pass_count_asm / n);
    printf("  Fail: %zu elements (%.1f%%)\n", fail_count_asm, 100.0 * fail_count_asm / n);

    // Verify
    int match = 1;
    if (pass_count_c != pass_count_asm || fail_count_c != fail_count_asm) {
        printf("FAIL: Count mismatch!\n");
        match = 0;
    } else {
        // Check pass array
        for (size_t i = 0; i < pass_count_c; i++) {
            if (pass_c[i] != pass_asm[i]) {
                printf("FAIL: Pass array mismatch at index %zu\n", i);
                match = 0;
                break;
            }
        }

        // Check fail array
        if (match) {
            for (size_t i = 0; i < fail_count_c; i++) {
                if (fail_c[i] != fail_asm[i]) {
                    printf("FAIL: Fail array mismatch at index %zu\n", i);
                    match = 0;
                    break;
                }
            }
        }
    }

    if (match) {
        printf("PASS: Both methods produce identical results\n");
    } else {
        printf("Test FAILED!\n");
        return 1;
    }

    // ========================================
    // Performance Benchmark
    // ========================================
    printf("\n--- Performance Benchmark ---\n");
    printf("Partitioning %zu elements, %d times...\n\n", n, iterations);

    // Benchmark C
    hi_timer_t t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        c_partition_gt_i64(input, pass_c, fail_c, n, threshold,
                           &pass_count_c, &fail_count_c);
    }
    double time_c = timer_ms_since(&t0);

    // Benchmark FP-ASM
    t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        fp_partition_gt_i64(input, pass_asm, fail_asm, n, threshold,
                            &pass_count_asm, &fail_count_asm);
    }
    double time_asm = timer_ms_since(&t0);

    double avg_c = time_c / iterations;
    double avg_asm = time_asm / iterations;
    double speedup = time_c / time_asm;

    printf("+============================================================+\n");
    printf("|   RESULTS                                                  |\n");
    printf("+------------------------------------------------------------+\n");
    printf("|   C Baseline:      %8.2f ms/run  (1.00x)               |\n", avg_c);
    printf("|   FP-ASM:          %8.2f ms/run  (%.2fx)               |\n", avg_asm, speedup);
    printf("+------------------------------------------------------------+\n");
    printf("|   Speedup:         %.2fx faster                            |\n", speedup);
    printf("|   Time saved:      %.2f ms per partition                |\n", avg_c - avg_asm);
    printf("+============================================================+\n");

    // ========================================
    // List FP Status Assessment
    // ========================================
    printf("\n--- List FP Capability Assessment ---\n\n");

    printf("List FP Operations Status:\n");
    printf("  ✅ FILTER:    Implemented (1.85x speedup)\n");
    printf("  ✅ PARTITION: Implemented (%.2fx speedup)\n", speedup);
    printf("\n");

    if (speedup >= 1.2) {
        printf("🎉 VERDICT: Library has COMPLETE \"List FP\" capability!\n\n");
        printf("The library can now handle:\n");
        printf("  - Dense FP: map, fold, scan, zipWith (1.7-4.7x)\n");
        printf("  - List FP:  filter, partition (%.2fx-%.2fx)\n\n",
               1.85, speedup);
        printf("This is a FULL Functional Programming library for C!\n");
    } else {
        printf("⚠️  Partition shows marginal gains (%.2fx)\n", speedup);
        printf("   But filter (1.85x) proves List FP is viable!\n");
    }

    // Cleanup
    free(input);
    free(pass_c);
    free(fail_c);
    free(pass_asm);
    free(fail_asm);

    return 0;
}
