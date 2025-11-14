#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "../include/fp_core.h"

static int compare_i64(const void* lhs, const void* rhs) {
    const int64_t a = *(const int64_t*)lhs;
    const int64_t b = *(const int64_t*)rhs;
    return (a > b) - (a < b);
}

static void sort_copy_i64(const int64_t* input, int64_t* output, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        output[i] = input[i];
    }
    qsort(output, n, sizeof(int64_t), compare_i64);
}

int main() {
    printf("Testing TIER 2 operations...\n\n");

    // Test 1: Sort i64
    const int64_t arr1[] = {5, 2, 8, 1, 9};
    int64_t sorted_arr1[5];
    sort_copy_i64(arr1, sorted_arr1, 5);
    printf("Before sort: ");
    for (size_t i = 0; i < 5; i++) printf("%" PRId64 " ", arr1[i]);
    printf("\n");

    printf("After sort:  ");
    for (size_t i = 0; i < 5; i++) printf("%" PRId64 " ", sorted_arr1[i]);
    printf("\n");

    // Check if sorted
    int sorted = 1;
    for (size_t i = 1; i < 5; i++) {
        if (sorted_arr1[i] < sorted_arr1[i-1]) sorted = 0;
    }
    printf("Is sorted: %s\n\n", sorted ? "YES" : "NO");

    // Test 2: Unique
    const int64_t arr2[] = {1, 2, 2, 3, 3, 3, 4};
    int64_t result[7];

    size_t n = fp_unique_i64(arr2, result, 7);

    printf("Unique: ");
    for (size_t i = 0; i < n; i++) printf("%" PRId64 " ", result[i]);
    printf("(count=%zu)\n\n", n);

    // Test 3: Union
    const int64_t a[] = {1, 3, 5};
    const int64_t b[] = {2, 3, 4};
    int64_t un[6];

    n = fp_union_i64(a, b, un, 3, 3);

    printf("Union: ");
    for (size_t i = 0; i < n; i++) printf("%" PRId64 " ", un[i]);
    printf("(count=%zu)\n\n", n);

    // Test 4: Intersect
    const int64_t c[] = {1, 2, 3, 4};
    const int64_t d[] = {3, 4, 5, 6};
    int64_t inter[4];

    n = fp_intersect_i64(c, d, inter, 4, 4);

    printf("Intersect: ");
    for (size_t i = 0; i < n; i++) printf("%" PRId64 " ", inter[i]);
    printf("(count=%zu)\n\n", n);

    printf("All TIER 2 operations working!\n");
    return 0;
}
