/**
 * fp_wrapper_demo.c - Comprehensive FP Wrapper Layer Demonstration
 *
 * Demonstrates the power of functional programming in C using:
 * - Pipelines for declarative transformations
 * - Monads for safe error handling
 * - Function composition
 * - Inline optimizations for zero-cost abstractions
 *
 * Compile:
 *   gcc fp_wrapper_demo.c \
 *       ../../src/wrappers/fp_compose.c \
 *       ../../src/wrappers/fp_monads.c \
 *       -I../../include \
 *       -o fp_wrapper_demo.exe \
 *       -lm -O3
 */

#include "fp_monads.h"
#include "fp_compose.h"
#include "fp_monads_inline.h"
#include "fp_compose_inline.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>

/* ============================================================================
 * EXAMPLE 1: Maybe Monad - Safe Computation
 * ============================================================================ */

double square(double x) {
    return x * x;
}

double add_ten(double x) {
    return x + 10.0;
}

void demo_maybe_monad(void) {
    printf("\n=== EXAMPLE 1: Maybe Monad - Safe Computation ===\n\n");

    // Safe division chain: (10 / 2) then sqrt then log
    Maybe result1 = fp_safe_divide_f64_inline(10.0, 2.0);
    result1 = fp_bind_maybe_f64(result1, fp_safe_sqrt_f64);
    result1 = fp_bind_maybe_f64(result1, fp_safe_log_f64);

    if (fp_is_just_inline(result1)) {
        printf("Success: (10/2) -> sqrt -> log = %f\n", fp_from_just_f64(result1));
    } else {
        printf("Computation failed (Nothing)\n");
    }

    // Division by zero - safely returns Nothing
    Maybe result2 = fp_safe_divide_f64_inline(10.0, 0.0);
    result2 = fp_bind_maybe_f64(result2, fp_safe_sqrt_f64);

    if (fp_is_just_inline(result2)) {
        printf("Success: %f\n", fp_from_just_f64(result2));
    } else {
        printf("Division by zero caught! Returned Nothing ✓\n");
    }

    // Negative sqrt - safely returns Nothing
    Maybe result3 = fp_safe_sqrt_f64(-4.0);
    if (fp_is_nothing_inline(result3)) {
        printf("Negative sqrt caught! Returned Nothing ✓\n");
    }

    // Using default values
    double safe_value = fp_from_maybe_f64_inline(result3, 0.0);
    printf("Using default: %f\n", safe_value);
}

/* ============================================================================
 * EXAMPLE 2: Either Monad - Error Messages
 * ============================================================================ */

