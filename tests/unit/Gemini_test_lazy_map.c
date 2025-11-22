#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "../../include/fp_compose.h"

// The function we will map over the sequence
static double square(double x) {
    return x * x;
}

// Helper to compare two double arrays
static bool arrays_equal(const double* a, const double* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (fabs(a[i] - b[i]) > 1e-6) {
            return false;
        }
    }
    return true;
}

int main() {
    printf("--- Starting Gemini Test ---\n");
    printf("Running test for Gemini_test_lazy_map...\n");

    // 1. Create the source data and initial lazy sequence
    const double source_data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    const size_t n = sizeof(source_data) / sizeof(source_data[0]);
    fp_lazy_seq_t* initial_seq = fp_lazy_from_array_f64(source_data, n);
    if (!initial_seq) {
        fprintf(stderr, "Failed to create initial lazy sequence.\n");
        return 1;
    }

    // 2. Create a new lazy sequence by mapping the `square` function
    // This is the function we need to implement
    fp_lazy_seq_t* mapped_seq = fp_lazy_map_f64(initial_seq, square);
    if (!mapped_seq) {
        fprintf(stderr, "fp_lazy_map_f64 returned NULL.\n");
        fp_lazy_free_f64(initial_seq);
        return 1;
    }

    // 3. Force evaluation of the new lazy sequence into an array
    double result_array[5];
    size_t result_size = 0;
    // We use fp_lazy_to_array_f64, which is already implemented
    double* temp_result = fp_lazy_to_array_f64(mapped_seq, n, &result_size);
    if (!temp_result) {
        fprintf(stderr, "fp_lazy_to_array_f64 returned NULL.\n");
        fp_lazy_free_f64(mapped_seq); // This should also free initial_seq
        return 1;
    }
    for(size_t i=0; i<result_size; ++i) result_array[i] = temp_result[i];
    free(temp_result);


    // 4. Assert that the resulting array contains the correct squared values
    const double expected_data[] = {1.0, 4.0, 9.0, 16.0, 25.0};
    if (result_size != n) {
        fprintf(stderr, "Test failed: Expected size %zu, but got %zu.\n", n, result_size);
        fp_lazy_free_f64(mapped_seq);
        return 1;
    }

    if (!arrays_equal(result_array, expected_data, n)) {
        fprintf(stderr, "Test failed: Result array does not match expected data.\n");
        fprintf(stderr, "Expected: [1.0, 4.0, 9.0, 16.0, 25.0]\n");
        fprintf(stderr, "Got:      [");
        for (size_t i = 0; i < result_size; i++) {
            fprintf(stderr, "%.1f", result_array[i]);
            if (i < result_size - 1) fprintf(stderr, ", ");
        }
        fprintf(stderr, "]\n");
        fp_lazy_free_f64(mapped_seq);
        return 1;
    }

    printf("Test passed!\n");

    // 5. Clean up
    fp_lazy_free_f64(mapped_seq); // This should chain cleanup calls

    return 0;
}
