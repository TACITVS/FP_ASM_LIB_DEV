#include <stdio.h>
#include <stdint.h>

// Declare the assembly functions
extern uint64_t fp_reduce_add_u64(const uint64_t* in, size_t n);
extern uint64_t fp_reduce_min_u64(const uint64_t* in, size_t n);
extern uint64_t fp_reduce_max_u64(const uint64_t* in, size_t n);

int main() {
    printf("Testing critical U64 fixes...\n\n");

    // Test 1: Empty array (n=0)
    uint64_t dummy = 42;
    uint64_t result_min = fp_reduce_min_u64(&dummy, 0);
    printf("Test 1 - min(n=0): 0x%llX (expected 0xFFFFFFFFFFFFFFFF)\n", result_min);

    // Test 2: Null pointer
    uint64_t result_null = fp_reduce_add_u64(NULL, 10);
    printf("Test 2 - add(NULL): %llu (expected 0)\n", result_null);

    // Test 3: Basic correctness
    uint64_t data[] = {100, 50, 200, 25, 150};
    uint64_t result_min_data = fp_reduce_min_u64(data, 5);
    uint64_t result_max_data = fp_reduce_max_u64(data, 5);
    printf("Test 3 - min([100,50,200,25,150]): %llu (expected 25)\n", result_min_data);
    printf("Test 3 - max([100,50,200,25,150]): %llu (expected 200)\n", result_max_data);

    // Check results
    int passed = 0;
    if (result_min == 0xFFFFFFFFFFFFFFFFULL) {
        printf("PASS Test 1\n");
        passed++;
    } else {
        printf("FAIL Test 1\n");
    }

    if (result_null == 0) {
        printf("PASS Test 2\n");
        passed++;
    } else {
        printf("FAIL Test 2\n");
    }

    if (result_min_data == 25 && result_max_data == 200) {
        printf("PASS Test 3\n");
        passed++;
    } else {
        printf("FAIL Test 3\n");
    }

    printf("\n%d/3 tests passed\n", passed);
    return (passed == 3) ? 0 : 1;
}
