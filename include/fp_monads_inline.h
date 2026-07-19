/**
 * fp_monads_inline.h - Inline Optimizations for Hot Paths
 *
 * Provides inline versions of frequently-used monad operations
 * for zero-overhead abstractions in performance-critical code.
 *
 * Usage:
 *   Include this AFTER fp_monads.h in hot-path code.
 *   The compiler will inline these for maximum performance.
 */

#ifndef FP_MONADS_INLINE_H
#define FP_MONADS_INLINE_H

#include "fp_monads.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * INLINE MAYBE OPERATIONS (Hot Path)
 * ============================================================================ */

/**
 * Inline Maybe constructors - Zero overhead
 */
static inline Maybe fp_just_f64_inline(double value) {
    Maybe m = { .tag = FP_JUST, .value_f64 = value };
    return m;
}

static inline Maybe fp_just_i64_inline(int64_t value) {
    Maybe m = { .tag = FP_JUST, .value_i64 = value };
    return m;
}

static inline Maybe fp_nothing_inline(void) {
    Maybe m = { .tag = FP_NOTHING, .value_f64 = 0.0 };
    return m;
}

/**
 * Inline Maybe predicates - Compiles to single comparison
 */
static inline bool fp_is_just_inline(Maybe m) {
    return m.tag == FP_JUST;
}

static inline bool fp_is_nothing_inline(Maybe m) {
    return m.tag == FP_NOTHING;
}

/**
 * Inline Maybe accessors - Safe with default (hot path)
 */
static inline double fp_from_maybe_f64_inline(Maybe m, double default_val) {
    return (m.tag == FP_JUST) ? m.value_f64 : default_val;
}

static inline int64_t fp_from_maybe_i64_inline(Maybe m, int64_t default_val) {
    return (m.tag == FP_JUST) ? m.value_i64 : default_val;
}

/**
 * Inline Maybe fmap - Zero overhead when successful
 */
static inline Maybe fp_fmap_maybe_f64_inline(Maybe m, double (*fn)(double)) {
    if (m.tag == FP_NOTHING || !fn) return fp_nothing_inline();
    return fp_just_f64_inline(fn(m.value_f64));
}

static inline Maybe fp_fmap_maybe_i64_inline(Maybe m, int64_t (*fn)(int64_t)) {
    if (m.tag == FP_NOTHING || !fn) return fp_nothing_inline();
    return fp_just_i64_inline(fn(m.value_i64));
}

/* ============================================================================
 * INLINE EITHER OPERATIONS (Hot Path)
 * ============================================================================ */

/**
 * Inline Either constructors
 */
static inline Either fp_right_f64_inline(double value) {
    Either e = {
        .tag = FP_RIGHT,
        .right = { .value_f64 = value, .value_i64 = 0, .value_ptr = NULL }
    };
    return e;
}

static inline Either fp_right_i64_inline(int64_t value) {
    Either e = {
        .tag = FP_RIGHT,
        .right = { .value_f64 = 0.0, .value_i64 = value, .value_ptr = NULL }
    };
    return e;
}

static inline Either fp_left_inline(const char* error_msg, int error_code) {
    Either e = {
        .tag = FP_LEFT,
        .left = { .error_msg = error_msg, .error_code = error_code }
    };
    return e;
}

/**
 * Inline Either predicates
 */
static inline bool fp_is_right_inline(Either e) {
    return e.tag == FP_RIGHT;
}

static inline bool fp_is_left_inline(Either e) {
    return e.tag == FP_LEFT;
}

/**
 * Inline Either accessors (hot path)
 */
static inline double fp_from_right_f64_inline(Either e) {
    return (e.tag == FP_RIGHT) ? e.right.value_f64 : 0.0;
}

static inline int64_t fp_from_right_i64_inline(Either e) {
    return (e.tag == FP_RIGHT) ? e.right.value_i64 : 0;
}

/**
 * Inline Either fmap - Propagates errors with zero overhead
 */
static inline Either fp_fmap_either_f64_inline(Either e, double (*fn)(double)) {
    if (e.tag == FP_LEFT || !fn) return e;
    return fp_right_f64_inline(fn(e.right.value_f64));
}

