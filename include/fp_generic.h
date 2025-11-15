/**
 * FP-ASM Generic Type System
 *
 * This module extends FP-ASM to support arbitrary types (not just i64/f64)
 * while maintaining strict functional purity guarantees.
 *
 * Design Philosophy:
 * - Type-generic operations using void* + element_size
 * - User-supplied comparison/operation functions
 * - Zero heap allocation (user provides buffers)
 * - Complete immutability (all inputs const)
 * - No side effects
 *
 * Enables FP composition for ANY type:
 * - Structs (Person, Product, etc.)
 * - Strings (char*)
 * - Custom data types
 * - Arrays of arbitrary elements
 *
 * Example Use Cases:
 * - Quick sort for any comparable type
 * - Map/filter/fold over struct arrays
 * - Generic data transformations
 * - Type-safe functional pipelines
 */

#ifndef FP_GENERIC_H
#define FP_GENERIC_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CATEGORY 12: GENERIC HIGHER-ORDER FUNCTIONS
 *
 * These functions work with ANY data type, not just i64/f64.
 * ============================================================================ */

/**
 * Generic fold left (reduce)
 *
 * Haskell equivalent:
 *   foldl :: (b -> a -> b) -> b -> [a] -> b
 *
 * @param input       Pointer to input array (any type)
 * @param n           Number of elements
 * @param elem_size   Size of each element in bytes
 * @param acc         Pointer to accumulator (will be modified in-place via callback)
 * @param fn          Reduction function: fn(acc, elem, context)
 *                    - acc: pointer to accumulator (read/write)
 *                    - elem: pointer to current element (read-only)
 *                    - context: user-supplied context
 * @param context     User context (closures)
 *
 * Example (sum of struct field):
 *   typedef struct { int id; double value; } Item;
 *   void sum_values(void* acc, const void* elem, void* ctx) {
 *       *(double*)acc += ((Item*)elem)->value;
 *   }
 *   double total = 0.0;
 *   fp_foldl_generic(items, n, sizeof(Item), &total, sum_values, NULL);
 */
void fp_foldl_generic(const void* input, size_t n, size_t elem_size,
                      void* acc,
                      void (*fn)(void* acc, const void* elem, void* ctx),
                      void* context);

/**
 * Generic map (transform)
 *
 * Haskell equivalent:
 *   map :: (a -> b) -> [a] -> [b]
 *
 * @param input       Pointer to input array (type A)
 * @param output      Pointer to output array (type B)
 * @param n           Number of elements
 * @param in_size     Size of input elements in bytes
 * @param out_size    Size of output elements in bytes
 * @param fn          Transform function: fn(out, in, context)
 *                    - out: pointer to output element (write)
 *                    - in: pointer to input element (read-only)
 *                    - context: user-supplied context
 * @param context     User context
 *
 * Example (extract field from struct):
 *   typedef struct { int id; double value; } Item;
 *   void extract_value(void* out, const void* in, void* ctx) {
 *       *(double*)out = ((Item*)in)->value;
 *   }
 *   double values[100];
 *   fp_map_generic(items, values, n, sizeof(Item), sizeof(double),
 *                  extract_value, NULL);
 */
void fp_map_generic(const void* input, void* output, size_t n,
                    size_t in_size, size_t out_size,
                    void (*fn)(void* out, const void* in, void* ctx),
                    void* context);

/**
 * Generic filter (select)
 *
 * Haskell equivalent:
 *   filter :: (a -> Bool) -> [a] -> [a]
 *
 * @param input       Pointer to input array
 * @param output      Pointer to output array (must be at least n elements)
 * @param n           Number of input elements
 * @param elem_size   Size of each element in bytes
 * @param predicate   Filter function: predicate(elem, context) -> bool
 * @param context     User context
 * @return            Number of elements in output array
 *
 * Example (filter by threshold):
 *   typedef struct { int id; double score; } Student;
 *   bool high_score(const void* elem, void* ctx) {
 *       return ((Student*)elem)->score >= *(double*)ctx;
 *   }
 *   double threshold = 90.0;
 *   Student high_scorers[100];
 *   size_t count = fp_filter_generic(students, high_scorers, n,
 *                                     sizeof(Student), high_score, &threshold);
 */
size_t fp_filter_generic(const void* input, void* output, size_t n,
                         size_t elem_size,
                         bool (*predicate)(const void* elem, void* ctx),
                         void* context);

