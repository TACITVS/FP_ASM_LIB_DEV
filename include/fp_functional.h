/**
 * fp_functional.h
 *
 * Extended functional-programming operations (callback-based, plain C).
 *
 * These complement the array/SIMD primitives and the four base HOFs in
 * fp_general_hof.c (foldl / map / filter / zipWith) with the operations that
 * FP programmers routinely reach for: right folds, scans, predicate-based
 * slicing and partitioning, the Foldable queries, a list-monad bind
 * (concatMap), and the anamorphic generators (iterate / unfoldr).
 *
 * Convention (matches fp_general_hof.c):
 *   - i64 and f64 variants; user functions take a trailing `void* ctx`.
 *   - Functions that produce arrays write into a caller-provided `output`
 *     buffer and return the number of elements written.
 *
 * These are deliberately plain C: arbitrary function-pointer callbacks can't be
 * inlined into the SIMD kernels. Use the specialized kernels (fp_reduce_*,
 * fp_map_*, ...) on hot paths; use these for general, composable logic.
 */
#ifndef FP_FUNCTIONAL_H
#define FP_FUNCTIONAL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "fp_monads.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- folds without an explicit seed (use the first/last element) ----
 * foldl1/foldr1 are undefined on an empty list; here they return 0. */
int64_t fp_foldl1_i64(const int64_t* in, size_t n, int64_t (*fn)(int64_t acc, int64_t x, void*), void* ctx);
double  fp_foldl1_f64(const double* in, size_t n, double (*fn)(double acc, double x, void*), void* ctx);
int64_t fp_foldr1_i64(const int64_t* in, size_t n, int64_t (*fn)(int64_t x, int64_t acc, void*), void* ctx);
double  fp_foldr1_f64(const double* in, size_t n, double (*fn)(double x, double acc, void*), void* ctx);

/* ---- scanl1: scan seeded by the first element (out[0] = in[0]) ---- */
size_t fp_scanl1_i64(const int64_t* in, int64_t* out, size_t n, int64_t (*fn)(int64_t acc, int64_t x, void*), void* ctx);
size_t fp_scanl1_f64(const double* in, double* out, size_t n, double (*fn)(double acc, double x, void*), void* ctx);

/* ---- mapAccumL: map carrying a left-to-right accumulator; returns final acc.
 * fn(acc, x, &out_elem) computes the emitted element and the next acc. ---- */
int64_t fp_mapAccumL_i64(const int64_t* in, int64_t* out, size_t n, int64_t acc0,
                         int64_t (*fn)(int64_t acc, int64_t x, int64_t* out_elem, void*), void* ctx);
double  fp_mapAccumL_f64(const double* in, double* out, size_t n, double acc0,
                         double (*fn)(double acc, double x, double* out_elem, void*), void* ctx);

/* ---- zipWith3: combine three inputs elementwise ---- */
size_t fp_zipWith3_i64(const int64_t* a, const int64_t* b, const int64_t* c, int64_t* out, size_t n,
                       int64_t (*fn)(int64_t, int64_t, int64_t, void*), void* ctx);
size_t fp_zipWith3_f64(const double* a, const double* b, const double* c, double* out, size_t n,
                       double (*fn)(double, double, double, void*), void* ctx);

/* ---- right fold: foldr f z [x0..xn] = f x0 (f x1 (... (f xn z))) ---- */
int64_t fp_foldr_i64(const int64_t* in, size_t n, int64_t init,
                     int64_t (*fn)(int64_t x, int64_t acc, void* ctx), void* ctx);
double  fp_foldr_f64(const double* in, size_t n, double init,
                     double (*fn)(double x, double acc, void* ctx), void* ctx);

/* ---- scans (inclusive): out[i] = fold of in[0..i] ---- */
size_t fp_scanl_i64(const int64_t* in, int64_t* out, size_t n, int64_t init,
                    int64_t (*fn)(int64_t acc, int64_t x, void* ctx), void* ctx);
