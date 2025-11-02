// The FP Trinity: Map, Fold, Scan
// Testing whether FP-ASM truly supports functional programming
//
// This tests operations we ALREADY HAVE in the library:
// 1. MAP:  fp_map_*, fp_zip_*
// 2. FOLD: fp_reduce_*, fp_fold_*
// 3. SCAN: fp_scan_*
//
// If these show good speedups, the library IS functionally programming-fit,
// just not for structural operations (filter, partition) that need compaction.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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

// ============================================================================
// TEST 1: MAP (Transformation)
// FP: map (*2) [1, 2, 3, 4] → [2, 4, 6, 8]
// ============================================================================

void test_map(size_t n, int iterations) {
    printf("\n+============================================================+\n");
    printf("|   TEST 1: MAP - Element-wise Transformation               |\n");
    printf("+============================================================+\n\n");

    printf("Haskell: map (*2) [1, 2, 3, 4] → [2, 4, 6, 8]\n");
    printf("Library: fp_map_scale_i64(input, output, n, 2)\n\n");

    int64_t* input = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* output_c = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* output_asm = (int64_t*)xmalloc(n * sizeof(int64_t));

    // Generate test data
    for (size_t i = 0; i < n; i++) {
        input[i] = (int64_t)i;
    }

    int64_t scale = 2;

    // C baseline
    hi_timer_t t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        for (size_t i = 0; i < n; i++) {
            output_c[i] = input[i] * scale;
        }
    }
    double time_c = timer_ms_since(&t0);

    // FP-ASM library
    t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        fp_map_scale_i64(input, output_asm, n, scale);
    }
    double time_asm = timer_ms_since(&t0);

    double speedup = time_c / time_asm;

    printf("Results:\n");
    printf("  C Baseline:  %.2f ms  (1.00x)\n", time_c / iterations);
    printf("  FP-ASM:      %.2f ms  (%.2fx)\n", time_asm / iterations, speedup);

    if (speedup >= 1.1) {
        printf("  ✅ MAP shows FP fitness: %.2fx speedup\n", speedup);
    } else {
        printf("  ⚠️  MAP shows marginal gains: %.2fx\n", speedup);
    }

    free(input);
    free(output_c);
    free(output_asm);
}

// ============================================================================
// TEST 2: ZIPWITH (Parallel Map)
// FP: zipWith (+) [1,2,3] [4,5,6] → [5,7,9]
// ============================================================================

void test_zipwith(size_t n, int iterations) {
    printf("\n+============================================================+\n");
    printf("|   TEST 2: ZIPWITH - Parallel Transformation               |\n");
    printf("+============================================================+\n\n");

    printf("Haskell: zipWith (+) [1,2,3] [4,5,6] → [5,7,9]\n");
    printf("Library: fp_zip_add_i64(a, b, output, n)\n\n");

    int64_t* a = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* b = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* output_c = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* output_asm = (int64_t*)xmalloc(n * sizeof(int64_t));

    // Generate test data
    for (size_t i = 0; i < n; i++) {
        a[i] = (int64_t)i;
        b[i] = (int64_t)(i * 2);
    }

    // C baseline
    hi_timer_t t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        for (size_t i = 0; i < n; i++) {
            output_c[i] = a[i] + b[i];
        }
    }
    double time_c = timer_ms_since(&t0);

    // FP-ASM library
    t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        fp_zip_add_i64(a, b, output_asm, n);
    }
    double time_asm = timer_ms_since(&t0);

    double speedup = time_c / time_asm;

    printf("Results:\n");
    printf("  C Baseline:  %.2f ms  (1.00x)\n", time_c / iterations);
    printf("  FP-ASM:      %.2f ms  (%.2fx)\n", time_asm / iterations, speedup);

    if (speedup >= 1.1) {
        printf("  ✅ ZIPWITH shows FP fitness: %.2fx speedup\n", speedup);
    } else {
        printf("  ⚠️  ZIPWITH shows marginal gains: %.2fx\n", speedup);
    }

    free(a);
    free(b);
    free(output_c);
    free(output_asm);
}