/**
 * Generic zipWith (combine two arrays)
 *
 * Haskell equivalent:
 *   zipWith :: (a -> b -> c) -> [a] -> [b] -> [c]
 *
 * @param input_a     Pointer to first input array (type A)
 * @param input_b     Pointer to second input array (type B)
 * @param output      Pointer to output array (type C)
 * @param n           Number of elements (all arrays)
 * @param size_a      Size of type A in bytes
 * @param size_b      Size of type B in bytes
 * @param size_c      Size of type C in bytes
 * @param fn          Combiner function: fn(out, a, b, context)
 * @param context     User context
 *
 * Example (join two structs):
 *   typedef struct { int id; } Person;
 *   typedef struct { double salary; } Job;
 *   typedef struct { int id; double salary; } Employee;
 *   void join(void* out, const void* a, const void* b, void* ctx) {
 *       Employee* emp = (Employee*)out;
 *       emp->id = ((Person*)a)->id;
 *       emp->salary = ((Job*)b)->salary;
 *   }
 *   Employee employees[100];
 *   fp_zipWith_generic(persons, jobs, employees, n, sizeof(Person),
 *                      sizeof(Job), sizeof(Employee), join, NULL);
 */
void fp_zipWith_generic(const void* input_a, const void* input_b, void* output, size_t n,
                        size_t size_a, size_t size_b, size_t size_c,
                        void (*fn)(void* out, const void* a, const void* b, void* ctx),
                        void* context);

/* ============================================================================
 * CATEGORY 13: GENERIC SORTING AND COMPARISON
 *
 * Type-generic sorting functions maintaining functional purity.
 * ============================================================================ */

/**
 * Comparison function signature for generic sort
 *
 * Returns:
 *   < 0  if a < b
 *   = 0  if a == b
 *   > 0  if a > b
 *
 * Must be a pure function (no side effects, deterministic).
 */
typedef int (*fp_compare_fn)(const void* a, const void* b, void* context);

/**
 * Generic Quick Sort (functional, pure)
 *
 * Haskell equivalent:
 *   quicksort :: Ord a => [a] -> [a]
 *   quicksort [] = []
 *   quicksort (p:xs) = quicksort [x | x <- xs, x < p] ++ [p] ++ quicksort [x | x <- xs, x >= p]
 *
 * @param input       Input array (immutable)
 * @param output      Output array (sorted result, must be preallocated)
 * @param n           Number of elements
 * @param elem_size   Size of each element in bytes
 * @param compare     Comparison function
 * @param context     User context for comparison
 *
 * PURITY GUARANTEE:
 * - Input array is NEVER modified (const)
 * - All sorting done in output buffer
 * - No heap allocation
 * - Deterministic (same input = same output)
 *
 * Performance: O(n log n) average, O(n²) worst case
 * Stack usage: O(log n) average (tail-call optimization)
 *
 * Example:
 *   int compare_ints(const void* a, const void* b, void* ctx) {
 *       return *(int*)a - *(int*)b;
 *   }
 *   int sorted[100];
 *   fp_quicksort_generic(data, sorted, 100, sizeof(int), compare_ints, NULL);
 */
void fp_quicksort_generic(const void* input, void* output, size_t n,
                          size_t elem_size,
                          fp_compare_fn compare,
                          void* context);

/**
 * Generic Merge Sort (functional, pure, stable)
 *
 * Stable alternative to Quick Sort.
 *
 * @param input       Input array (immutable)
 * @param output      Output array (sorted result)
 * @param n           Number of elements
 * @param elem_size   Size of each element in bytes
 * @param compare     Comparison function
 * @param context     User context
 * @param temp        Temporary buffer (size: n * elem_size)
 *
 * Performance: O(n log n) guaranteed
 * Stability: Yes (equal elements maintain relative order)
 * Stack usage: O(log n)
 *
 * Requires temp buffer for merging (no heap allocation).
 */
void fp_mergesort_generic(const void* input, void* output, size_t n,
                          size_t elem_size,
                          fp_compare_fn compare,
                          void* context,
                          void* temp);

/* ============================================================================
 * CATEGORY 14: GENERIC LIST OPERATIONS
 *
 * Type-generic versions of common list operations.
 * ============================================================================ */

/**
 * Generic partition (split by predicate)
 *
 * Haskell equivalent:
 *   partition :: (a -> Bool) -> [a] -> ([a], [a])
 *
 * @param input       Input array
 * @param output_true Elements where predicate is true
 * @param output_false Elements where predicate is false
 * @param n           Number of input elements
 * @param elem_size   Size of each element
 * @param predicate   Filter function
 * @param context     User context
 * @param count_true  [OUT] Number of elements in output_true
 * @param count_false [OUT] Number of elements in output_false
 */
void fp_partition_generic(const void* input,
                          void* output_true, void* output_false, size_t n,
                          size_t elem_size,
                          bool (*predicate)(const void* elem, void* ctx),
                          void* context,
                          size_t* count_true, size_t* count_false);

/**
 * Generic take (first n elements)
 *
 * Haskell equivalent:
 *   take :: Int -> [a] -> [a]
 */
void fp_take_generic(const void* input, void* output, size_t n, size_t count,
                     size_t elem_size);

/**
 * Generic drop (skip first n elements)
 *
 * Haskell equivalent:
 *   drop :: Int -> [a] -> [a]
 */
size_t fp_drop_generic(const void* input, void* output, size_t n, size_t count,
                       size_t elem_size);

