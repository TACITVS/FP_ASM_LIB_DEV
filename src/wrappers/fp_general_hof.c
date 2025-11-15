/**
 * fp_general_hof.c
 *
 * General Higher-Order Functions for 100% FP Language Equivalence
 *
 * This file implements the 4 fundamental higher-order functions that complete
 * FP-ASM's equivalence with Haskell Prelude, Common Lisp, and ML/OCaml:
 *   1. foldl  - General reduction with arbitrary function
 *   2. map    - General transformation with arbitrary function
 *   3. filter - General selection with arbitrary predicate
 *   4. zipWith - General combination with arbitrary function
 *
 * DESIGN CHOICE:
 * These functions are implemented in plain C (not assembly) because:
 *   - Function pointer indirection prevents most SIMD optimizations
 *   - The overhead is unavoidable when supporting arbitrary user functions
 *   - Specialized versions (fp_reduce_add, fp_map_abs, etc.) remain for hot paths
 *
 * PERFORMANCE:
 * Expect ~20-30% slower than specialized versions due to indirect function calls.
 * This is the price of generality - Haskell has the same tradeoff!
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../include/fp_core.h"

/* ============================================================================
 * FOLDL - General left fold (reduction)
 * ============================================================================ */

/**
 * General left fold for int64_t arrays
 *
 * Iterates left-to-right, accumulating values using the provided function.
 * This is the FOUNDATION of functional programming - all other reductions
 * (sum, product, max, min, etc.) can be expressed as foldl.
 *
 * Example implementations using foldl:
 *   sum     = foldl (+) 0
 *   product = foldl (*) 1
 *   max     = foldl max INT64_MIN
 *   count   = foldl (\acc _ -> acc + 1) 0
 */
int64_t fp_foldl_i64(const int64_t* input, size_t n, int64_t init,
                     int64_t (*fn)(int64_t acc, int64_t x, void* ctx),
                     void* context) {
    if (!input || !fn) return init;

    int64_t acc = init;
    for (size_t i = 0; i < n; i++) {
        acc = fn(acc, input[i], context);
    }
    return acc;
}

/**
 * General left fold for f64 arrays
 */
double fp_foldl_f64(const double* input, size_t n, double init,
                    double (*fn)(double acc, double x, void* ctx),
                    void* context) {
    if (!input || !fn) return init;

    double acc = init;
    for (size_t i = 0; i < n; i++) {
        acc = fn(acc, input[i], context);
    }
    return acc;
}

/* ============================================================================
 * MAP - General element-wise transformation
 * ============================================================================ */

/**
 * General map for int64_t arrays
 *
 * Transforms each element independently using the provided function.
 * This is the canonical FP transformation operation.
 *
 * Example implementations using map:
 *   double  = map (\x -> x * 2)
 *   square  = map (\x -> x * x)
 *   abs     = map (\x -> x < 0 ? -x : x)
 *   negate  = map (\x -> -x)
 */
void fp_map_i64(const int64_t* input, int64_t* output, size_t n,
                int64_t (*fn)(int64_t x, void* ctx),
                void* context) {
    if (!input || !output || !fn) return;

    for (size_t i = 0; i < n; i++) {
        output[i] = fn(input[i], context);
    }
}

/**
 * General map for f64 arrays
 */
void fp_map_f64(const double* input, double* output, size_t n,
                double (*fn)(double x, void* ctx),
                void* context) {
    if (!input || !output || !fn) return;

    for (size_t i = 0; i < n; i++) {
        output[i] = fn(input[i], context);
    }
}

/* ============================================================================
 * FILTER - General element selection
 * ============================================================================ */

/**
 * General filter for int64_t arrays
 *
 * Selects elements matching the provided predicate, maintaining order.
 * This is the canonical FP selection operation.
 *
 * Example implementations using filter:
 *   positives    = filter (\x -> x > 0)
 *   evens        = filter (\x -> x % 2 == 0)
 *   in_range     = filter (\x -> x >= min && x <= max)
 *   non_zero     = filter (\x -> x != 0)
 */
