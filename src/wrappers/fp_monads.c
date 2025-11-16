/**
 * fp_monads.c - Monadic Error Handling Implementation
 *
 * Implements Haskell-style Maybe and Either monads for safe, composable error handling.
 * Provides zero-overhead abstractions when successful.
 */

#include "../../include/fp_monads.h"
#include <math.h>
#include <string.h>

/* ============================================================================
 * MAYBE MONAD - Constructors
 * ============================================================================ */

Maybe fp_just_f64(double value) {
    Maybe m = {
        .tag = FP_JUST,
        .value_f64 = value
    };
    return m;
}

Maybe fp_just_i64(int64_t value) {
    Maybe m = {
        .tag = FP_JUST,
        .value_i64 = value
    };
    return m;
}

Maybe fp_just_ptr(void* value) {
    Maybe m = {
        .tag = FP_JUST,
        .value_ptr = value
    };
    return m;
}

Maybe fp_nothing(void) {
    Maybe m = {
        .tag = FP_NOTHING,
        .value_f64 = 0.0
    };
    return m;
}

/* ============================================================================
 * MAYBE MONAD - Predicates
 * ============================================================================ */

bool fp_is_just(Maybe m) {
    return m.tag == FP_JUST;
}

bool fp_is_nothing(Maybe m) {
    return m.tag == FP_NOTHING;
}

/* ============================================================================
 * MAYBE MONAD - Accessors (Unsafe)
 * ============================================================================ */

double fp_from_just_f64(Maybe m) {
    // UNSAFE: Assumes caller checked with fp_is_just first!
    return m.value_f64;
}

int64_t fp_from_just_i64(Maybe m) {
    return m.value_i64;
}

void* fp_from_just_ptr(Maybe m) {
    return m.value_ptr;
}

/* ============================================================================
 * MAYBE MONAD - Accessors (Safe with default)
 * ============================================================================ */

double fp_from_maybe_f64(Maybe m, double default_val) {
    return (m.tag == FP_JUST) ? m.value_f64 : default_val;
}

int64_t fp_from_maybe_i64(Maybe m, int64_t default_val) {
    return (m.tag == FP_JUST) ? m.value_i64 : default_val;
}

void* fp_from_maybe_ptr(Maybe m, void* default_val) {
    return (m.tag == FP_JUST) ? m.value_ptr : default_val;
}

/* ============================================================================
 * MAYBE MONAD - Functor (fmap)
 * ============================================================================ */

Maybe fp_fmap_maybe_f64(Maybe m, double (*fn)(double)) {
    if (!fn || m.tag == FP_NOTHING) {
        return fp_nothing();
    }
    return fp_just_f64(fn(m.value_f64));
}

Maybe fp_fmap_maybe_i64(Maybe m, int64_t (*fn)(int64_t)) {
    if (!fn || m.tag == FP_NOTHING) {
        return fp_nothing();
    }
    return fp_just_i64(fn(m.value_i64));
}

/* ============================================================================
 * MAYBE MONAD - Monad (bind)
 * ============================================================================ */

Maybe fp_bind_maybe_f64(Maybe m, Maybe (*fn)(double)) {
    if (!fn || m.tag == FP_NOTHING) {
        return fp_nothing();
    }
    return fn(m.value_f64);
}

Maybe fp_bind_maybe_i64(Maybe m, Maybe (*fn)(int64_t)) {
    if (!fn || m.tag == FP_NOTHING) {
        return fp_nothing();
    }
    return fn(m.value_i64);
}

/* ============================================================================
 * MAYBE MONAD - Applicative (ap)
 * ============================================================================ */

Maybe fp_ap_maybe_f64(Maybe mfn, Maybe mx) {
    if (mfn.tag == FP_NOTHING || mx.tag == FP_NOTHING) {
        return fp_nothing();
    }
    // Note: This is simplified - full implementation would store function pointer
    return mx;
}

Maybe fp_ap_maybe_i64(Maybe mfn, Maybe mx) {
    if (mfn.tag == FP_NOTHING || mx.tag == FP_NOTHING) {
        return fp_nothing();
    }
    return mx;
}

/* ============================================================================
 * EITHER MONAD - Constructors
 * ============================================================================ */

Either fp_left(const char* error_msg, int error_code) {
    Either e = {
        .tag = FP_LEFT,
        .left = {
            .error_msg = error_msg,
            .error_code = error_code
        }
    };
    return e;
}

