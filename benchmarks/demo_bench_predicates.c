// FP-ASM-DemoBench-Predicates
// Tests the "Predicates" module (Module 6) - Boolean operations.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
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

// -------------------- Helpers ------------------------------
static void* xmalloc(size_t bytes) {
    void* p = malloc(bytes);
    if (!p) {
        fprintf(stderr, "Out of memory requesting %zu bytes\n", bytes);
        exit(EXIT_FAILURE);
    }
    return p;
}

// -------------------- C Reference Implementations --------------------
static bool c_pred_all_eq_const_i64(const int64_t* arr, size_t n, int64_t value) {
    for (size_t i = 0; i < n; i++) {
        if (arr[i] != value) return false;
    }
    return true;
}

static bool c_pred_any_gt_const_i64(const int64_t* arr, size_t n, int64_t value) {
    for (size_t i = 0; i < n; i++) {
        if (arr[i] > value) return true;
    }
    return false;
}

static bool c_pred_all_gt_zip_i64(const int64_t* a, const int64_t* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] <= b[i]) return false;
    }
    return true;
}

// -------------------- Main --------------------
int main(int argc, char** argv) {
    size_t n = 10000000;  // 10M elements default
    int iterations = 100;

    if (argc >= 2) n = (size_t)atoll(argv[1]);
    if (argc >= 3) iterations = atoi(argv[2]);

    printf("Benchmark: Predicates (Module 6)\n");
    printf("Array Size: n=%zu elements, Iterations: %d\n\n", n, iterations);

    // Allocate arrays
    int64_t* arr_all_eq = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* arr_any_gt = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* arr_a = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* arr_b = (int64_t*)xmalloc(n * sizeof(int64_t));

    // Initialize test arrays
    for (size_t i = 0; i < n; i++) {
        arr_all_eq[i] = 42;              // All equal to 42
        arr_any_gt[i] = (int64_t)i - 50; // Mix of values
        arr_a[i] = (int64_t)i + 10;      // a[i] > b[i]
        arr_b[i] = (int64_t)i;
    }

    // ========================================
    // Correctness Checks
    // ========================================
    printf("Running Correctness Checks...\n");

    // --- Test 1: all_eq with array of all 42s ---
    bool c_result = c_pred_all_eq_const_i64(arr_all_eq, n, 42);
    bool asm_result = fp_pred_all_eq_const_i64(arr_all_eq, n, 42);
    if (c_result == asm_result && asm_result == true) {
        printf("[PASS] all_eq (all match)\n");
    } else {
        printf("[FAIL] all_eq (all match): C=%d, ASM=%d, expected=1\n", c_result, asm_result);
        return 1;
    }

    // --- Test 2: all_eq with mismatch at end ---
    arr_all_eq[n-1] = 99;  // Break pattern
    c_result = c_pred_all_eq_const_i64(arr_all_eq, n, 42);
    asm_result = fp_pred_all_eq_const_i64(arr_all_eq, n, 42);
    if (c_result == asm_result && asm_result == false) {
        printf("[PASS] all_eq (mismatch)\n");
    } else {
        printf("[FAIL] all_eq (mismatch): C=%d, ASM=%d, expected=0\n", c_result, asm_result);
        return 1;
    }
    arr_all_eq[n-1] = 42;  // Restore

    // --- Test 3: any_gt with values > threshold ---
    c_result = c_pred_any_gt_const_i64(arr_any_gt, n, 1000);
    asm_result = fp_pred_any_gt_const_i64(arr_any_gt, n, 1000);
    if (c_result == asm_result && asm_result == true) {
        printf("[PASS] any_gt (found)\n");
    } else {
        printf("[FAIL] any_gt (found): C=%d, ASM=%d, expected=1\n", c_result, asm_result);
        return 1;
    }

    // --- Test 4: any_gt with no values > threshold ---
    c_result = c_pred_any_gt_const_i64(arr_any_gt, n, (int64_t)n * 2);
    asm_result = fp_pred_any_gt_const_i64(arr_any_gt, n, (int64_t)n * 2);
    if (c_result == asm_result && asm_result == false) {
        printf("[PASS] any_gt (not found)\n");
    } else {
        printf("[FAIL] any_gt (not found): C=%d, ASM=%d, expected=0\n", c_result, asm_result);
        return 1;
    }

    // --- Test 5: all_gt_zip with a[i] > b[i] ---
    c_result = c_pred_all_gt_zip_i64(arr_a, arr_b, n);
    asm_result = fp_pred_all_gt_zip_i64(arr_a, arr_b, n);
    if (c_result == asm_result && asm_result == true) {
        printf("[PASS] all_gt_zip (all greater)\n");
    } else {
        printf("[FAIL] all_gt_zip (all greater): C=%d, ASM=%d, expected=1\n", c_result, asm_result);
        return 1;
    }

    // --- Test 6: all_gt_zip with violation ---
    arr_a[n/2] = arr_b[n/2];  // Make one equal (not greater)
    c_result = c_pred_all_gt_zip_i64(arr_a, arr_b, n);
    asm_result = fp_pred_all_gt_zip_i64(arr_a, arr_b, n);
    if (c_result == asm_result && asm_result == false) {
        printf("[PASS] all_gt_zip (violation)\n");
    } else {
        printf("[FAIL] all_gt_zip (violation): C=%d, ASM=%d, expected=0\n", c_result, asm_result);
        return 1;
    }
    arr_a[n/2] = arr_b[n/2] + 10;  // Restore

    printf("\nAll checks passed. Proceeding to benchmarks...\n\n");

    // ========================================
    // Benchmarks
    // ========================================

    volatile int sink = 0;

    // --- Benchmark: all_eq_const (true case) ---
    printf("== all_eq_const (all match) ==\n");

    hi_timer_t t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        sink += c_pred_all_eq_const_i64(arr_all_eq, n, 42);
    }
    double elapsed_c_all_eq = timer_ms_since(&t0);

    t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        sink += fp_pred_all_eq_const_i64(arr_all_eq, n, 42);
    }
    double elapsed_asm_all_eq = timer_ms_since(&t0);

    double speedup_all_eq = elapsed_c_all_eq / elapsed_asm_all_eq;
    printf("C   : %8.3f ms   (1.00x)\n", elapsed_c_all_eq);
    printf("ASM : %8.3f ms   (%.2fx)\n", elapsed_asm_all_eq, speedup_all_eq);
    printf("\n");

    // --- Benchmark: any_gt_const (early exit) ---
    printf("== any_gt_const (early match) ==\n");

    // Put match near beginning for early exit test
    arr_any_gt[10] = 100000;

    t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        sink += c_pred_any_gt_const_i64(arr_any_gt, n, 50000);
    }
    double elapsed_c_any_gt = timer_ms_since(&t0);

    t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        sink += fp_pred_any_gt_const_i64(arr_any_gt, n, 50000);
    }
    double elapsed_asm_any_gt = timer_ms_since(&t0);

    double speedup_any_gt = elapsed_c_any_gt / elapsed_asm_any_gt;
    printf("C   : %8.3f ms   (1.00x)\n", elapsed_c_any_gt);
    printf("ASM : %8.3f ms   (%.2fx)\n", elapsed_asm_any_gt, speedup_any_gt);
    printf("\n");

    // --- Benchmark: all_gt_zip (true case) ---
    printf("== all_gt_zip (all greater) ==\n");

    t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        sink += c_pred_all_gt_zip_i64(arr_a, arr_b, n);
    }
    double elapsed_c_all_gt_zip = timer_ms_since(&t0);

    t0 = timer_start();
    for (int iter = 0; iter < iterations; iter++) {
        sink += fp_pred_all_gt_zip_i64(arr_a, arr_b, n);
    }
    double elapsed_asm_all_gt_zip = timer_ms_since(&t0);

    double speedup_all_gt_zip = elapsed_c_all_gt_zip / elapsed_asm_all_gt_zip;
    printf("C   : %8.3f ms   (1.00x)\n", elapsed_c_all_gt_zip);
    printf("ASM : %8.3f ms   (%.2fx)\n", elapsed_asm_all_gt_zip, speedup_all_gt_zip);
    printf("\n");

    // Cleanup
    free(arr_all_eq);
    free(arr_any_gt);
    free(arr_a);
    free(arr_b);

    printf("Done. (sink=%d)\n", sink);
    return 0;
}