size_t fp_filter_i64(const int64_t* input, int64_t* output, size_t n,
                     bool (*predicate)(int64_t x, void* ctx),
                     void* context) {
    if (!input || !output || !predicate) return 0;

    size_t write_idx = 0;
    for (size_t i = 0; i < n; i++) {
        if (predicate(input[i], context)) {
            output[write_idx++] = input[i];
        }
    }
    return write_idx;
}

/**
 * General filter for f64 arrays
 */
size_t fp_filter_f64(const double* input, double* output, size_t n,
                     bool (*predicate)(double x, void* ctx),
                     void* context) {
    if (!input || !output || !predicate) return 0;

    size_t write_idx = 0;
    for (size_t i = 0; i < n; i++) {
        if (predicate(input[i], context)) {
            output[write_idx++] = input[i];
        }
    }
    return write_idx;
}

/* ============================================================================
 * ZIPWITH - General parallel combination
 * ============================================================================ */

/**
 * General zipWith for int64_t arrays
 *
 * Combines corresponding elements from two arrays using the provided function.
 * This is the canonical FP parallel operation.
 *
 * Example implementations using zipWith:
 *   add      = zipWith (\x y -> x + y)
 *   multiply = zipWith (\x y -> x * y)
 *   max      = zipWith (\x y -> x > y ? x : y)
 *   distance = zipWith (\x y -> abs(x - y))
 */
void fp_zipWith_i64(const int64_t* input_a, const int64_t* input_b, int64_t* output, size_t n,
                    int64_t (*fn)(int64_t x, int64_t y, void* ctx),
                    void* context) {
    if (!input_a || !input_b || !output || !fn) return;

    for (size_t i = 0; i < n; i++) {
        output[i] = fn(input_a[i], input_b[i], context);
    }
}

/**
 * General zipWith for f64 arrays
 */
void fp_zipWith_f64(const double* input_a, const double* input_b, double* output, size_t n,
                    double (*fn)(double x, double y, void* ctx),
                    void* context) {
    if (!input_a || !input_b || !output || !fn) return;

    for (size_t i = 0; i < n; i++) {
        output[i] = fn(input_a[i], input_b[i], context);
    }
}

/* ============================================================================
 * COMPOSE - Function composition (Haskell-style)
 * ============================================================================ */

/**
 * Generic function composition: (f . g)(x) = f(g(x))
 *
 * Haskell equivalent:
 *   (.) :: (b -> c) -> (a -> b) -> (a -> c)
 *   compose f g = \x -> f (g x)
 *
 * This is the FOUNDATION of functional programming pipelines.
 * Instead of nested function calls or intermediate variables:
 *   temp = map g input
 *   result = map f temp
 *
 * You write:
 *   result = compose f g input
 *
 * PURITY GUARANTEE:
 * - Input array NEVER modified (const)
 * - Intermediate results isolated in temp buffer
 * - No heap allocation (user provides temp)
 * - Deterministic (same input = same output)
 *
 * Performance: Same as two separate map calls, but clearer intent.
 */
void fp_compose_generic(const void* input, void* output, size_t n,
                        size_t size_a, size_t size_b, size_t size_c,
                        void (*g)(void* out, const void* in, void* ctx_g),
                        void* ctx_g,
                        void (*f)(void* out, const void* in, void* ctx_f),
                        void* ctx_f,
                        void* temp) {
    if (!input || !output || !g || !f || !temp) return;

    const unsigned char* in_ptr = (const unsigned char*)input;
    unsigned char* temp_ptr = (unsigned char*)temp;
    unsigned char* out_ptr = (unsigned char*)output;

    // Step 1: Apply g to all elements (a -> b)
    for (size_t i = 0; i < n; i++) {
        g(temp_ptr + i * size_b, in_ptr + i * size_a, ctx_g);
    }

    // Step 2: Apply f to all intermediate results (b -> c)
    for (size_t i = 0; i < n; i++) {
        f(out_ptr + i * size_c, temp_ptr + i * size_b, ctx_f);
    }
}