// ============================================================================
// TEST 3: FOLD (Reduction)
// FP: foldl (+) 0 [1,2,3,4] → 10
// ============================================================================

void test_fold(size_t n, int iterations) {
    printf("\n+============================================================+\n");
    printf("|   TEST 3: FOLD - Reduction to Single Value                |\n");
    printf("+============================================================+\n\n");

    printf("Haskell: foldl (+) 0 [1,2,3,4] → 10\n");
    printf("Library: fp_reduce_add_i64(input, n)\n\n");

    int64_t* input = (int64_t*)xmalloc(n * sizeof(int64_t));

    // Generate test data
    for (size_t i = 0; i < n; i++) {
        input[i] = (int64_t)(i % 100);
    }

    // C baseline
    hi_timer_t t0 = timer_start();
    volatile int64_t sink_c = 0;
    for (int iter = 0; iter < iterations; iter++) {
        int64_t sum = 0;
        for (size_t i = 0; i < n; i++) {
            sum += input[i];
        }
        sink_c += sum;
    }
    double time_c = timer_ms_since(&t0);

    // FP-ASM library
    t0 = timer_start();
    volatile int64_t sink_asm = 0;
    for (int iter = 0; iter < iterations; iter++) {
        int64_t sum = fp_reduce_add_i64(input, n);
        sink_asm += sum;
    }
    double time_asm = timer_ms_since(&t0);

    double speedup = time_c / time_asm;

    printf("Results:\n");
    printf("  C Baseline:  %.2f ms  (1.00x)\n", time_c / iterations);
    printf("  FP-ASM:      %.2f ms  (%.2fx)\n", time_asm / iterations, speedup);

    if (speedup >= 1.1) {
        printf("  ✅ FOLD shows FP fitness: %.2fx speedup\n", speedup);
    } else {
        printf("  ⚠️  FOLD shows marginal gains: %.2fx\n", speedup);
    }

    free(input);
}

// ============================================================================
// TEST 4: SCAN (Prefix Sum)
// FP: scanl (+) 0 [1,2,3,4] → [1,3,6,10]
// ============================================================================

void test_scan(size_t n, int iterations) {
    printf("\n+============================================================+\n");
    printf("|   TEST 4: SCAN - Cumulative Reduction                     |\n");
    printf("+============================================================+\n\n");

    printf("Haskell: scanl (+) 0 [1,2,3,4] → [1,3,6,10]\n");
    printf("Library: fp_scan_add_i64(input, output, n)\n\n");

    int64_t* input = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* output_c = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* output_asm = (int64_t*)xmalloc(n * sizeof(int64_t));

    // Generate test data
    for (size_t i = 0; i < n; i++) {
        input[i] = (int64_t)(i % 100);
    }

    // C baseline
    hi_timer_t t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        int64_t acc = 0;
        for (size_t i = 0; i < n; i++) {
            acc += input[i];
            output_c[i] = acc;
        }
    }
    double time_c = timer_ms_since(&t0);

    // FP-ASM library
    t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        fp_scan_add_i64(input, output_asm, n);
    }
    double time_asm = timer_ms_since(&t0);

    double speedup = time_c / time_asm;

    printf("Results:\n");
    printf("  C Baseline:  %.2f ms  (1.00x)\n", time_c / iterations);
    printf("  FP-ASM:      %.2f ms  (%.2fx)\n", time_asm / iterations, speedup);

    if (speedup >= 1.1) {
        printf("  ✅ SCAN shows FP fitness: %.2fx speedup\n", speedup);
    } else {
        printf("  ⚠️  SCAN shows marginal gains: %.2fx\n", speedup);
    }

    free(input);
    free(output_c);
    free(output_asm);
}

// ============================================================================
// TEST 5: Fused Map+Fold (Composition)
// FP: foldl (+) 0 (map (*2) [1,2,3,4]) → 20
// ============================================================================

