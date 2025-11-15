// fp_radix_sort.c
//
// Radix Sort Implementation
// Demonstrates FP-ASM primitives working with INTEGER types (u8, u32, u64)
//
// Algorithm: LSD (Least Significant Digit) Radix Sort
// - Stable sort using counting sort on each byte/digit
// - O(n·k) complexity where k = number of bytes per element
// - Naturally operates on integers (showcases u8, u32, u64 types)
//
// FP Primitives Used:
// - Histogram computation (implicit via counting)
// - Prefix sum (for stable partitioning)
// - In-place partitioning

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "fp_core.h"

// Result structure for radix sort
typedef struct {
    void* sorted_data;     // Sorted output array
    size_t n;              // Number of elements
    size_t element_size;   // Size of each element in bytes
    double sort_time_ms;   // Time taken to sort (milliseconds)
} RadixSortResult;

// ============================================================================
// U8 Radix Sort (Single-pass counting sort - only 1 byte per element)
// ============================================================================

// Counting sort for u8 (0-255 range, single pass)
void fp_radix_sort_u8(const uint8_t* in, uint8_t* out, size_t n) {
    // Allocate histogram for 256 possible byte values
    uint32_t histogram[256] = {0};

    // Count occurrences of each byte value
    for (size_t i = 0; i < n; i++) {
        histogram[in[i]]++;
    }

    // Write sorted output (stable)
    size_t write_idx = 0;
    for (int value = 0; value < 256; value++) {
        for (uint32_t count = 0; count < histogram[value]; count++) {
            out[write_idx++] = (uint8_t)value;
        }
    }
}

// ============================================================================
// U32 Radix Sort (4-pass LSD radix sort, one pass per byte)
// ============================================================================

// Counting sort for a single byte position (stable)
static void counting_sort_u32_by_byte(
    const uint32_t* in,
    uint32_t* out,
    size_t n,
    int byte_index  // 0=LSB, 1, 2, 3=MSB
) {
    uint32_t histogram[256] = {0};
    uint32_t prefix_sum[256] = {0};

    // Extract byte at position byte_index
    const int shift = byte_index * 8;

    // Count occurrences
    for (size_t i = 0; i < n; i++) {
        uint8_t byte = (in[i] >> shift) & 0xFF;
        histogram[byte]++;
    }

    // Compute prefix sum (cumulative positions)
    prefix_sum[0] = 0;
    for (int i = 1; i < 256; i++) {
        prefix_sum[i] = prefix_sum[i - 1] + histogram[i - 1];
    }

    // Place elements in sorted order (stable)
    for (size_t i = 0; i < n; i++) {
        uint8_t byte = (in[i] >> shift) & 0xFF;
        out[prefix_sum[byte]++] = in[i];
    }
}

// LSD Radix Sort for u32 (4 passes, LSB to MSB)
void fp_radix_sort_u32(const uint32_t* in, uint32_t* out, size_t n) {
    // Need temporary buffer for ping-pong sorting
    uint32_t* temp = (uint32_t*)malloc(n * sizeof(uint32_t));

    // Copy input to output for first pass
    memcpy(out, in, n * sizeof(uint32_t));

    // Sort by each byte, LSB to MSB
    for (int byte_idx = 0; byte_idx < 4; byte_idx++) {
        if (byte_idx % 2 == 0) {
            counting_sort_u32_by_byte(out, temp, n, byte_idx);
        } else {
            counting_sort_u32_by_byte(temp, out, n, byte_idx);
        }
    }

    // If odd number of passes, copy temp back to out
    // (We did 4 passes, so result is in temp after pass 3, then out after pass 4)
    // Actually, pass 0: out->temp, pass 1: temp->out, pass 2: out->temp, pass 3: temp->out
    // So result is already in out

    free(temp);
}

// ============================================================================
// U64 Radix Sort (8-pass LSD radix sort, one pass per byte)
// ============================================================================

// Counting sort for a single byte position (stable) - u64 version
static void counting_sort_u64_by_byte(
    const uint64_t* in,
    uint64_t* out,
    size_t n,
    int byte_index  // 0=LSB, ..., 7=MSB
) {
    uint32_t histogram[256] = {0};
    uint32_t prefix_sum[256] = {0};

    const int shift = byte_index * 8;

    // Count occurrences
    for (size_t i = 0; i < n; i++) {
        uint8_t byte = (in[i] >> shift) & 0xFF;
        histogram[byte]++;
    }

    // Compute prefix sum
    prefix_sum[0] = 0;
    for (int i = 1; i < 256; i++) {
        prefix_sum[i] = prefix_sum[i - 1] + histogram[i - 1];
    }

    // Place elements in sorted order (stable)
    for (size_t i = 0; i < n; i++) {
        uint8_t byte = (in[i] >> shift) & 0xFF;
        out[prefix_sum[byte]++] = in[i];
    }
}

// LSD Radix Sort for u64 (8 passes, LSB to MSB)
void fp_radix_sort_u64(const uint64_t* in, uint64_t* out, size_t n) {
    uint64_t* temp = (uint64_t*)malloc(n * sizeof(uint64_t));

    memcpy(out, in, n * sizeof(uint64_t));

    // Sort by each byte, LSB to MSB
    for (int byte_idx = 0; byte_idx < 8; byte_idx++) {
        if (byte_idx % 2 == 0) {
            counting_sort_u64_by_byte(out, temp, n, byte_idx);
        } else {
            counting_sort_u64_by_byte(temp, out, n, byte_idx);
        }
    }

    // 8 passes means result is in out (even number)

    free(temp);
}

// ============================================================================
// Helper Functions
// ============================================================================

// Verify array is sorted
int is_sorted_u8(const uint8_t* arr, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (arr[i] < arr[i - 1]) return 0;
    }
    return 1;
}

int is_sorted_u32(const uint32_t* arr, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (arr[i] < arr[i - 1]) return 0;
    }
    return 1;
}

int is_sorted_u64(const uint64_t* arr, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (arr[i] < arr[i - 1]) return 0;
    }
    return 1;
}

// Generate random data for testing
void generate_random_u8(uint8_t* arr, size_t n, unsigned int seed) {
    srand(seed);
    for (size_t i = 0; i < n; i++) {
        arr[i] = rand() % 256;
    }
}

void generate_random_u32(uint32_t* arr, size_t n, unsigned int seed) {
    srand(seed);
    for (size_t i = 0; i < n; i++) {
        arr[i] = ((uint32_t)rand() << 16) | (uint32_t)rand();
    }
}

void generate_random_u64(uint64_t* arr, size_t n, unsigned int seed) {
    srand(seed);
    for (size_t i = 0; i < n; i++) {
        arr[i] = ((uint64_t)rand() << 48) |
                 ((uint64_t)rand() << 32) |
                 ((uint64_t)rand() << 16) |
                 (uint64_t)rand();
    }
}
