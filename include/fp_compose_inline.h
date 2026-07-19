/**
 * fp_compose_inline.h - Inline Optimizations for Composition Hot Paths
 *
 * Provides inline versions of frequently-used composition operations
 * for zero-overhead abstractions in performance-critical code.
 *
 * Usage:
 *   Include this AFTER fp_compose.h in hot-path code.
 *   The compiler will inline these for maximum performance.
 */

#ifndef FP_COMPOSE_INLINE_H
#define FP_COMPOSE_INLINE_H

#include "fp_compose.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * INLINE BASIC COMBINATORS
 * ============================================================================ */

/**
 * Inline identity - Compiles to no-op (returns input register)
 */
static inline double fp_id_f64_inline(double x) {
    return x;
}

static inline int64_t fp_id_i64_inline(int64_t x) {
    return x;
}

/**
 * Inline const - Returns constant value (register or immediate)
 */
static inline double fp_const_f64_inline_apply(fp_const_t c, double ignored) {
    (void)ignored;
    return c.value_f64;
}

static inline int64_t fp_const_i64_inline_apply(fp_const_t c, int64_t ignored) {
    (void)ignored;
    return c.value_i64;
}

/* ============================================================================
 * INLINE FUNCTION COMPOSITION (f . g)
 * ============================================================================ */

/**
 * Inline 2-function composition - Compiles to two function calls
 */
static inline double fp_compose_2_f64_inline(double x, double (*f)(double), double (*g)(double)) {
    return f(g(x));
}

static inline int64_t fp_compose_2_i64_inline(int64_t x, int64_t (*f)(int64_t), int64_t (*g)(int64_t)) {
    return f(g(x));
}

/**
 * Inline 3-function composition - For common f . g . h pattern
 */
static inline double fp_compose_3_f64_inline(double x,
                                               double (*f)(double),
                                               double (*g)(double),
                                               double (*h)(double)) {
    return f(g(h(x)));
}

static inline int64_t fp_compose_3_i64_inline(int64_t x,
                                                int64_t (*f)(int64_t),
                                                int64_t (*g)(int64_t),
                                                int64_t (*h)(int64_t)) {
    return f(g(h(x)));
}

/* ============================================================================
 * INLINE PARTIAL APPLICATION (Hot Path)
 * ============================================================================ */

/**
 * Inline partial map application - Zero overhead for curried functions
 */
static inline void fp_apply_partial_map_f64_inline(fp_partial_map_f64_t partial,
                                                     const double* in,
                                                     double* out,
                                                     size_t n) {
    if (!in || !out || !partial.transform) return;

    // Compiler can unroll this loop for small n
    for (size_t i = 0; i < n; i++) {
        out[i] = partial.transform(in[i], partial.context);
    }
}

/**
 * Inline partial filter application
 */
static inline size_t fp_apply_partial_filter_f64_inline(fp_partial_filter_f64_t partial,
                                                          const double* in,
                                                          double* out,
                                                          size_t n) {
    if (!in || !out || !partial.predicate) return 0;

    size_t write_idx = 0;
    for (size_t i = 0; i < n; i++) {
        if (partial.predicate(in[i], partial.context)) {
            out[write_idx++] = in[i];
        }
    }
    return write_idx;
}

/* ============================================================================
 * INLINE PIPELINE HELPERS (for simple use cases)
 * ============================================================================ */

/**
 * Inline simple map - For when you don't need full pipeline machinery
 */
static inline void fp_simple_map_f64_inline(const double* input,
                                              double* output,
                                              size_t n,
                                              double (*fn)(double)) {
    if (!input || !output || !fn) return;

    for (size_t i = 0; i < n; i++) {
        output[i] = fn(input[i]);
    }
}

static inline void fp_simple_map_i64_inline(const int64_t* input,
                                              int64_t* output,
                                              size_t n,
                                              int64_t (*fn)(int64_t)) {
    if (!input || !output || !fn) return;

    for (size_t i = 0; i < n; i++) {
        output[i] = fn(input[i]);
    }
}

/**
 * Inline simple filter - For when you don't need full pipeline
 */
static inline size_t fp_simple_filter_f64_inline(const double* input,
                                                   double* output,
                                                   size_t n,
                                                   bool (*pred)(double)) {
    if (!input || !output || !pred) return 0;

    size_t write_idx = 0;
    for (size_t i = 0; i < n; i++) {
        if (pred(input[i])) {
            output[write_idx++] = input[i];
        }
    }
    return write_idx;
}

static inline size_t fp_simple_filter_i64_inline(const int64_t* input,
                                                   int64_t* output,
                                                   size_t n,
                                                   bool (*pred)(int64_t)) {
    if (!input || !output || !pred) return 0;

    size_t write_idx = 0;
    for (size_t i = 0; i < n; i++) {
        if (pred(input[i])) {
            output[write_idx++] = input[i];
        }
    }
    return write_idx;
}

/**
 * Inline simple reduce - For when you don't need full pipeline
 */
static inline double fp_simple_reduce_f64_inline(const double* input,
                                                   size_t n,
                                                   double init,
                                                   double (*fn)(double, double)) {
    if (!input || !fn) return init;

    double acc = init;
    for (size_t i = 0; i < n; i++) {
        acc = fn(acc, input[i]);
    }
    return acc;
}

static inline int64_t fp_simple_reduce_i64_inline(const int64_t* input,
                                                    size_t n,
                                                    int64_t init,
                                                    int64_t (*fn)(int64_t, int64_t)) {
    if (!input || !fn) return init;

    int64_t acc = init;
    for (size_t i = 0; i < n; i++) {
        acc = fn(acc, input[i]);
    }
    return acc;
}

