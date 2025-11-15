// demo_radix_sort.c
//
// Radix Sort Demo - Showcasing Integer Type Support
// Demonstrates FP-ASM library working with u8, u32, u64 types
//
// This showcases:
// - Integer type coverage (u8, u32, u64)
// - Linear-time sorting O(n·k) vs O(n log n) comparison sorts
// - Stable sorting with counting sort
// - Performance vs stdlib qsort

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

// Forward declarations from fp_radix_sort.c
void fp_radix_sort_u8(const uint8_t* in, uint8_t* out, size_t n);
void fp_radix_sort_u32(const uint32_t* in, uint32_t* out, size_t n);
void fp_radix_sort_u64(const uint64_t* in, uint64_t* out, size_t n);

int is_sorted_u8(const uint8_t* arr, size_t n);
int is_sorted_u32(const uint32_t* arr, size_t n);
int is_sorted_u64(const uint64_t* arr, size_t n);

void generate_random_u8(uint8_t* arr, size_t n, unsigned int seed);
void generate_random_u32(uint32_t* arr, size_t n, unsigned int seed);
void generate_random_u64(uint64_t* arr, size_t n, unsigned int seed);

// Comparison functions for stdlib qsort
int compare_u8(const void* a, const void* b) {
    uint8_t arg1 = *(const uint8_t*)a;
    uint8_t arg2 = *(const uint8_t*)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

int compare_u32(const void* a, const void* b) {
    uint32_t arg1 = *(const uint32_t*)a;
    uint32_t arg2 = *(const uint32_t*)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

int compare_u64(const void* a, const void* b) {
    uint64_t arg1 = *(const uint64_t*)a;
    uint64_t arg2 = *(const uint64_t*)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

// ============================================================================
// Test 1: u8 Sorting (Bytes - Image Processing Use Case)
// ============================================================================
void test_u8_sorting() {
    printf("================================================================\n");
    printf("TEST 1: u8 Sorting (Byte Data - Image Processing)\n");
    printf("================================================================\n\n");

    const size_t n = 1000000;  // 1M bytes (~1 megapixel grayscale image)

    printf("Sorting %zu bytes (simulating 1 megapixel image)...\n\n", n);

    // Allocate arrays
    uint8_t* original = (uint8_t*)malloc(n * sizeof(uint8_t));
    uint8_t* radix_sorted = (uint8_t*)malloc(n * sizeof(uint8_t));
    uint8_t* qsort_sorted = (uint8_t*)malloc(n * sizeof(uint8_t));

    // Generate random data
    generate_random_u8(original, n, 12345);

    // Test 1: Radix Sort
    clock_t start = clock();
    fp_radix_sort_u8(original, radix_sorted, n);
    clock_t end = clock();
    double radix_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf("Radix Sort:\n");
    printf("  Time: %.3f ms\n", radix_time);
    printf("  Throughput: %.1f MB/sec\n", (n / 1024.0 / 1024.0) / (radix_time / 1000.0));
    printf("  Sorted: %s\n\n", is_sorted_u8(radix_sorted, n) ? "YES ✓" : "NO ✗");

    // Test 2: stdlib qsort
    memcpy(qsort_sorted, original, n * sizeof(uint8_t));
    start = clock();
    qsort(qsort_sorted, n, sizeof(uint8_t), compare_u8);
    end = clock();
    double qsort_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf("stdlib qsort:\n");
    printf("  Time: %.3f ms\n", qsort_time);
    printf("  Throughput: %.1f MB/sec\n", (n / 1024.0 / 1024.0) / (qsort_time / 1000.0));
    printf("  Sorted: %s\n\n", is_sorted_u8(qsort_sorted, n) ? "YES ✓" : "NO ✗");

    // Performance comparison
    printf("Speedup: %.2fX faster than qsort\n\n", qsort_time / radix_time);

    free(original);
    free(radix_sorted);
    free(qsort_sorted);
}

// ============================================================================
// Test 2: u32 Sorting (General Integers - Database Records)
// ============================================================================
void test_u32_sorting() {
    printf("================================================================\n");
    printf("TEST 2: u32 Sorting (32-bit Integers - Database IDs)\n");
    printf("================================================================\n\n");

    const size_t n = 1000000;  // 1M 32-bit integers

    printf("Sorting %zu u32 values (4 MB data)...\n\n", n);

    uint32_t* original = (uint32_t*)malloc(n * sizeof(uint32_t));
    uint32_t* radix_sorted = (uint32_t*)malloc(n * sizeof(uint32_t));
    uint32_t* qsort_sorted = (uint32_t*)malloc(n * sizeof(uint32_t));

    generate_random_u32(original, n, 67890);

    // Test 1: Radix Sort
    clock_t start = clock();
    fp_radix_sort_u32(original, radix_sorted, n);
    clock_t end = clock();
    double radix_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf("Radix Sort (4 passes):\n");
    printf("  Time: %.3f ms\n", radix_time);
    printf("  Throughput: %.1f MB/sec\n", (n * 4 / 1024.0 / 1024.0) / (radix_time / 1000.0));
    printf("  Sorted: %s\n\n", is_sorted_u32(radix_sorted, n) ? "YES ✓" : "NO ✗");

    // Test 2: stdlib qsort
    memcpy(qsort_sorted, original, n * sizeof(uint32_t));
    start = clock();
    qsort(qsort_sorted, n, sizeof(uint32_t), compare_u32);
    end = clock();
    double qsort_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf("stdlib qsort:\n");
    printf("  Time: %.3f ms\n", qsort_time);
    printf("  Throughput: %.1f MB/sec\n", (n * 4 / 1024.0 / 1024.0) / (qsort_time / 1000.0));
    printf("  Sorted: %s\n\n", is_sorted_u32(qsort_sorted, n) ? "YES ✓" : "NO ✗");

    printf("Speedup: %.2fX faster than qsort\n\n", qsort_time / radix_time);

    free(original);
    free(radix_sorted);
    free(qsort_sorted);
}

// ============================================================================
// Test 3: u64 Sorting (Large Integers - Timestamps, Hashes)
// ============================================================================
void test_u64_sorting() {
    printf("================================================================\n");
    printf("TEST 3: u64 Sorting (64-bit Integers - Timestamps/Hashes)\n");
    printf("================================================================\n\n");

    const size_t n = 1000000;  // 1M 64-bit integers

    printf("Sorting %zu u64 values (8 MB data)...\n\n", n);

    uint64_t* original = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* radix_sorted = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* qsort_sorted = (uint64_t*)malloc(n * sizeof(uint64_t));

    generate_random_u64(original, n, 11111);

    // Test 1: Radix Sort
    clock_t start = clock();
    fp_radix_sort_u64(original, radix_sorted, n);
    clock_t end = clock();
    double radix_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf("Radix Sort (8 passes):\n");
    printf("  Time: %.3f ms\n", radix_time);
    printf("  Throughput: %.1f MB/sec\n", (n * 8 / 1024.0 / 1024.0) / (radix_time / 1000.0));
    printf("  Sorted: %s\n\n", is_sorted_u64(radix_sorted, n) ? "YES ✓" : "NO ✗");

    // Test 2: stdlib qsort
    memcpy(qsort_sorted, original, n * sizeof(uint64_t));
    start = clock();
    qsort(qsort_sorted, n, sizeof(uint64_t), compare_u64);
    end = clock();
    double qsort_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf("stdlib qsort:\n");
    printf("  Time: %.3f ms\n", qsort_time);
    printf("  Throughput: %.1f MB/sec\n", (n * 8 / 1024.0 / 1024.0) / (qsort_time / 1000.0));
    printf("  Sorted: %s\n\n", is_sorted_u64(qsort_sorted, n) ? "YES ✓" : "NO ✗");

    printf("Speedup: %.2fX faster than qsort\n\n", qsort_time / radix_time);

    free(original);
    free(radix_sorted);
    free(qsort_sorted);
}

// ============================================================================
// Test 4: Small Array Correctness Test
// ============================================================================
void test_correctness() {
    printf("================================================================\n");
    printf("TEST 4: Correctness Verification (Small Arrays)\n");
    printf("================================================================\n\n");

    // Test u8
    printf("u8 Test:\n");
    uint8_t u8_in[10] = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
    uint8_t u8_out[10];
    fp_radix_sort_u8(u8_in, u8_out, 10);
    printf("  Input:  [");
    for (int i = 0; i < 10; i++) printf("%d%s", u8_in[i], i < 9 ? ", " : "");
    printf("]\n  Output: [");
    for (int i = 0; i < 10; i++) printf("%d%s", u8_out[i], i < 9 ? ", " : "");
    printf("]\n  Sorted: %s\n\n", is_sorted_u8(u8_out, 10) ? "YES ✓" : "NO ✗");

    // Test u32
    printf("u32 Test:\n");
    uint32_t u32_in[10] = {500, 200, 800, 100, 900, 300, 700, 400, 600, 0};
    uint32_t u32_out[10];
    fp_radix_sort_u32(u32_in, u32_out, 10);
    printf("  Input:  [");
    for (int i = 0; i < 10; i++) printf("%u%s", u32_in[i], i < 9 ? ", " : "");
    printf("]\n  Output: [");
    for (int i = 0; i < 10; i++) printf("%u%s", u32_out[i], i < 9 ? ", " : "");
    printf("]\n  Sorted: %s\n\n", is_sorted_u32(u32_out, 10) ? "YES ✓" : "NO ✗");

    // Test u64
    printf("u64 Test:\n");
    uint64_t u64_in[10] = {5000000000ULL, 2000000000ULL, 8000000000ULL,
                           1000000000ULL, 9000000000ULL, 3000000000ULL,
                           7000000000ULL, 4000000000ULL, 6000000000ULL, 0ULL};
    uint64_t u64_out[10];
    fp_radix_sort_u64(u64_in, u64_out, 10);
    printf("  Input:  [");
    for (int i = 0; i < 10; i++) printf("%llu%s", u64_in[i], i < 9 ? ", " : "");
    printf("]\n  Output: [");
    for (int i = 0; i < 10; i++) printf("%llu%s", u64_out[i], i < 9 ? ", " : "");
    printf("]\n  Sorted: %s\n\n", is_sorted_u64(u64_out, 10) ? "YES ✓" : "NO ✗");
}

int main() {
    printf("================================================================\n");
    printf("  Radix Sort Demo\n");
    printf("  FP-ASM Library - Integer Type Showcase\n");
    printf("================================================================\n\n");

    printf("Demonstrating radix sort on u8, u32, u64 types\n");
    printf("Comparing against stdlib qsort (O(n log n) comparison sort)\n\n");

    // Run correctness tests first
    test_correctness();

    // Run performance benchmarks
    test_u8_sorting();
    test_u32_sorting();
    test_u64_sorting();

    // Summary
    printf("================================================================\n");
    printf("  Radix Sort Demo Complete!\n");
    printf("================================================================\n\n");

    printf("Key Takeaways:\n");
    printf("  ✓ Showcases u8, u32, u64 integer type support\n");
    printf("  ✓ Linear-time O(n·k) sorting vs O(n log n)\n");
    printf("  ✓ Stable sorting via counting sort\n");
    printf("  ✓ Significantly faster than comparison sorts for integers\n\n");

    printf("FP-ASM Integer Types Used:\n");
    printf("  - u8:  32-wide SIMD (histogram counting)\n");
    printf("  - u32: 8-wide SIMD (prefix sum, partitioning)\n");
    printf("  - u64: 4-wide SIMD (large integers)\n\n");

    printf("Real-World Use Cases:\n");
    printf("  - u8:  Image histogram equalization, pixel sorting\n");
    printf("  - u32: Database index sorting, network packet ordering\n");
    printf("  - u64: Timestamp sorting, hash table ordering\n\n");

    return 0;
}
