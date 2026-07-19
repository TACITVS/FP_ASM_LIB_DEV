/**
 * fp_monads.h - Monadic Error Handling for C
 * 
 * Brings Haskell-style Maybe and Either monads to C.
 * Enables safe, composable error handling without exceptions.
 * 
 * Features:
 * - Maybe monad (for optional values)
 * - Either monad (for error values)
 * - Monadic operations (bind, fmap, sequence)
 * - Safe arithmetic operations
 * 
 * Example:
 *   Maybe result = fp_safe_divide_f64(10.0, 2.0)
 *       ->bind(fp_safe_sqrt)
 *       ->bind(fp_safe_log)
 *       ->map(double_it);
 *   
 *   if (is_just(result)) {
 *       printf("Success: %f\n", from_just(result));
 *   } else {
 *       printf("Error in computation\n");
 *   }
 */

#ifndef FP_MONADS_H
#define FP_MONADS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * MAYBE MONAD (Optional values)
 * ============================================================================ */

// Maybe type: Just value | Nothing
typedef enum {
    FP_NOTHING,
    FP_JUST
} fp_maybe_tag_t;

typedef struct {
    fp_maybe_tag_t tag;
    union {
        double value_f64;
        int64_t value_i64;
        void* value_ptr;
    };
} Maybe;

// Maybe constructors
Maybe fp_just_f64(double value);
Maybe fp_just_i64(int64_t value);
Maybe fp_just_ptr(void* value);
Maybe fp_nothing(void);

// Maybe predicates
bool fp_is_just(Maybe m);
bool fp_is_nothing(Maybe m);

// Maybe accessors (unsafe - use with is_just check!)
double fp_from_just_f64(Maybe m);
int64_t fp_from_just_i64(Maybe m);
void* fp_from_just_ptr(Maybe m);

// Maybe accessors (safe - with default)
double fp_from_maybe_f64(Maybe m, double default_val);
int64_t fp_from_maybe_i64(Maybe m, int64_t default_val);
void* fp_from_maybe_ptr(Maybe m, void* default_val);

// Maybe functor: fmap :: (a -> b) -> Maybe a -> Maybe b
Maybe fp_fmap_maybe_f64(Maybe m, double (*fn)(double));
Maybe fp_fmap_maybe_i64(Maybe m, int64_t (*fn)(int64_t));

// Maybe monad: bind :: Maybe a -> (a -> Maybe b) -> Maybe b
Maybe fp_bind_maybe_f64(Maybe m, Maybe (*fn)(double));
Maybe fp_bind_maybe_i64(Maybe m, Maybe (*fn)(int64_t));

// Maybe applicative: ap :: Maybe (a -> b) -> Maybe a -> Maybe b
Maybe fp_ap_maybe_f64(Maybe mfn, Maybe mx);
Maybe fp_ap_maybe_i64(Maybe mfn, Maybe mx);

/* ============================================================================
 * EITHER MONAD (Error handling with values)
 * ============================================================================ */

// Either type: Left error | Right value
typedef enum {
    FP_LEFT,   // Error case
    FP_RIGHT   // Success case
} fp_either_tag_t;

typedef struct {
    fp_either_tag_t tag;
    union {
        struct {
            const char* error_msg;
            int error_code;
        } left;
        struct {
            double value_f64;
            int64_t value_i64;
            void* value_ptr;
        } right;
    };
} Either;

// Either constructors
Either fp_left(const char* error_msg, int error_code);
Either fp_right_f64(double value);
Either fp_right_i64(int64_t value);
Either fp_right_ptr(void* value);

// Either predicates
bool fp_is_left(Either e);
bool fp_is_right(Either e);

// Either accessors
const char* fp_from_left_msg(Either e);
int fp_from_left_code(Either e);
double fp_from_right_f64(Either e);
int64_t fp_from_right_i64(Either e);
void* fp_from_right_ptr(Either e);

// Either functor: fmap :: (a -> b) -> Either e a -> Either e b
Either fp_fmap_either_f64(Either e, double (*fn)(double));
Either fp_fmap_either_i64(Either e, int64_t (*fn)(int64_t));

// Either monad: bind :: Either e a -> (a -> Either e b) -> Either e b
Either fp_bind_either_f64(Either e, Either (*fn)(double));
Either fp_bind_either_i64(Either e, Either (*fn)(int64_t));

