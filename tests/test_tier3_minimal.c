#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// Declare just the functions we want to test
extern size_t fp_group_i64(const int64_t* input, int64_t* groups_out, int64_t* counts_out, size_t n);
extern bool fp_reduce_and_bool(const int64_t* input, size_t n);

int main() {
    printf("Testing TIER 3 minimal...\n");

    // Test 1: fp_group_i64
    int64_t input[] = {1,1,2,2,2,3};
    int64_t groups[6], counts[6];

    size_t n = fp_group_i64(input, groups, counts, 6);
    printf("Groups found: %zu\n", n);

    // Test 2: fp_reduce_and_bool
    int64_t all_true[] = {1, 5, -3, 100};
    bool result = fp_reduce_and_bool(all_true, 4);
    printf("AND all true: %s\n", result ? "PASS" : "FAIL");

    printf("Minimal tests completed!\n");
    return 0;
}