/* ============================================================================
 * INLINE MAP-REDUCE FUSION (Single-pass optimization)
 * ============================================================================ */

/**
 * Inline fused map-reduce - Single pass, no temporary array
 * This is the key optimization that makes FP competitive with imperative code
 */
static inline double fp_fused_map_reduce_f64_inline(const double* input,
                                                      size_t n,
                                                      double (*map_fn)(double),
                                                      double init,
                                                      double (*reduce_fn)(double, double)) {
    if (!input || !map_fn || !reduce_fn) return init;

    double acc = init;
    for (size_t i = 0; i < n; i++) {
        double mapped = map_fn(input[i]);
        acc = reduce_fn(acc, mapped);
    }
    return acc;
}

static inline int64_t fp_fused_map_reduce_i64_inline(const int64_t* input,
                                                       size_t n,
                                                       int64_t (*map_fn)(int64_t),
                                                       int64_t init,
                                                       int64_t (*reduce_fn)(int64_t, int64_t)) {
    if (!input || !map_fn || !reduce_fn) return init;

    int64_t acc = init;
    for (size_t i = 0; i < n; i++) {
        int64_t mapped = map_fn(input[i]);
        acc = reduce_fn(acc, mapped);
    }
    return acc;
}

/**
 * Inline fused filter-reduce - Single pass
 */
static inline double fp_fused_filter_reduce_f64_inline(const double* input,
                                                         size_t n,
                                                         bool (*pred)(double),
                                                         double init,
                                                         double (*reduce_fn)(double, double)) {
    if (!input || !pred || !reduce_fn) return init;

    double acc = init;
    for (size_t i = 0; i < n; i++) {
        if (pred(input[i])) {
            acc = reduce_fn(acc, input[i]);
        }
    }
    return acc;
}

static inline int64_t fp_fused_filter_reduce_i64_inline(const int64_t* input,
                                                          size_t n,
                                                          bool (*pred)(int64_t),
                                                          int64_t init,
                                                          int64_t (*reduce_fn)(int64_t, int64_t)) {
    if (!input || !pred || !reduce_fn) return init;

    int64_t acc = init;
    for (size_t i = 0; i < n; i++) {
        if (pred(input[i])) {
            acc = reduce_fn(acc, input[i]);
        }
    }
    return acc;
}

/**
 * Inline fused map-filter-reduce - Triple fusion for maximum performance
 */
static inline double fp_fused_map_filter_reduce_f64_inline(const double* input,
                                                             size_t n,
                                                             double (*map_fn)(double),
                                                             bool (*pred)(double),
                                                             double init,
                                                             double (*reduce_fn)(double, double)) {
    if (!input || !map_fn || !pred || !reduce_fn) return init;

    double acc = init;
    for (size_t i = 0; i < n; i++) {
        double mapped = map_fn(input[i]);
        if (pred(mapped)) {
            acc = reduce_fn(acc, mapped);
        }
    }
    return acc;
}

/* ============================================================================
 * INLINE LAZY EVALUATION HELPERS
 * ============================================================================ */

/**
 * Inline lazy take - For small n, avoids overhead of lazy infrastructure
 */
static inline size_t fp_inline_take_f64(const double* input,
                                          double* output,
                                          size_t input_size,
                                          size_t take_count) {
    if (!input || !output) return 0;

    size_t n = (take_count < input_size) ? take_count : input_size;
    for (size_t i = 0; i < n; i++) {
        output[i] = input[i];
    }
    return n;
}

/**
 * Inline lazy drop - For small n
 */
static inline size_t fp_inline_drop_f64(const double* input,
                                          double* output,
                                          size_t input_size,
                                          size_t drop_count) {
    if (!input || !output || drop_count >= input_size) return 0;

    size_t remaining = input_size - drop_count;
    for (size_t i = 0; i < remaining; i++) {
        output[i] = input[drop_count + i];
    }
    return remaining;
}

/* ============================================================================
 * INLINE COMMON HIGHER-ORDER PATTERNS
 * ============================================================================ */

/**
 * Inline zipWith - Combine two arrays with binary function
 */
static inline void fp_inline_zip_with_f64(const double* a,
                                            const double* b,
                                            double* output,
                                            size_t n,
                                            double (*fn)(double, double)) {
    if (!a || !b || !output || !fn) return;

    for (size_t i = 0; i < n; i++) {
        output[i] = fn(a[i], b[i]);
    }
}

static inline void fp_inline_zip_with_i64(const int64_t* a,
                                            const int64_t* b,
                                            int64_t* output,
                                            size_t n,
                                            int64_t (*fn)(int64_t, int64_t)) {
    if (!a || !b || !output || !fn) return;

    for (size_t i = 0; i < n; i++) {
        output[i] = fn(a[i], b[i]);
    }
}

/**
 * Inline foreach - Side effects (I/O, logging, etc.)
 */
static inline void fp_inline_foreach_f64(const double* input,
                                           size_t n,
                                           void (*fn)(double)) {
    if (!input || !fn) return;

    for (size_t i = 0; i < n; i++) {
        fn(input[i]);
    }
}

static inline void fp_inline_foreach_i64(const int64_t* input,
                                           size_t n,
                                           void (*fn)(int64_t)) {
    if (!input || !fn) return;

    for (size_t i = 0; i < n; i++) {
        fn(input[i]);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* FP_COMPOSE_INLINE_H */