// Either fold: either :: (e -> c) -> (a -> c) -> Either e a -> c
double fp_fold_either_f64(Either e, double (*on_left)(const char*, int), double (*on_right)(double));
int64_t fp_fold_either_i64(Either e, int64_t (*on_left)(const char*, int), int64_t (*on_right)(int64_t));

/* ============================================================================
 * SAFE ARITHMETIC OPERATIONS (returning Maybe)
 * ============================================================================ */

// Safe division (returns Nothing on division by zero)
Maybe fp_safe_divide_f64(double numerator, double denominator);
Maybe fp_safe_divide_i64(int64_t numerator, int64_t denominator);

// Safe square root (returns Nothing on negative input)
Maybe fp_safe_sqrt_f64(double x);

// Safe logarithm (returns Nothing on non-positive input)
Maybe fp_safe_log_f64(double x);
Maybe fp_safe_log10_f64(double x);

// Safe array access (returns Nothing on out-of-bounds)
Maybe fp_safe_at_f64(const double* array, size_t size, size_t index);
Maybe fp_safe_at_i64(const int64_t* array, size_t size, size_t index);

// Safe head/tail (returns Nothing on empty array)
Maybe fp_safe_head_f64(const double* array, size_t size);
Maybe fp_safe_head_i64(const int64_t* array, size_t size);
Maybe fp_safe_tail_f64(const double* array, size_t size, double** out_ptr, size_t* out_size);
Maybe fp_safe_tail_i64(const int64_t* array, size_t size, int64_t** out_ptr, size_t* out_size);

/* ============================================================================
 * SAFE ARITHMETIC OPERATIONS (returning Either)
 * ============================================================================ */

// Division with error message
Either fp_checked_divide_f64(double numerator, double denominator);
Either fp_checked_divide_i64(int64_t numerator, int64_t denominator);

// Square root with error message
Either fp_checked_sqrt_f64(double x);

// Array access with error message
Either fp_checked_at_f64(const double* array, size_t size, size_t index);
Either fp_checked_at_i64(const int64_t* array, size_t size, size_t index);

/* ============================================================================
 * SEQUENCE OPERATIONS (for arrays of Maybe/Either)
 * ============================================================================ */

// sequence :: [Maybe a] -> Maybe [a]
// Converts array of Maybe into Maybe of array
// Returns Nothing if any element is Nothing
Maybe fp_sequence_maybe_f64(const Maybe* maybes, size_t n, double* out_array);
Maybe fp_sequence_maybe_i64(const Maybe* maybes, size_t n, int64_t* out_array);

// sequence :: [Either e a] -> Either e [a]
// Converts array of Either into Either of array
// Returns first Left if any element is Left
Either fp_sequence_either_f64(const Either* eithers, size_t n, double* out_array);
Either fp_sequence_either_i64(const Either* eithers, size_t n, int64_t* out_array);

// traverse :: (a -> Maybe b) -> [a] -> Maybe [b]
// Map function returning Maybe over array, collect results
Maybe fp_traverse_maybe_f64(const double* input, size_t n, Maybe (*fn)(double), double* out_array);
Maybe fp_traverse_maybe_i64(const int64_t* input, size_t n, Maybe (*fn)(int64_t), int64_t* out_array);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

// catMaybes :: [Maybe a] -> [a]
// Filter out Nothing values, extract Just values
size_t fp_cat_maybes_f64(const Maybe* maybes, size_t n, double* out_array);
size_t fp_cat_maybes_i64(const Maybe* maybes, size_t n, int64_t* out_array);

// mapMaybe :: (a -> Maybe b) -> [a] -> [b]
// Map function returning Maybe, keep only Just results
size_t fp_map_maybe_f64(const double* input, size_t n, Maybe (*fn)(double), double* out_array);
size_t fp_map_maybe_i64(const int64_t* input, size_t n, Maybe (*fn)(int64_t), int64_t* out_array);

// partition :: [Either e a] -> ([e], [a])
// Separate lefts and rights
void fp_partition_either_f64(
    const Either* eithers, size_t n,
    const char** left_errors, size_t* left_count,
    double* right_values, size_t* right_count
);

#endif /* FP_MONADS_H */