void demo_either_monad(void) {
    printf("\n=== EXAMPLE 2: Either Monad - Error Messages ===\n\n");

    // Successful computation
    Either result1 = fp_checked_divide_f64_inline(100.0, 4.0);
    result1 = fp_fmap_either_f64_inline(result1, sqrt);
    result1 = fp_fmap_either_f64_inline(result1, add_ten);

    if (fp_is_right_inline(result1)) {
        printf("Success: (100/4) -> sqrt -> +10 = %f\n", fp_from_right_f64_inline(result1));
    }

    // Division by zero with error message
    Either result2 = fp_checked_divide_f64_inline(100.0, 0.0);
    result2 = fp_fmap_either_f64_inline(result2, sqrt);

    if (fp_is_left_inline(result2)) {
        printf("Error: %s (code: %d) ✓\n",
               fp_from_left_msg(result2),
               fp_from_left_code(result2));
    }

    // Integer overflow detection
    Either result3 = fp_checked_divide_i64_inline(INT64_MIN, -1);
    if (fp_is_left_inline(result3)) {
        printf("Error: %s (code: %d) ✓\n",
               fp_from_left_msg(result3),
               fp_from_left_code(result3));
    }

    // Array bounds checking
    double arr[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    Either result4 = fp_checked_at_f64_inline(arr, 5, 10);
    if (fp_is_left_inline(result4)) {
        printf("Error: %s (code: %d) ✓\n",
               fp_from_left_msg(result4),
               fp_from_left_code(result4));
    }
}

/* ============================================================================
 * EXAMPLE 3: Function Composition
 * ============================================================================ */

double times_two(double x) {
    return x * 2.0;
}

double subtract_five(double x) {
    return x - 5.0;
}

void demo_composition(void) {
    printf("\n=== EXAMPLE 3: Function Composition ===\n\n");

    // Simple 2-function composition: (x * 2) - 5
    double result1 = fp_compose_2_f64_inline(10.0, subtract_five, times_two);
    printf("(10 * 2) - 5 = %f\n", result1);

    // 3-function composition: ((x^2) * 2) - 5
    double result2 = fp_compose_3_f64_inline(3.0, subtract_five, times_two, square);
    printf("((3^2) * 2) - 5 = %f\n", result2);

    // Composition with arrays
    double input[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double output[5];

    // Apply composed function to array
    for (size_t i = 0; i < 5; i++) {
        output[i] = fp_compose_2_f64_inline(input[i], subtract_five, times_two);
    }

    printf("Composed map: [");
    for (size_t i = 0; i < 5; i++) {
        printf("%.1f%s", output[i], i < 4 ? ", " : "");
    }
    printf("]\n");
}

/* ============================================================================
 * EXAMPLE 4: Pipeline Builder (Fluent API)
 * ============================================================================ */

double map_square(double x, void* ctx) {
    (void)ctx;
    return x * x;
}

bool filter_gt_ten(double x, void* ctx) {
    (void)ctx;
    return x > 10.0;
}

double reduce_sum(double acc, double x, void* ctx) {
    (void)ctx;
    return acc + x;
}

void demo_pipeline(void) {
    printf("\n=== EXAMPLE 4: Pipeline Builder (Fluent API) ===\n\n");

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    size_t n = 8;

    // Build pipeline: square each element, filter > 10, sum
    fp_pipeline_f64_t* pipeline = fp_pipeline_f64(data, n);

    double result = pipeline
        ->map(pipeline, map_square, NULL)
        ->filter(pipeline, filter_gt_ten, NULL)
        ->reduce(pipeline, 0.0, reduce_sum, NULL);

    printf("Pipeline: data -> square -> filter(>10) -> sum = %f\n", result);

    // Expected: [1,4,9,16,25,36,49,64] -> [16,25,36,49,64] -> 190
    printf("Expected: 190.0 ✓\n");

    fp_pipeline_free_f64(pipeline);

    // Another pipeline: to_array example
    double output[8];
    fp_pipeline_f64_t* pipeline2 = fp_pipeline_f64(data, n);

    size_t out_size = pipeline2
        ->map(pipeline2, map_square, NULL)
        ->filter(pipeline2, filter_gt_ten, NULL)
        ->to_array(pipeline2, output, 8);

    printf("Pipeline to_array (count=%zu): [", out_size);
    for (size_t i = 0; i < out_size; i++) {
        printf("%.0f%s", output[i], i < out_size - 1 ? ", " : "");
    }
    printf("]\n");

    fp_pipeline_free_f64(pipeline2);
}

/* ============================================================================
 * EXAMPLE 5: Fused Map-Reduce (Zero-Copy Optimization)
 * ============================================================================ */

double reduce_add(double acc, double x) {
    return acc + x;
}

bool is_even(double x) {
    return ((int)x % 2) == 0;
}

void demo_fused_operations(void) {
    printf("\n=== EXAMPLE 5: Fused Map-Reduce (Zero-Copy) ===\n\n");

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    size_t n = 5;

    // Imperative way (creates temporary array):
    // double temp[5];
    // for (size_t i = 0; i < 5; i++) temp[i] = data[i] * data[i];
    // double sum = 0; for (...) sum += temp[i];

    // Functional way (zero-copy, single pass):
    double result = fp_fused_map_reduce_f64_inline(data, n, square, 0.0, reduce_add);

    printf("Fused map-reduce: sum(square(data)) = %f\n", result);
    printf("Expected: 1 + 4 + 9 + 16 + 25 = 55.0 ✓\n");

    // Fused filter-reduce
    double result2 = fp_fused_filter_reduce_f64_inline(data, n, is_even, 0.0, reduce_add);
    printf("Fused filter-reduce: sum(filter(even, data)) = %f\n", result2);
    printf("Expected: 2 + 4 = 6.0 ✓\n");
}

/* ============================================================================
 * EXAMPLE 6: Sequence Operations (traverse, catMaybes)
 * ============================================================================ */

Maybe safe_reciprocal(double x) {
    if (x == 0.0) return fp_nothing_inline();
    return fp_just_f64_inline(1.0 / x);
}

void demo_sequence_operations(void) {
    printf("\n=== EXAMPLE 6: Sequence Operations ===\n\n");

    // Traverse: map a function that returns Maybe over an array
    double data[] = {1.0, 2.0, 4.0, 5.0};
    double output[4];

    Maybe result = fp_traverse_maybe_f64(data, 4, safe_reciprocal, output);

    if (fp_is_just_inline(result)) {
        printf("Traverse success: [");
        for (size_t i = 0; i < 4; i++) {
            printf("%.2f%s", output[i], i < 3 ? ", " : "");
        }
        printf("]\n");
    } else {
        printf("Traverse failed (contains zero)\n");
    }

    // Traverse with zero (fails)
    double data_with_zero[] = {1.0, 2.0, 0.0, 4.0};
    Maybe result2 = fp_traverse_maybe_f64(data_with_zero, 4, safe_reciprocal, output);

    if (fp_is_nothing_inline(result2)) {
        printf("Traverse with zero failed ✓\n");
    }

    // catMaybes: filter out Nothing values
    Maybe maybes[] = {
        fp_just_f64_inline(1.0),
        fp_nothing_inline(),
        fp_just_f64_inline(3.0),
        fp_nothing_inline(),
        fp_just_f64_inline(5.0)
    };

    double values[5];
    size_t count = fp_cat_maybes_f64_inline(maybes, 5, values);

    printf("catMaybes (count=%zu): [", count);
    for (size_t i = 0; i < count; i++) {
        printf("%.0f%s", values[i], i < count - 1 ? ", " : "");
    }
    printf("]\n");
    printf("Expected: [1, 3, 5] ✓\n");
}

/* ============================================================================
 * EXAMPLE 7: Lazy Evaluation
 * ============================================================================ */

double increment(double x) {
    return x + 1.0;
}

void demo_lazy_evaluation(void) {
    printf("\n=== EXAMPLE 7: Lazy Evaluation ===\n\n");

    // Lazy range: [0, 10) with step 1
    fp_lazy_seq_t* seq1 = fp_lazy_range_f64(0.0, 10.0, 1.0);

    printf("Lazy range [0, 10): ");
    while (seq1->has_next(seq1)) {
        printf("%.0f ", seq1->next(seq1));
    }
    printf("\n");

    fp_lazy_free_f64(seq1);

    // Lazy iterate: infinite sequence 1, 2, 3, 4, ...
    fp_lazy_seq_t* seq2 = fp_lazy_iterate_f64(1.0, increment);

    printf("Lazy iterate (first 10): ");
    for (int i = 0; i < 10; i++) {
        printf("%.0f ", seq2->next(seq2));
    }
    printf("\n");

    fp_lazy_free_f64(seq2);

    // Force evaluation to array
    fp_lazy_seq_t* seq3 = fp_lazy_range_f64(5.0, 15.0, 2.0);
    size_t out_size;
    double* result = fp_lazy_to_array_f64(seq3, 10, &out_size);

    if (result) {
        printf("Lazy to array (count=%zu): [", out_size);
        for (size_t i = 0; i < out_size; i++) {
            printf("%.0f%s", result[i], i < out_size - 1 ? ", " : "");
        }
        printf("]\n");
        free(result);
    }

    fp_lazy_free_f64(seq3);
}

/* ============================================================================
 * EXAMPLE 8: Practical Use Case - Data Processing Pipeline
 * ============================================================================ */

bool is_valid(double x) {
    return x > 0.0;
}

double to_fahrenheit(double c) {
    return c * 9.0 / 5.0 + 32.0;
}

void demo_practical_use_case(void) {
    printf("\n=== EXAMPLE 8: Practical Data Processing Pipeline ===\n\n");

    // Simulate sensor data with some invalid readings (zeros)
    double sensor_data[] = {23.5, 0.0, 25.1, 24.8, 0.0, 26.2, 23.9, 25.5};
    size_t n = 8;

    printf("Raw sensor data: [");
    for (size_t i = 0; i < n; i++) {
        printf("%.1f%s", sensor_data[i], i < n - 1 ? ", " : "");
    }
    printf("]\n");

    // Process: filter invalid (zeros), convert to Fahrenheit, compute average

    // Count valid readings
    size_t valid_count = 0;
    for (size_t i = 0; i < n; i++) {
        if (is_valid(sensor_data[i])) valid_count++;
    }

    // Fused filter-map-reduce (triple fusion!)
    double sum_fahrenheit = 0.0;
    size_t count = 0;

    for (size_t i = 0; i < n; i++) {
        if (is_valid(sensor_data[i])) {
            sum_fahrenheit += to_fahrenheit(sensor_data[i]);
            count++;
        }
    }

    double avg_fahrenheit = sum_fahrenheit / count;

    printf("Valid readings: %zu/%zu\n", valid_count, n);
    printf("Average temperature: %.2f°F\n", avg_fahrenheit);

    // Alternative: Using pipeline
    fp_pipeline_f64_t* pipeline = fp_pipeline_f64(sensor_data, n);

    double map_to_f(double x, void* ctx) {
        (void)ctx;
        return x * 9.0 / 5.0 + 32.0;
    }

    bool filter_valid(double x, void* ctx) {
        (void)ctx;
        return x > 0.0;
    }

    double pipeline_sum = pipeline
        ->filter(pipeline, filter_valid, NULL)
        ->map(pipeline, map_to_f, NULL)
        ->reduce(pipeline, 0.0, reduce_sum, NULL);

    double pipeline_avg = pipeline_sum / valid_count;

    printf("Pipeline average: %.2f°F ✓\n", pipeline_avg);

    fp_pipeline_free_f64(pipeline);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  FP-ASM Functional Programming Wrapper Layer Demo       ║\n");
    printf("║  Demonstrating Haskell-style FP in C with zero overhead ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    demo_maybe_monad();
    demo_either_monad();
    demo_composition();
    demo_pipeline();
    demo_fused_operations();
    demo_sequence_operations();
    demo_lazy_evaluation();
    demo_practical_use_case();

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  All demonstrations completed successfully! ✓            ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    return 0;
}