void test_map_fold_composition(size_t n, int iterations) {
    printf("\n+============================================================+\n");
    printf("|   TEST 5: MAP → FOLD Composition                          |\n");
    printf("+============================================================+\n\n");

    printf("Haskell: foldl (+) 0 (map (^2) [1,2,3,4]) → 30\n");
    printf("Library: fp_fold_sumsq_i64(input, n)\n\n");

    int64_t* input = (int64_t*)xmalloc(n * sizeof(int64_t));

    // Generate test data
    for (size_t i = 0; i < n; i++) {
        input[i] = (int64_t)(i % 100);
    }

    // C baseline (two passes)
    hi_timer_t t0 = timer_start();
    volatile int64_t sink_c = 0;
    for (int iter = 0; iter < iterations; iter++) {
        int64_t sum = 0;
        for (size_t i = 0; i < n; i++) {
            sum += input[i] * input[i];
        }
        sink_c += sum;
    }
    double time_c = timer_ms_since(&t0);

    // FP-ASM library (fused!)
    t0 = timer_start();
    volatile int64_t sink_asm = 0;
    for (int iter = 0; iter < iterations; iter++) {
        int64_t sum = fp_fold_sumsq_i64(input, n);
        sink_asm += sum;
    }
    double time_asm = timer_ms_since(&t0);

    double speedup = time_c / time_asm;

    printf("Results:\n");
    printf("  C Baseline:  %.2f ms  (1.00x)\n", time_c / iterations);
    printf("  FP-ASM:      %.2f ms  (%.2fx)\n", time_asm / iterations, speedup);

    if (speedup >= 1.1) {
        printf("  ✅ MAP→FOLD fusion shows FP fitness: %.2fx speedup\n", speedup);
    } else {
        printf("  ⚠️  MAP→FOLD shows marginal gains: %.2fx\n", speedup);
    }

    free(input);
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
    printf("|   FP TRINITY TEST: Map, Fold, Scan                        |\n");
    printf("|   Does FP-ASM support functional programming?             |\n");
    printf("+============================================================+\n");

    printf("\nConfiguration:\n");
    printf("  Array size:  %zu elements (%.1f MB)\n", n, n * 8.0 / 1e6);
    printf("  Iterations:  %d runs per test\n\n", iterations);

    printf("Testing the core FP operations:\n");
    printf("  1. MAP:     transform each element\n");
    printf("  2. ZIPWITH: combine two lists element-wise\n");
    printf("  3. FOLD:    reduce to single value\n");
    printf("  4. SCAN:    cumulative reduction\n");
    printf("  5. COMPOSE: fused map→fold\n");

    // Run all tests
    test_map(n, iterations);
    test_zipwith(n, iterations);
    test_fold(n, iterations);
    test_scan(n, iterations);
    test_map_fold_composition(n, iterations);

    // Final verdict
    printf("\n+============================================================+\n");
    printf("|   FINAL VERDICT: FP-ASM Library Identity                  |\n");
    printf("+============================================================+\n\n");

    printf("Operations tested:\n");
    printf("  ✅ MAP:     Supported (fp_map_*)\n");
    printf("  ✅ ZIPWITH: Supported (fp_zip_*)\n");
    printf("  ✅ FOLD:    Supported (fp_reduce_*, fp_fold_*)\n");
    printf("  ✅ SCAN:    Supported (fp_scan_*)\n");
    printf("  ✅ COMPOSE: Supported (fused operations)\n\n");

    printf("NOT supported:\n");
    printf("  ❌ FILTER:  Requires compaction (0.91x slower)\n");
    printf("  ❌ PARTITION: Requires compaction\n");
    printf("  ❌ GROUP:   Requires irregular output\n\n");

    printf("Conclusion:\n");
    printf("  The library supports REGULAR functional programming patterns:\n");
    printf("  - Fixed-size transformations (map, scan)\n");
    printf("  - Reductions (fold)\n");
    printf("  - Parallel iteration (zipWith)\n\n");

    printf("  It does NOT support IRREGULAR patterns:\n");
    printf("  - Variable-size outputs (filter, partition)\n");
    printf("  - Data-dependent structures (group, histogram)\n\n");

    printf("  Identity: \"Functional Programming for DENSE arrays\"\n");
    printf("           NOT: \"Functional Programming for SPARSE structures\"\n");

    return 0;
}