static inline Either fp_fmap_either_i64_inline(Either e, int64_t (*fn)(int64_t)) {
    if (e.tag == FP_LEFT || !fn) return e;
    return fp_right_i64_inline(fn(e.right.value_i64));
}

/* ============================================================================
 * INLINE SAFE OPERATIONS (Hot Path - Most Frequently Used)
 * ============================================================================ */

/**
 * Inline safe division - Compiles to branch + fdiv
 */
static inline Maybe fp_safe_divide_f64_inline(double numerator, double denominator) {
    if (denominator == 0.0) return fp_nothing_inline();
    return fp_just_f64_inline(numerator / denominator);
}

static inline Maybe fp_safe_divide_i64_inline(int64_t numerator, int64_t denominator) {
    if (denominator == 0) return fp_nothing_inline();
    if (numerator == INT64_MIN && denominator == -1) return fp_nothing_inline();
    return fp_just_i64_inline(numerator / denominator);
}

/**
 * Inline safe array access - Compiles to bounds check + load
 */
static inline Maybe fp_safe_at_f64_inline(const double* array, size_t size, size_t index) {
    if (!array || index >= size) return fp_nothing_inline();
    return fp_just_f64_inline(array[index]);
}

static inline Maybe fp_safe_at_i64_inline(const int64_t* array, size_t size, size_t index) {
    if (!array || index >= size) return fp_nothing_inline();
    return fp_just_i64_inline(array[index]);
}

/**
 * Inline safe head - Compiles to null check + load
 */
static inline Maybe fp_safe_head_f64_inline(const double* array, size_t size) {
    if (!array || size == 0) return fp_nothing_inline();
    return fp_just_f64_inline(array[0]);
}

static inline Maybe fp_safe_head_i64_inline(const int64_t* array, size_t size) {
    if (!array || size == 0) return fp_nothing_inline();
    return fp_just_i64_inline(array[0]);
}

/* ============================================================================
 * INLINE CHECKED OPERATIONS (Either variants for hot paths)
 * ============================================================================ */

/**
 * Inline checked division - Error messages included
 */
static inline Either fp_checked_divide_f64_inline(double numerator, double denominator) {
    if (denominator == 0.0) return fp_left_inline("Division by zero", -1);
    return fp_right_f64_inline(numerator / denominator);
}

static inline Either fp_checked_divide_i64_inline(int64_t numerator, int64_t denominator) {
    if (denominator == 0) return fp_left_inline("Division by zero", -1);
    if (numerator == INT64_MIN && denominator == -1) {
        return fp_left_inline("Integer overflow: INT64_MIN / -1", -2);
    }
    return fp_right_i64_inline(numerator / denominator);
}

/**
 * Inline checked array access
 */
static inline Either fp_checked_at_f64_inline(const double* array, size_t size, size_t index) {
    if (!array) return fp_left_inline("Null pointer dereference", -1);
    if (index >= size) return fp_left_inline("Array index out of bounds", -2);
    return fp_right_f64_inline(array[index]);
}

static inline Either fp_checked_at_i64_inline(const int64_t* array, size_t size, size_t index) {
    if (!array) return fp_left_inline("Null pointer dereference", -1);
    if (index >= size) return fp_left_inline("Array index out of bounds", -2);
    return fp_right_i64_inline(array[index]);
}

/* ============================================================================
 * INLINE UTILITIES
 * ============================================================================ */

/**
 * Inline catMaybes for small arrays - Loop unrolling friendly
 */
static inline size_t fp_cat_maybes_f64_inline(const Maybe* maybes, size_t n, double* out_array) {
    if (!maybes || !out_array) return 0;

    size_t write_idx = 0;
    for (size_t i = 0; i < n; i++) {
        if (maybes[i].tag == FP_JUST) {
            out_array[write_idx++] = maybes[i].value_f64;
        }
    }
    return write_idx;
}

static inline size_t fp_cat_maybes_i64_inline(const Maybe* maybes, size_t n, int64_t* out_array) {
    if (!maybes || !out_array) return 0;

    size_t write_idx = 0;
    for (size_t i = 0; i < n; i++) {
        if (maybes[i].tag == FP_JUST) {
            out_array[write_idx++] = maybes[i].value_i64;
        }
    }
    return write_idx;
}

#ifdef __cplusplus
}
#endif

#endif /* FP_MONADS_INLINE_H */