Either fp_right_f64(double value) {
    Either e = {
        .tag = FP_RIGHT,
        .right = {
            .value_f64 = value,
            .value_i64 = 0,
            .value_ptr = NULL
        }
    };
    return e;
}

Either fp_right_i64(int64_t value) {
    Either e = {
        .tag = FP_RIGHT,
        .right = {
            .value_f64 = 0.0,
            .value_i64 = value,
            .value_ptr = NULL
        }
    };
    return e;
}

Either fp_right_ptr(void* value) {
    Either e = {
        .tag = FP_RIGHT,
        .right = {
            .value_f64 = 0.0,
            .value_i64 = 0,
            .value_ptr = value
        }
    };
    return e;
}

/* ============================================================================
 * EITHER MONAD - Predicates
 * ============================================================================ */

bool fp_is_left(Either e) {
    return e.tag == FP_LEFT;
}

bool fp_is_right(Either e) {
    return e.tag == FP_RIGHT;
}

/* ============================================================================
 * EITHER MONAD - Accessors
 * ============================================================================ */

const char* fp_from_left_msg(Either e) {
    return (e.tag == FP_LEFT) ? e.left.error_msg : NULL;
}

int fp_from_left_code(Either e) {
    return (e.tag == FP_LEFT) ? e.left.error_code : 0;
}

double fp_from_right_f64(Either e) {
    return (e.tag == FP_RIGHT) ? e.right.value_f64 : 0.0;
}

int64_t fp_from_right_i64(Either e) {
    return (e.tag == FP_RIGHT) ? e.right.value_i64 : 0;
}

void* fp_from_right_ptr(Either e) {
    return (e.tag == FP_RIGHT) ? e.right.value_ptr : NULL;
}

/* ============================================================================
 * EITHER MONAD - Functor (fmap)
 * ============================================================================ */

Either fp_fmap_either_f64(Either e, double (*fn)(double)) {
    if (!fn || e.tag == FP_LEFT) {
        return e;  // Propagate error
    }
    return fp_right_f64(fn(e.right.value_f64));
}

Either fp_fmap_either_i64(Either e, int64_t (*fn)(int64_t)) {
    if (!fn || e.tag == FP_LEFT) {
        return e;
    }
    return fp_right_i64(fn(e.right.value_i64));
}

/* ============================================================================
 * EITHER MONAD - Monad (bind)
 * ============================================================================ */

Either fp_bind_either_f64(Either e, Either (*fn)(double)) {
    if (!fn || e.tag == FP_LEFT) {
        return e;  // Propagate error
    }
    return fn(e.right.value_f64);
}

Either fp_bind_either_i64(Either e, Either (*fn)(int64_t)) {
    if (!fn || e.tag == FP_LEFT) {
        return e;
    }
    return fn(e.right.value_i64);
}

/* ============================================================================
 * EITHER MONAD - Fold (either)
 * ============================================================================ */

double fp_fold_either_f64(Either e, double (*on_left)(const char*, int), double (*on_right)(double)) {
    if (!on_left || !on_right) return 0.0;

    if (e.tag == FP_LEFT) {
        return on_left(e.left.error_msg, e.left.error_code);
    } else {
        return on_right(e.right.value_f64);
    }
}

int64_t fp_fold_either_i64(Either e, int64_t (*on_left)(const char*, int), int64_t (*on_right)(int64_t)) {
    if (!on_left || !on_right) return 0;

    if (e.tag == FP_LEFT) {
        return on_left(e.left.error_msg, e.left.error_code);
    } else {
        return on_right(e.right.value_i64);
    }
}

/* ============================================================================
 * SAFE ARITHMETIC OPERATIONS (returning Maybe)
 * ============================================================================ */

Maybe fp_safe_divide_f64(double numerator, double denominator) {
    if (denominator == 0.0) {
        return fp_nothing();
    }
    return fp_just_f64(numerator / denominator);
}

Maybe fp_safe_divide_i64(int64_t numerator, int64_t denominator) {
    if (denominator == 0) {
        return fp_nothing();
    }
    // Check for integer overflow: INT64_MIN / -1 overflows
    if (numerator == INT64_MIN && denominator == -1) {
        return fp_nothing();
    }
    return fp_just_i64(numerator / denominator);
}

