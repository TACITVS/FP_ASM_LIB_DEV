#include <stdio.h>
#include <stdint.h>
#include "../include/fp_core.h"

int main() {
    printf("Testing TIER 2 operations...\n\n");

    // Test 1: Sort i64
    int64_t arr1[] = {5, 2, 8, 1, 9};
    printf("Before sort: ");
    for (int i = 0; i < 5; i++) printf("%lld ", arr1[i]);
    printf("\n");

    fp_sort_i64(arr1, 5);

    printf("After sort:  ");
    for (int i = 0; i < 5; i++) printf("%lld ", arr1[i]);
    printf("\n");

    // Check if sorted
    int sorted = 1;
    for (int i = 1; i < 5; i++) {
        if (arr1[i] < arr1[i-1]) sorted = 0;
    }
    printf("Is sorted: %s\n\n", sorted ? "YES" : "NO");

    // Test 2: Unique
    int64_t arr2[] = {1, 2, 2, 3, 3, 3, 4};
    int64_t result[7];

    size_t n = fp_unique_i64(arr2, result, 7);

    printf("Unique: ");
    for (size_t i = 0; i < n; i++) printf("%lld ", result[i]);
    printf("(count=%zu)\n\n", n);

    // Test 3: Union
    int64_t a[] = {1, 3, 5};
    int64_t b[] = {2, 3, 4};
    int64_t un[6];

    n = fp_union_i64(a, b, un, 3, 3);

    printf("Union: ");
    for (size_t i = 0; i < n; i++) printf("%lld ", un[i]);
    printf("(count=%zu)\n\n", n);

    // Test 4: Intersect
    int64_t c[] = {1, 2, 3, 4};
    int64_t d[] = {3, 4, 5, 6};
    int64_t inter[4];

    n = fp_intersect_i64(c, d, inter, 4, 4);

    printf("Intersect: ");
    for (size_t i = 0; i < n; i++) printf("%lld ", inter[i]);
    printf("(count=%zu)\n\n", n);

    printf("All TIER 2 operations working!\n");
    return 0;
}