size_t fp_scanl_f64(const double* in, double* out, size_t n, double init,
                    double (*fn)(double acc, double x, void* ctx), void* ctx);
size_t fp_scanr_i64(const int64_t* in, int64_t* out, size_t n, int64_t init,
                    int64_t (*fn)(int64_t x, int64_t acc, void* ctx), void* ctx);
size_t fp_scanr_f64(const double* in, double* out, size_t n, double init,
                    double (*fn)(double x, double acc, void* ctx), void* ctx);

/* ---- predicate-based slicing (general predicate) ---- */
size_t fp_take_while_i64(const int64_t* in, int64_t* out, size_t n, bool (*pred)(int64_t, void*), void* ctx);
size_t fp_take_while_f64(const double* in, double* out, size_t n, bool (*pred)(double, void*), void* ctx);
size_t fp_drop_while_i64(const int64_t* in, int64_t* out, size_t n, bool (*pred)(int64_t, void*), void* ctx);
size_t fp_drop_while_f64(const double* in, double* out, size_t n, bool (*pred)(double, void*), void* ctx);
/* span: prefix satisfying pred -> `take`; rest -> `drop`. Returns length of the prefix. */
size_t fp_span_i64(const int64_t* in, int64_t* take, int64_t* drop, size_t n, bool (*pred)(int64_t, void*), void* ctx);
size_t fp_span_f64(const double* in, double* take, double* drop, size_t n, bool (*pred)(double, void*), void* ctx);

/* ---- partition by predicate: matches -> yes, rest -> no. Returns count in yes. ---- */
size_t fp_partition_i64(const int64_t* in, int64_t* yes, int64_t* no, size_t n, bool (*pred)(int64_t, void*), void* ctx);
size_t fp_partition_f64(const double* in, double* yes, double* no, size_t n, bool (*pred)(double, void*), void* ctx);

/* ---- Foldable queries ---- */
bool   fp_all_i64(const int64_t* in, size_t n, bool (*pred)(int64_t, void*), void* ctx);
bool   fp_all_f64(const double* in, size_t n, bool (*pred)(double, void*), void* ctx);
bool   fp_any_i64(const int64_t* in, size_t n, bool (*pred)(int64_t, void*), void* ctx);
bool   fp_any_f64(const double* in, size_t n, bool (*pred)(double, void*), void* ctx);
size_t fp_count_if_i64(const int64_t* in, size_t n, bool (*pred)(int64_t, void*), void* ctx);
size_t fp_count_if_f64(const double* in, size_t n, bool (*pred)(double, void*), void* ctx);
/* find: first element satisfying pred, as Maybe (Nothing if none) */
Maybe  fp_find_i64(const int64_t* in, size_t n, bool (*pred)(int64_t, void*), void* ctx);
Maybe  fp_find_f64(const double* in, size_t n, bool (*pred)(double, void*), void* ctx);

/* ---- list monad bind: concatMap / flatMap ----
 * For each input, `fn` writes 0+ results into `scratch` (capacity `scratch_cap`)
 * and returns how many it wrote; they are appended to `out` (capacity out_cap).
 * Returns total written, or (size_t)-1 if `out_cap` is exceeded. */
size_t fp_concat_map_i64(const int64_t* in, size_t n, int64_t* out, size_t out_cap,
                         int64_t* scratch, size_t scratch_cap,
                         size_t (*fn)(int64_t x, int64_t* out, size_t cap, void* ctx), void* ctx);

/* ---- generators (anamorphisms) ---- */
/* iterate: out = [seed, f seed, f (f seed), ...] of length n */
size_t fp_iterate_i64(int64_t* out, size_t n, int64_t seed, int64_t (*fn)(int64_t, void*), void* ctx);
size_t fp_iterate_f64(double*  out, size_t n, double  seed, double  (*fn)(double,  void*), void* ctx);
/* unfoldr: from a seed, `fn` yields the next value + updated state and returns
 * true to continue / false to stop. Stops at `max` elements. Returns count. */