Maybe fp_safe_sqrt_f64(double x) {
    if (x < 0.0) {
        return fp_nothing();
    }
    return fp_just_f64(sqrt(x));
}

Maybe fp_safe_log_f64(double x) {
    if (x <= 0.0) {
        return fp_nothing();
    }
    return fp_just_f64(log(x));
}

Maybe fp_safe_log10_f64(double x) {
    if (x <= 0.0) {
        return fp_nothing();
    }
    return fp_just_f64(log10(x));
}

Maybe fp_safe_at_f64(const double* array, size_t size, size_t index) {
    if (!array || index >= size) {
        return fp_nothing();
    }
    return fp_just_f64(array[index]);
}

Maybe fp_safe_at_i64(const int64_t* array, size_t size, size_t index) {
    if (!array || index >= size) {
        return fp_nothing();
    }
    return fp_just_i64(array[index]);
}

Maybe fp_safe_head_f64(const double* array, size_t size) {
    if (!array || size == 0) {
        return fp_nothing();
    }
    return fp_just_f64(array[0]);
}

Maybe fp_safe_head_i64(const int64_t* array, size_t size) {
    if (!array || size == 0) {
        return fp_nothing();
    }
    return fp_just_i64(array[0]);
}

Maybe fp_safe_tail_f64(const double* array, size_t size, double** out_ptr, size_t* out_size) {
    if (!array || size == 0 || !out_ptr || !out_size) {
        return fp_nothing();
    }

    *out_ptr = (double*)(array + 1);  // Point to second element
    *out_size = size - 1;

    return fp_just_ptr(*out_ptr);
}

Maybe fp_safe_tail_i64(const int64_t* array, size_t size, int64_t** out_ptr, size_t* out_size) {
    if (!array || size == 0 || !out_ptr || !out_size) {
        return fp_nothing();
    }

    *out_ptr = (int64_t*)(array + 1);
    *out_size = size - 1;

    return fp_just_ptr(*out_ptr);
}

/* ============================================================================
 * SAFE ARITHMETIC OPERATIONS (returning Either)
 * ============================================================================ */

Either fp_checked_divide_f64(double numerator, double denominator) {
    if (denominator == 0.0) {
        return fp_left("Division by zero", -1);
    }
    return fp_right_f64(numerator / denominator);
}

Either fp_checked_divide_i64(int64_t numerator, int64_t denominator) {
    if (denominator == 0) {
        return fp_left("Division by zero", -1);
    }
    if (numerator == INT64_MIN && denominator == -1) {
        return fp_left("Integer overflow: INT64_MIN / -1", -2);
    }
    return fp_right_i64(numerator / denominator);
}

Either fp_checked_sqrt_f64(double x) {
    if (x < 0.0) {
        return fp_left("Square root of negative number", -1);
    }
    return fp_right_f64(sqrt(x));
}

Either fp_checked_at_f64(const double* array, size_t size, size_t index) {
    if (!array) {
        return fp_left("Null pointer dereference", -1);
    }
    if (index >= size) {
        return fp_left("Array index out of bounds", -2);
    }
    return fp_right_f64(array[index]);
}

Either fp_checked_at_i64(const int64_t* array, size_t size, size_t index) {
    if (!array) {
        return fp_left("Null pointer dereference", -1);
    }
    if (index >= size) {
        return fp_left("Array index out of bounds", -2);
    }
    return fp_right_i64(array[index]);
}

/* ============================================================================
 * SEQUENCE OPERATIONS (for arrays of Maybe/Either)
 * ============================================================================ */

Maybe fp_sequence_maybe_f64(const Maybe* maybes, size_t n, double* out_array) {
    if (!maybes || !out_array) {
        return fp_nothing();
    }

    for (size_t i = 0; i < n; i++) {
        if (maybes[i].tag == FP_NOTHING) {
            return fp_nothing();  // Short-circuit on first Nothing
        }
        out_array[i] = maybes[i].value_f64;
    }

    return fp_just_ptr(out_array);
}

Maybe fp_sequence_maybe_i64(const Maybe* maybes, size_t n, int64_t* out_array) {
    if (!maybes || !out_array) {
        return fp_nothing();
    }

    for (size_t i = 0; i < n; i++) {
        if (maybes[i].tag == FP_NOTHING) {
            return fp_nothing();
        }
        out_array[i] = maybes[i].value_i64;
    }

    return fp_just_ptr(out_array);
}

