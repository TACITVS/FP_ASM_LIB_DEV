/* =============================================================================
 * File:        test_zip_timed.c
 * Version:     1.0
 * Project:     FP-ASM Core Library - Module 3 (Fused Maps)
 * Path:        /test/test_zip_timed.c
 * 
 * Description: Isolated timing test for fp_zip_add_i64 function with large
 *              arrays (100M elements). Measures execution time of both C
 *              reference and ASM implementation to diagnose performance
 *              issues or crashes. Provides detailed stage-by-stage output
 *              with fflush() to pinpoint exact failure location.
 * 
 * Purpose:     Debugging tool to isolate zip_add_i64 behavior when called
 *              with very large arrays that cause crashes in full benchmark.
 * 
 * Dependencies: 
 *   - fp_core.h (function declarations)
 *   - fp_core_fused_maps.o (ASM implementations)
 *   - windows.h (for high-resolution timing)
 * 
 * Build:       gcc -O3 test_zip_timed.c fp_core_fused_maps.o -o test_timed.exe
 * Usage:       test_timed.exe
 * 
 * Expected Output:
 *   - Stage 1-5 completion messages with timestamps
 *   - C version timing (~XXX ms)
 *   - ASM version timing (~XXX ms, should be faster)
 *   - Correctness verification result
 * 
 * Author:      FP-ASM Debug Tools
 * Date:        2025-01-XX
 * ============================================================================= */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#include "../include/fp_core.h"

/* High-resolution timer using Windows QueryPerformanceCounter */
static double get_time_ms(void) {
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER now;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart * 1000.0 / (double)freq.QuadPart;
}

int main(void) {
    const size_t n = 100000000;  // 100 million elements
    double t0, t1;
    
    printf("=============================================================================\n");
    printf(" FP-ASM Isolated Zip Add Test (n=%zu, %.1f MB per array)\n", n, (n * 8.0) / (1024*1024));
    printf("=============================================================================\n\n");
    
    printf("Stage 1: Allocating arrays...\n");
    fflush(stdout);
    int64_t* a = malloc(n * sizeof(int64_t));
    int64_t* b = malloc(n * sizeof(int64_t));
    int64_t* out_c = malloc(n * sizeof(int64_t));
    int64_t* out_asm = malloc(n * sizeof(int64_t));
    
    if (!a || !b || !out_c || !out_asm) {
        printf("ERROR: Memory allocation failed!\n");
        return 1;
    }
    printf("  Success: Allocated 4 arrays of %zu elements each\n\n", n);
    
    printf("Stage 2: Initializing arrays...\n");
    fflush(stdout);
    for (size_t i = 0; i < n; i++) {
        a[i] = (int64_t)(i % 100) - 50;
        b[i] = (int64_t)(i % 50) - 25;
    }
    printf("  Success: Arrays initialized\n");
    printf("  Sample values: a[0]=%lld, b[0]=%lld, expected sum=%lld\n\n", 
           (long long)a[0], (long long)b[0], (long long)(a[0] + b[0]));
    
    printf("Stage 3: Running C reference implementation...\n");
    fflush(stdout);
    t0 = get_time_ms();
    for (size_t i = 0; i < n; i++) {
        out_c[i] = a[i] + b[i];
    }
    t1 = get_time_ms();
    printf("  SUCCESS: C version completed in %.3f ms\n", t1 - t0);
    printf("  Result: out_c[0]=%lld\n\n", (long long)out_c[0]);
    
    printf("Stage 4: Running ASM implementation...\n");
    printf("  About to call fp_zip_add_i64(a, b, out_asm, %zu)...\n", n);
    fflush(stdout);
    t0 = get_time_ms();
    fp_zip_add_i64(a, b, out_asm, n);
    t1 = get_time_ms();
    printf("  SUCCESS: ASM version completed in %.3f ms\n", t1 - t0);
    printf("  Result: out_asm[0]=%lld\n\n", (long long)out_asm[0]);
    
    printf("Stage 5: Verifying correctness...\n");
    fflush(stdout);
    t0 = get_time_ms();
    size_t errors = 0;
    for (size_t i = 0; i < n; i++) {
        if (out_c[i] != out_asm[i]) {
            if (errors < 10) {  // Only print first 10 errors
                printf("  MISMATCH at index %zu: C=%lld, ASM=%lld\n", 
                       i, (long long)out_c[i], (long long)out_asm[i]);
            }
            errors++;
        }
    }
    t1 = get_time_ms();
    
    if (errors == 0) {
        printf("  SUCCESS: All %zu elements match! (verified in %.3f ms)\n", n, t1 - t0);
    } else {
        printf("  FAILED: Found %zu mismatches\n", errors);
    }
    
    printf("\n=============================================================================\n");
    printf(" Test Summary:\n");
    printf("   Arrays:      4 x %zu elements\n", n);
    printf("   Status:      %s\n", errors == 0 ? "PASS" : "FAIL");
    printf("=============================================================================\n");
    
    free(a); free(b); free(out_c); free(out_asm);
    return errors == 0 ? 0 : 1;
}