/**
 * Generic reverse
 *
 * Haskell equivalent:
 *   reverse :: [a] -> [a]
 */
void fp_reverse_generic(const void* input, void* output, size_t n,
                        size_t elem_size);

/**
 * Generic find (first element matching predicate)
 *
 * Haskell equivalent:
 *   find :: (a -> Bool) -> [a] -> Maybe a
 *
 * @param input       Input array
 * @param n           Number of elements
 * @param elem_size   Size of each element
 * @param predicate   Search predicate
 * @param context     User context
 * @param result      [OUT] Pointer to found element (if found)
 * @return            true if found, false otherwise
 */
bool fp_find_generic(const void* input, size_t n, size_t elem_size,
                     bool (*predicate)(const void* elem, void* ctx),
                     void* context,
                     void* result);

/* ============================================================================
 * CATEGORY 15: FUNCTION COMPOSITION
 *
 * Haskell-style function composition for building pipelines.
 * ============================================================================ */

/**
 * Generic function composition (map f . map g)
 *
 * Haskell equivalent:
 *   (.) :: (b -> c) -> (a -> b) -> (a -> c)
 *   compose f g = \x -> f (g x)
 *
 * Composes two functions by applying them sequentially to an array:
 *   result = map f (map g input)
 *
 * @param input       Input array (type A)
 * @param output      Output array (type C)
 * @param n           Number of elements
 * @param size_a      Size of type A in bytes
 * @param size_b      Size of intermediate type B in bytes
 * @param size_c      Size of output type C in bytes
 * @param g           First function to apply: g(b_out, a_in, ctx_g)  [a -> b]
 * @param ctx_g       Context for function g
 * @param f           Second function to apply: f(c_out, b_in, ctx_f) [b -> c]
 * @param ctx_f       Context for function f
 * @param temp        Temporary buffer for intermediate results (size: n * size_b)
 *
 * PURITY GUARANTEE:
 * - Input array is NEVER modified (const)
 * - All computation done via output and temp buffers
 * - No heap allocation (user provides buffers)
 * - Deterministic (same input = same output)
 *
 * Example (compose two transformations):
 *   // g: int -> double (multiply by 2)
 *   void double_it(void* out, const void* in, void* ctx) {
 *       *(double*)out = *(int*)in * 2.0;
 *   }
 *
 *   // f: double -> double (add 10)
 *   void add_10(void* out, const void* in, void* ctx) {
 *       *(double*)out = *(double*)in + 10.0;
 *   }
 *
 *   int input[100];
 *   double output[100];
 *   double temp[100];
 *   fp_compose_generic(input, output, 100,
 *                      sizeof(int), sizeof(double), sizeof(double),
 *                      double_it, NULL, add_10, NULL, temp);
 *   // Result: output[i] = (input[i] * 2.0) + 10.0
 */
void fp_compose_generic(const void* input, void* output, size_t n,
                        size_t size_a, size_t size_b, size_t size_c,
                        void (*g)(void* out, const void* in, void* ctx_g),
                        void* ctx_g,
                        void (*f)(void* out, const void* in, void* ctx_f),
                        void* ctx_f,
                        void* temp);

/* ============================================================================
 * HELPER MACROS FOR TYPE SAFETY
 * ============================================================================ */

/**
 * Type-safe wrapper for fp_quicksort_generic
 *
 * Usage:
 *   FP_QUICKSORT(MyStruct, data, sorted, n, compare_fn, ctx)
 *
 * Expands to:
 *   fp_quicksort_generic(data, sorted, n, sizeof(MyStruct), compare_fn, ctx)
 */
#define FP_QUICKSORT(TYPE, input, output, n, cmp, ctx) \
    fp_quicksort_generic((input), (output), (n), sizeof(TYPE), (cmp), (ctx))

#define FP_MERGESORT(TYPE, input, output, n, cmp, ctx, temp) \
    fp_mergesort_generic((input), (output), (n), sizeof(TYPE), (cmp), (ctx), (temp))

#define FP_MAP(IN_TYPE, OUT_TYPE, input, output, n, fn, ctx) \
    fp_map_generic((input), (output), (n), sizeof(IN_TYPE), sizeof(OUT_TYPE), (fn), (ctx))

#define FP_FILTER(TYPE, input, output, n, pred, ctx) \
    fp_filter_generic((input), (output), (n), sizeof(TYPE), (pred), (ctx))

#define FP_REVERSE(TYPE, input, output, n) \
    fp_reverse_generic((input), (output), (n), sizeof(TYPE))

#define FP_COMPOSE(TYPE_A, TYPE_B, TYPE_C, input, output, n, g, ctx_g, f, ctx_f, temp) \
    fp_compose_generic((input), (output), (n), sizeof(TYPE_A), sizeof(TYPE_B), sizeof(TYPE_C), (g), (ctx_g), (f), (ctx_f), (temp))

#ifdef __cplusplus
}
#endif

#endif /* FP_GENERIC_H */