size_t fp_unfoldr_i64(int64_t* out, size_t max, int64_t seed,
                      bool (*fn)(int64_t* state, int64_t* out_val, void* ctx), void* ctx);
size_t fp_unfoldr_f64(double* out, size_t max, double seed,
                      bool (*fn)(double* state, double* out_val, void* ctx), void* ctx);

/* ================= Milestone B: ordering & grouping ================
 * sortBy is a STABLE sort (like Haskell's) with a context-aware comparator
 * returning <0 / 0 / >0. It sorts in place and allocates an O(n) temp buffer. */
void fp_sort_by_i64(int64_t* arr, size_t n, int (*cmp)(int64_t a, int64_t b, void* ctx), void* ctx);
void fp_sort_by_f64(double*  arr, size_t n, int (*cmp)(double  a, double  b, void* ctx), void* ctx);

/* groupBy: split into maximal runs of CONSECUTIVE elements for which eq holds.
 * Copies `in` to `out_flat` (same order) and writes each run's length to
 * `out_lens` (capacity >= n). Returns the number of groups. */
size_t fp_group_by_i64(const int64_t* in, size_t n, int64_t* out_flat, size_t* out_lens,
                       bool (*eq)(int64_t, int64_t, void*), void* ctx);
size_t fp_group_by_f64(const double* in, size_t n, double* out_flat, size_t* out_lens,
                       bool (*eq)(double, double, void*), void* ctx);

/* nubBy: remove duplicates by an equality predicate, keeping first occurrences
 * and input order. O(n^2), like Haskell's nub. Returns the deduplicated count. */
size_t fp_nub_by_i64(const int64_t* in, size_t n, int64_t* out, bool (*eq)(int64_t, int64_t, void*), void* ctx);
size_t fp_nub_by_f64(const double* in, size_t n, double* out, bool (*eq)(double, double, void*), void* ctx);

/* intersperse: place `sep` between adjacent elements. Writes 2n-1 elements. */
size_t fp_intersperse_i64(const int64_t* in, size_t n, int64_t sep, int64_t* out);
size_t fp_intersperse_f64(const double* in, size_t n, double sep, double* out);

/* transpose a row-major rows x cols matrix into a cols x rows matrix. */
size_t fp_transpose_i64(const int64_t* in, size_t rows, size_t cols, int64_t* out);
size_t fp_transpose_f64(const double* in, size_t rows, size_t cols, double* out);

/* mapMaybe (map then keep only the Justs) already lives in fp_monads.h:
 *   size_t fp_map_maybe_i64(const int64_t* in, size_t n, Maybe (*fn)(int64_t), int64_t* out);
 * Included here via fp_monads.h so callers get the whole toolkit from one header. */

/* ============ Milestone C: traversal & monoidal folds ============
 * traverse = a fused map + sequence that short-circuits on the first failure
 * (the all-or-nothing validation pattern). On success `out` holds the results
 * and the return is Just/Right carrying the count; on failure it is Nothing /
 * the first Left, and `out` is left partially written.
 * (fp_traverse_maybe_{i64,f64} already live in fp_monads.h, included above.) */
Either fp_traverse_either_i64(const int64_t* in, size_t n, Either (*fn)(int64_t), int64_t* out);
Either fp_traverse_either_f64(const double*  in, size_t n, Either (*fn)(double),  double*  out);

/* foldMap = mconcat . map f — fold with an explicit monoid (empty + combine).
 * e.g. sum = foldMap id (+) 0; maximum = foldMap id max MIN; any p = foldMap p (||) false. */
int64_t fp_fold_map_i64(const int64_t* in, size_t n, int64_t empty,
                        int64_t (*map)(int64_t), int64_t (*combine)(int64_t, int64_t));
double  fp_fold_map_f64(const double* in, size_t n, double empty,
                        double (*map)(double), double (*combine)(double, double));

#ifdef __cplusplus
}
#endif
#endif /* FP_FUNCTIONAL_H */