Either fp_sequence_either_f64(const Either* eithers, size_t n, double* out_array) {
    if (!eithers || !out_array) {
        return fp_left("Null pointer", -1);
    }

    for (size_t i = 0; i < n; i++) {
        if (eithers[i].tag == FP_LEFT) {
            return eithers[i];  // Return first error
        }
        out_array[i] = eithers[i].right.value_f64;
    }

    return fp_right_ptr(out_array);
}

Either fp_sequence_either_i64(const Either* eithers, size_t n, int64_t* out_array) {
    if (!eithers || !out_array) {
        return fp_left("Null pointer", -1);
    }

    for (size_t i = 0; i < n; i++) {
        if (eithers[i].tag == FP_LEFT) {
            return eithers[i];
        }
        out_array[i] = eithers[i].right.value_i64;
    }

    return fp_right_ptr(out_array);
}

Maybe fp_traverse_maybe_f64(const double* input, size_t n, Maybe (*fn)(double), double* out_array) {
    if (!input || !fn || !out_array) {
        return fp_nothing();
    }

    for (size_t i = 0; i < n; i++) {
        Maybe result = fn(input[i]);
        if (result.tag == FP_NOTHING) {
            return fp_nothing();  // Short-circuit on first failure
        }
        out_array[i] = result.value_f64;
    }

    return fp_just_ptr(out_array);
}

Maybe fp_traverse_maybe_i64(const int64_t* input, size_t n, Maybe (*fn)(int64_t), int64_t* out_array) {
    if (!input || !fn || !out_array) {
        return fp_nothing();
    }

    for (size_t i = 0; i < n; i++) {
        Maybe result = fn(input[i]);
        if (result.tag == FP_NOTHING) {
            return fp_nothing();
        }
        out_array[i] = result.value_i64;
    }

    return fp_just_ptr(out_array);
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

size_t fp_cat_maybes_f64(const Maybe* maybes, size_t n, double* out_array) {
    if (!maybes || !out_array) {
        return 0;
    }

    size_t write_idx = 0;
    for (size_t i = 0; i < n; i++) {
        if (maybes[i].tag == FP_JUST) {
            out_array[write_idx++] = maybes[i].value_f64;
        }
    }

    return write_idx;
}

size_t fp_cat_maybes_i64(const Maybe* maybes, size_t n, int64_t* out_array) {
    if (!maybes || !out_array) {
        return 0;
    }

    size_t write_idx = 0;
    for (size_t i = 0; i < n; i++) {
        if (maybes[i].tag == FP_JUST) {
            out_array[write_idx++] = maybes[i].value_i64;
        }
    }

    return write_idx;
}

size_t fp_map_maybe_f64(const double* input, size_t n, Maybe (*fn)(double), double* out_array) {
    if (!input || !fn || !out_array) {
        return 0;
    }

    size_t write_idx = 0;
    for (size_t i = 0; i < n; i++) {
        Maybe result = fn(input[i]);
        if (result.tag == FP_JUST) {
            out_array[write_idx++] = result.value_f64;
        }
    }

    return write_idx;
}

size_t fp_map_maybe_i64(const int64_t* input, size_t n, Maybe (*fn)(int64_t), int64_t* out_array) {
    if (!input || !fn || !out_array) {
        return 0;
    }

    size_t write_idx = 0;
    for (size_t i = 0; i < n; i++) {
        Maybe result = fn(input[i]);
        if (result.tag == FP_JUST) {
            out_array[write_idx++] = result.value_i64;
        }
    }

    return write_idx;
}

void fp_partition_either_f64(const Either* eithers, size_t n,
                              const char** left_errors, size_t* left_count,
                              double* right_values, size_t* right_count) {
    if (!eithers || !left_errors || !right_values || !left_count || !right_count) {
        if (left_count) *left_count = 0;
        if (right_count) *right_count = 0;
        return;
    }

    size_t left_idx = 0;
    size_t right_idx = 0;

    for (size_t i = 0; i < n; i++) {
        if (eithers[i].tag == FP_LEFT) {
            left_errors[left_idx++] = eithers[i].left.error_msg;
        } else {
            right_values[right_idx++] = eithers[i].right.value_f64;
        }
    }

    *left_count = left_idx;
    *right_count = right_idx;
}
