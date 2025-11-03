#pragma once

#include <stdint.h>
#include <stddef.h> // for size_t
#include <stdbool.h>

/* ============================================================================
 *
 * FP-ASM Core Library (fp_core.h)
 *
 * High-performance functional programming library for C using hand-optimized
 * x64 assembly with AVX2 SIMD instructions.
 *
 * ============================================================================
 *
 * FUNCTIONAL PURITY GUARANTEE
 *
 * This library maintains strict functional programming principles. ALL functions
 * in this library guarantee the following purity contract:
 *
 * 1. INPUT IMMUTABILITY
 *    - All input parameters marked `const type*` are NEVER modified
 *    - Input data can be safely shared, reused, or passed as literals
 *    - No hidden side effects or mutations
 *
 * 2. OUTPUT CLARITY
 *    - Functions that produce array output use explicit `type* output` parameters
 *    - Return values are used for scalars, counts, or status codes
 *    - No ambiguous parameters that serve dual input/output roles
 *
 * 3. NO HIDDEN STATE
 *    - No global variables modified
 *    - No static local state
 *    - Each call is independent and deterministic
 *
 * 4. CONST-CORRECTNESS
 *    - Input arrays: Always `const type*`
 *    - Output arrays: Always `type*` (never const)
 *    - Compiler-enforced immutability guarantees
 *
 * VERIFICATION:
 *    - See CONST_CORRECTNESS_AUDIT.md for comprehensive audit results
 *    - See COMPLETE_PURITY_AUDIT.md for detailed purity analysis
 *    - See test_purity.c for runtime verification tests
 *
 * INTERNAL IMPLEMENTATION NOTES:
 *    - Some functions (percentiles, quartiles, outlier detection) perform
 *      internal sorting on a COPY of the input data, preserving the original
 *    - See fp_percentile_wrappers.c for the copy-and-sort pattern
 *    - No function ever mutates user-provided input data
 *
 * ============================================================================ */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Category 1: Fused Folds (Module 2)
 * ============================================================================ */

/* i64 fused folds (4 elements per YMM register) */
int64_t fp_fold_sumsq_i64(const int64_t* in, size_t n);
int64_t fp_fold_dotp_i64(const int64_t* a, const int64_t* b, size_t n);
int64_t fp_fold_sad_i64(const int64_t* a, const int64_t* b, size_t n);

/* f64 fused folds (4 elements per YMM register) */
double  fp_fold_dotp_f64(const double* a, const double* b, size_t n);

/* i32 fused folds (8 elements per YMM register - 2X throughput!) */
int32_t fp_fold_sumsq_i32(const int32_t* in, size_t n);
int32_t fp_fold_dotp_i32(const int32_t* a, const int32_t* b, size_t n);
int32_t fp_fold_sad_i32(const int32_t* a, const int32_t* b, size_t n);

/* f32 fused folds (8 elements per YMM register - 2X throughput!) */
float   fp_fold_sumsq_f32(const float* in, size_t n);
float   fp_fold_dotp_f32(const float* a, const float* b, size_t n);
float   fp_fold_sad_f32(const float* a, const float* b, size_t n);

/* u32 fused folds (8 elements per YMM register - 2X throughput!) */
uint32_t fp_fold_sumsq_u32(const uint32_t* in, size_t n);
uint32_t fp_fold_dotp_u32(const uint32_t* a, const uint32_t* b, size_t n);
uint32_t fp_fold_sad_u32(const uint32_t* a, const uint32_t* b, size_t n);

/* u64 fused folds (4 elements per YMM register) */
uint64_t fp_fold_sumsq_u64(const uint64_t* in, size_t n);
uint64_t fp_fold_dotp_u64(const uint64_t* a, const uint64_t* b, size_t n);
uint64_t fp_fold_sad_u64(const uint64_t* a, const uint64_t* b, size_t n);

/* i16 fused folds (16 elements per YMM register - 4X throughput!) */
int16_t fp_fold_sumsq_i16(const int16_t* in, size_t n);
int16_t fp_fold_dotp_i16(const int16_t* a, const int16_t* b, size_t n);
int16_t fp_fold_sad_i16(const int16_t* a, const int16_t* b, size_t n);

/* u16 fused folds (16 elements per YMM register - 4X throughput!) */
uint16_t fp_fold_sumsq_u16(const uint16_t* in, size_t n);
uint16_t fp_fold_dotp_u16(const uint16_t* a, const uint16_t* b, size_t n);
uint16_t fp_fold_sad_u16(const uint16_t* a, const uint16_t* b, size_t n);

/* i8 fused folds (32 elements per YMM register - 8X throughput! BUT no vpmullb) */
int8_t fp_fold_sumsq_i8(const int8_t* in, size_t n);
int8_t fp_fold_dotp_i8(const int8_t* a, const int8_t* b, size_t n);
int8_t fp_fold_sad_i8(const int8_t* a, const int8_t* b, size_t n);

/* ============================================================================
 * Category 2: Simple Folds (Module 1)
 * ============================================================================ */

/* i64 reductions (4 elements per YMM register) */
int64_t fp_reduce_add_i64(const int64_t* in, size_t n);
int64_t fp_reduce_max_i64(const int64_t* in, size_t n);
int64_t fp_reduce_min_i64(const int64_t* in, size_t n);

/* f64 reductions (4 elements per YMM register) */
double  fp_reduce_add_f64(const double* in, size_t n);
double  fp_reduce_max_f64(const double* in, size_t n);
double  fp_reduce_min_f64(const double* in, size_t n);

/* i32 reductions (8 elements per YMM register - 2X throughput!) */
int32_t fp_reduce_add_i32(const int32_t* in, size_t n);
int32_t fp_reduce_mul_i32(const int32_t* in, size_t n);
int32_t fp_reduce_min_i32(const int32_t* in, size_t n);
int32_t fp_reduce_max_i32(const int32_t* in, size_t n);

/* f32 reductions (8 elements per YMM register - 2X throughput!) */
float   fp_reduce_add_f32(const float* in, size_t n);
float   fp_reduce_mul_f32(const float* in, size_t n);
float   fp_reduce_min_f32(const float* in, size_t n);
float   fp_reduce_max_f32(const float* in, size_t n);

/* u32 reductions (8 elements per YMM register - 2X throughput!) */
uint32_t fp_reduce_add_u32(const uint32_t* in, size_t n);
uint32_t fp_reduce_mul_u32(const uint32_t* in, size_t n);
uint32_t fp_reduce_min_u32(const uint32_t* in, size_t n);
uint32_t fp_reduce_max_u32(const uint32_t* in, size_t n);

/* u64 reductions (4 elements per YMM register) */
uint64_t fp_reduce_add_u64(const uint64_t* in, size_t n);
uint64_t fp_reduce_mul_u64(const uint64_t* in, size_t n);
uint64_t fp_reduce_min_u64(const uint64_t* in, size_t n);
uint64_t fp_reduce_max_u64(const uint64_t* in, size_t n);

/* i16 reductions (16 elements per YMM register - 4X throughput!) */
int16_t fp_reduce_add_i16(const int16_t* in, size_t n);
int16_t fp_reduce_mul_i16(const int16_t* in, size_t n);
int16_t fp_reduce_min_i16(const int16_t* in, size_t n);
int16_t fp_reduce_max_i16(const int16_t* in, size_t n);

/* u16 reductions (16 elements per YMM register - 4X throughput!) */
uint16_t fp_reduce_add_u16(const uint16_t* in, size_t n);
uint16_t fp_reduce_mul_u16(const uint16_t* in, size_t n);
uint16_t fp_reduce_min_u16(const uint16_t* in, size_t n);
uint16_t fp_reduce_max_u16(const uint16_t* in, size_t n);

/* i8 reductions (32 elements per YMM register - 8X throughput! BUT no vpmullb) */
int8_t fp_reduce_add_i8(const int8_t* in, size_t n);
int8_t fp_reduce_mul_i8(const int8_t* in, size_t n);
int8_t fp_reduce_min_i8(const int8_t* in, size_t n);
int8_t fp_reduce_max_i8(const int8_t* in, size_t n);

/* ============================================================================
 * Category 3: Fused Maps (Module 3)
 *
 * BLAS Level 1 style operations. Often memory-bound, but optimal SIMD
 * implementation is required to saturate bandwidth.
 * ============================================================================ */

/**
 * FP: out = zipWith (+) (map (*c) x) y  (Classic AXPY)
 * C:  for(i) { out[i] = c * x[i] + y[i]; }
 * Win: SIMD (FMA) for f64, Scalar unroll for i64, vpmulld+vpaddd for i32
 */
void fp_map_axpy_f64(const double* x, const double* y, double* out, size_t n, double c);
void fp_map_axpy_i64(const int64_t* x, const int64_t* y, int64_t* out, size_t n, int64_t c);
void fp_map_axpy_i32(const int32_t* x, const int32_t* y, int32_t* out, size_t n, int32_t c);
void fp_map_axpy_f32(const float* x, const float* y, float* out, size_t n, float c);
void fp_map_axpy_u32(const uint32_t* x, const uint32_t* y, uint32_t* out, size_t n, uint32_t c);
void fp_map_axpy_u64(const uint64_t* x, const uint64_t* y, uint64_t* out, size_t n, uint64_t c);
void fp_map_axpy_i16(const int16_t* x, const int16_t* y, int16_t* out, size_t n, int16_t c);
void fp_map_axpy_u16(const uint16_t* x, const uint16_t* y, uint16_t* out, size_t n, uint16_t c);
void fp_map_axpy_i8(const int8_t* x, const int8_t* y, int8_t* out, size_t n, int8_t c);

/**
 * FP: out = map (*c) in
 * C:  for(i) { out[i] = c * in[i]; }
 * Win: Scalar unroll (i64), Guaranteed SIMD (f64), 8-wide SIMD (i32/f32)
 */
void fp_map_scale_i64(const int64_t* in, int64_t* out, size_t n, int64_t c);
void fp_map_scale_f64(const double* in, double* out, size_t n, double c);
void fp_map_scale_i32(const int32_t* in, int32_t* out, size_t n, int32_t c);
void fp_map_scale_f32(const float* in, float* out, size_t n, float c);
void fp_map_scale_u32(const uint32_t* in, uint32_t* out, size_t n, uint32_t c);
void fp_map_scale_u64(const uint64_t* in, uint64_t* out, size_t n, uint64_t c);
void fp_map_scale_i16(const int16_t* in, int16_t* out, size_t n, int16_t c);
void fp_map_scale_u16(const uint16_t* in, uint16_t* out, size_t n, uint16_t c);
void fp_map_scale_i8(const int8_t* in, int8_t* out, size_t n, int8_t c);

/**
 * FP: out = map (+c) in
 * C:  for(i) { out[i] = in[i] + c; }
 * Win: Guaranteed SIMD (all types), 8-wide for i32/f32
 */
void fp_map_offset_i64(const int64_t* in, int64_t* out, size_t n, int64_t c);
void fp_map_offset_f64(const double* in, double* out, size_t n, double c);
void fp_map_offset_i32(const int32_t* in, int32_t* out, size_t n, int32_t c);
void fp_map_offset_f32(const float* in, float* out, size_t n, float c);
void fp_map_offset_u32(const uint32_t* in, uint32_t* out, size_t n, uint32_t c);
void fp_map_offset_u64(const uint64_t* in, uint64_t* out, size_t n, uint64_t c);
void fp_map_offset_i16(const int16_t* in, int16_t* out, size_t n, int16_t c);
void fp_map_offset_u16(const uint16_t* in, uint16_t* out, size_t n, uint16_t c);
void fp_map_offset_i8(const int8_t* in, int8_t* out, size_t n, int8_t c);

/**
 * FP: out = zipWith (+) a b
 * C:  for(i) { out[i] = a[i] + b[i]; }
 * Win: Guaranteed SIMD (all types), 8-wide for i32/f32
 */
void fp_zip_add_i64(const int64_t* a, const int64_t* b, int64_t* out, size_t n);
void fp_zip_add_f64(const double* a, const double* b, double* out, size_t n);
void fp_zip_add_i32(const int32_t* a, const int32_t* b, int32_t* out, size_t n);
void fp_zip_add_f32(const float* a, const float* b, float* out, size_t n);
void fp_zip_add_u32(const uint32_t* a, const uint32_t* b, uint32_t* out, size_t n);
void fp_zip_add_u64(const uint64_t* a, const uint64_t* b, uint64_t* out, size_t n);
void fp_zip_add_i16(const int16_t* a, const int16_t* b, int16_t* out, size_t n);
void fp_zip_add_u16(const uint16_t* a, const uint16_t* b, uint16_t* out, size_t n);
void fp_zip_add_i8(const int8_t* a, const int8_t* b, int8_t* out, size_t n);
void fp_zip_add_u8(const uint8_t* a, const uint8_t* b, uint8_t* out, size_t n);
void fp_map_offset_u8(const uint8_t* in, uint8_t* out, size_t n, uint8_t c);
void fp_map_scale_u8(const uint8_t* in, uint8_t* out, size_t n, uint8_t c);
void fp_map_axpy_u8(const uint8_t* x, const uint8_t* y, uint8_t* out, size_t n, uint8_t c);
uint8_t fp_fold_sad_u8(const uint8_t* a, const uint8_t* b, size_t n);
uint8_t fp_fold_dotp_u8(const uint8_t* a, const uint8_t* b, size_t n);
uint8_t fp_fold_sumsq_u8(const uint8_t* in, size_t n);
uint8_t fp_reduce_max_u8(const uint8_t* in, size_t n);
uint8_t fp_reduce_min_u8(const uint8_t* in, size_t n);
uint8_t fp_reduce_mul_u8(const uint8_t* in, size_t n);
uint8_t fp_reduce_add_u8(const uint8_t* in, size_t n);

/* u8 fused maps (32 elements per YMM register - 8X throughput! BUT no vpmullb) */
/* u8 reductions (32 elements per YMM register - 8X throughput! BUT no vpmullb) */
/* u8 fused folds (32 elements per YMM register - 8X throughput! BUT no vpmullb) */


/* ============================================================================
 * Category 4: Simple Maps (Module 4) - Transformers
 *
 * Element-wise transformations. Guaranteed optimal SIMD performance.
 * ============================================================================ */

/**
 * FP: out = map abs in
 * C:  for(i) { out[i] = abs(in[i]); }
 * Win: Guaranteed SIMD using bitwise trick (i64) or mask (f64)
 */
void fp_map_abs_i64(const int64_t* in, int64_t* out, size_t n);
void fp_map_abs_f64(const double* in, double* out, size_t n);

/**
 * FP: out = map sqrt in
 * C:  for(i) { out[i] = sqrt(in[i]); }
 * Win: Guaranteed SIMD using vsqrtpd/vsqrtps
 */
void fp_map_sqrt_f64(const double* in, double* out, size_t n);

/**
 * FP: out = map (clamp min_val max_val) in
 * C:  for(i) { out[i] = max(min_val, min(max_val, in[i])); }
 * Win: Guaranteed SIMD (f64), Scalar for i64 (no vpmaxsq in AVX2)
 */
void fp_map_clamp_i64(const int64_t* in, int64_t* out, size_t n, int64_t min_val, int64_t max_val);
void fp_map_clamp_f64(const double* in, double* out, size_t n, double min_val, double max_val);


/* ============================================================================
 * Category 5: Scans (Module 5) - Prefix Sums
 *
 * Inclusive prefix sum (scan) operations. Inherently sequential, optimized
 * with scalar loop unrolling. Parallel SIMD algorithms are future work.
 * ============================================================================ */

/**
 * FP: out = scanl1 (+) in  (inclusive prefix sum)
 * C:  acc = 0; for(i) { acc += in[i]; out[i] = acc; }
 * Example: [1, 2, 3, 4] -> [1, 3, 6, 10]
 * Win: Scalar with 4-way loop unrolling
 */
void fp_scan_add_i64(const int64_t* in, int64_t* out, size_t n);
void fp_scan_add_f64(const double* in, double* out, size_t n);


/* ============================================================================
 * Category 6: Predicates (Module 6) - Boolean Operations
 *
 * Return true/false based on array comparisons. Optimized with AVX2 compare
 * instructions and mask extraction for early exit.
 * ============================================================================ */

/**
 * FP: all (== value) arr
 * C:  for(i) if (arr[i] != value) return false; return true;
 * Returns true if ALL elements equal the constant value
 */
bool fp_pred_all_eq_const_i64(const int64_t* arr, size_t n, int64_t value);

/**
 * FP: any (> value) arr
 * C:  for(i) if (arr[i] > value) return true; return false;
 * Returns true if ANY element is greater than the constant value
 */
bool fp_pred_any_gt_const_i64(const int64_t* arr, size_t n, int64_t value);

/**
 * FP: all (uncurry (>)) (zip a b)
 * C:  for(i) if (a[i] <= b[i]) return false; return true;
 * Returns true if ALL corresponding elements satisfy a[i] > b[i]
 */
bool fp_pred_all_gt_zip_i64(const int64_t* a, const int64_t* b, size_t n);


/* ============================================================================
 * Category 7: Stream Compaction (Module 7) - List FP Operations
 *
 * Variable-size output operations (filter, partition) using SIMD compaction.
 * These are THE operations that distinguish "List FP" from "Array FP".
 * ============================================================================ */

/**
 * FP: filter (> threshold) list
 * C:  result = []; for(i) if (arr[i] > threshold) result.append(arr[i]);
 * Returns: number of elements written to output
 *
 * This is the CRITICAL test for "List FP" fitness!
 */
size_t fp_filter_gt_i64_simple(const int64_t* input, int64_t* output, size_t n, int64_t threshold);

/**
 * FP: partition (> threshold) list → (pass, fail)
 * C:  pass = []; fail = []; for(i) { if (arr[i] > thresh) pass.append(arr[i]); else fail.append(arr[i]); }
 *
 * Splits input into two outputs based on predicate.
 */
void fp_partition_gt_i64(const int64_t* input, int64_t* output_pass, int64_t* output_fail,
                         size_t n, int64_t threshold,
                         size_t* out_pass_count, size_t* out_fail_count);

/**
 * FP: takeWhile (> threshold) list
 * C:  result = []; for(i) { if (arr[i] > threshold) result.append(arr[i]); else break; }
 *
 * Takes elements while predicate is true, stops at first failure (early exit).
 */
size_t fp_take_while_gt_i64(const int64_t* input, int64_t* output, size_t n, int64_t threshold);

/**
 * FP: dropWhile (> threshold) list
 * C:  skip = true; for(i) { if (skip && arr[i] > threshold) continue; skip = false; result.append(arr[i]); }
 *
 * Drops elements while predicate is true, returns rest (early exit).
 */
size_t fp_drop_while_gt_i64(const int64_t* input, int64_t* output, size_t n, int64_t threshold);


/* ============================================================================
 * Category 8: Essential Operations (Module 8) - TIER 1 Completeness
 *
 * Core operations present in every FP standard library (Haskell Prelude,
 * Common Lisp, Scheme). These complete the library to ~70% coverage.
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * Index-Based Operations
 * ---------------------------------------------------------------------------- */

/**
 * FP: take n list
 * C:  for(i = 0; i < min(n, len); i++) output[i] = input[i];
 * Haskell: take 3 [1,2,3,4,5] → [1,2,3]
 * Returns: number of elements taken
 */
size_t fp_take_n_i64(const int64_t* input, int64_t* output, size_t array_len, size_t take_count);

/**
 * FP: drop n list
 * C:  for(i = n; i < len; i++) output[i-n] = input[i];
 * Haskell: drop 2 [1,2,3,4,5] → [3,4,5]
 * Returns: number of elements in output
 */
size_t fp_drop_n_i64(const int64_t* input, int64_t* output, size_t array_len, size_t drop_count);

/**
 * FP: take (end - start) . drop start
 * C:  for(i = start; i < end; i++) output[i-start] = input[i];
 * Haskell: slice 2 5 [0,1,2,3,4,5,6] → [2,3,4]
 * Returns: number of elements in output
 */
size_t fp_slice_i64(const int64_t* input, int64_t* output, size_t array_len,
                     size_t start, size_t end);

/* ----------------------------------------------------------------------------
 * Additional Reductions
 * ---------------------------------------------------------------------------- */

/**
 * FP: product list
 * C:  result = 1; for(i) result *= arr[i];
 * Haskell: product [1,2,3,4] → 24
 * Returns: product of all elements (1 for empty array)
 */
int64_t fp_reduce_product_i64(const int64_t* input, size_t n);
double fp_reduce_product_f64(const double* input, size_t n);

/* ----------------------------------------------------------------------------
 * Search Operations
 * ---------------------------------------------------------------------------- */

/**
 * FP: findIndex (== target) list
 * C:  for(i) if (arr[i] == target) return i; return -1;
 * Haskell: findIndex (== 7) [1,3,7,2] → Just 2
 * Returns: index of first match, or -1 if not found
 */
int64_t fp_find_index_i64(const int64_t* input, size_t n, int64_t target);

/**
 * FP: elem target list
 * C:  for(i) if (arr[i] == target) return true; return false;
 * Haskell: elem 7 [1,3,7,2] → True
 * Returns: 1 if found, 0 if not found
 */
bool fp_contains_i64(const int64_t* input, size_t n, int64_t target);

/* ----------------------------------------------------------------------------
 * Array Manipulation
 * ---------------------------------------------------------------------------- */

/**
 * FP: reverse list
 * C:  for(i) output[i] = input[n-1-i];
 * Haskell: reverse [1,2,3,4] → [4,3,2,1]
 */
void fp_reverse_i64(const int64_t* input, int64_t* output, size_t n);

/**
 * FP: list1 ++ list2
 * C:  memcpy(output, a, na*8); memcpy(output+na, b, nb*8);
 * Haskell: [1,2,3] ++ [4,5,6] → [1,2,3,4,5,6]
 * Returns: total length (len_a + len_b)
 */
size_t fp_concat_i64(const int64_t* input_a, const int64_t* input_b, int64_t* output,
                      size_t len_a, size_t len_b);

/**
 * FP: replicate n value
 * C:  for(i = 0; i < n; i++) output[i] = value;
 * Haskell: replicate 5 7 → [7,7,7,7,7]
 */
void fp_replicate_i64(int64_t* output, size_t n, int64_t value);


/* ============================================================================
 * Category 9: TIER 2 Operations (Module 9) - Sorting and Set Operations
 *
 * Advanced operations for complete FP standard library coverage (~85%).
 * Includes high-performance sorting and set-theoretic operations.
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * Sorting Operations
 * ---------------------------------------------------------------------------- */

/**
 * DESIGN DECISION: This library does NOT provide in-place sorting functions.
 *
 * Rationale: FP-ASM is a functional programming library that guarantees input
 * immutability. In-place sorting violates this core principle by mutating
 * user data.
 *
 * Alternative: Functions that require sorted data (percentiles, quartiles, etc.)
 * perform internal sorting on a copy of the input, leaving the original data
 * untouched.
 *
 * For explicit sorting needs, use standard C qsort() or C++ std::sort().
 * Example:
 *   double* sorted = (double*)malloc(n * sizeof(double));
 *   memcpy(sorted, data, n * sizeof(double));
 *   qsort(sorted, n, sizeof(double), compare_double);
 */

/* ----------------------------------------------------------------------------
 * Set Operations
 * ---------------------------------------------------------------------------- */

/**
 * FP: nub list  (remove consecutive duplicates)
 * C:  unique_copy(sorted_array, output);
 * Haskell: nub [1,2,2,3,3,3,4] → [1,2,3,4]
 *
 * Note: Input should be sorted for correctness
 * Returns: number of unique elements
 */
size_t fp_unique_i64(const int64_t* input, int64_t* output, size_t n);

/**
 * FP: union list1 list2
 * C:  set_union(sorted_a, sorted_b, output);
 * Haskell: union [1,2,3] [2,3,4] → [1,2,3,4]
 *
 * Note: Both inputs should be sorted
 * Returns: number of elements in union
 */
size_t fp_union_i64(const int64_t* array_a, const int64_t* array_b, int64_t* output,
                     size_t len_a, size_t len_b);

/**
 * FP: intersect list1 list2
 * C:  set_intersection(sorted_a, sorted_b, output);
 * Haskell: intersect [1,2,3] [2,3,4] → [2,3]
 *
 * Note: Both inputs should be sorted
 * Returns: number of elements in intersection
 */
size_t fp_intersect_i64(const int64_t* array_a, const int64_t* array_b, int64_t* output,
                         size_t len_a, size_t len_b);


/* ============================================================================
 * Category 10: TIER 3 Operations (Module 10) - Advanced FP (100% Complete!)
 *
 * Final operations for COMPLETE FP standard library coverage.
 * Includes grouping, sequence generation, and boolean reductions.
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * Grouping Operations
 * ---------------------------------------------------------------------------- */

/**
 * FP: group list  (group consecutive equal elements)
 * Haskell: group [1,1,2,2,2,3] → [[1,1],[2,2,2],[3]]
 *
 * Due to C constraints, returns parallel arrays:
 * - groups_out: representative value for each group
 * - counts_out: size of each group
 *
 * Returns: number of groups
 */
size_t fp_group_i64(const int64_t* input, int64_t* groups_out, int64_t* counts_out, size_t n);

/**
 * FP: Run-length encoding
 * Alternative: returns interleaved [val1, count1, val2, count2, ...]
 *
 * More memory-efficient than separate arrays
 * Returns: number of output elements (groups * 2)
 */
size_t fp_run_length_encode_i64(const int64_t* input, int64_t* output, size_t n);

/* ----------------------------------------------------------------------------
 * Sequence Generation (Unfold)
 * ---------------------------------------------------------------------------- */

/**
 * FP: iterate (+step) start
 * Haskell: take n $ iterate (+step) start → [start, start+step, start+2*step, ...]
 *
 * Generates arithmetic sequence
 */
void fp_iterate_add_i64(int64_t* output, size_t n, int64_t start, int64_t step);

/**
 * FP: iterate (*factor) start
 * Haskell: take n $ iterate (*factor) start → [start, start*factor, start*factor^2, ...]
 *
 * Generates geometric sequence
 */
void fp_iterate_mul_i64(int64_t* output, size_t n, int64_t start, int64_t factor);

/**
 * FP: [start..end-1]
 * Haskell: [1..10] → [1,2,3,4,5,6,7,8,9,10]
 *
 * Generates range of integers
 * Returns: number of elements (max(0, end - start))
 */
size_t fp_range_i64(int64_t* output, int64_t start, int64_t end);

/* ----------------------------------------------------------------------------
 * Boolean Reductions
 * ---------------------------------------------------------------------------- */

/**
 * FP: and list
 * Haskell: and [True, True, False] → False
 *
 * Returns: 1 if all elements are non-zero, 0 if any zero
 * Note: Treats int64_t as bool (0=false, non-zero=true)
 */
bool fp_reduce_and_bool(const int64_t* input, size_t n);

/**
 * FP: or list
 * Haskell: or [False, False, True] → True
 *
 * Returns: 1 if any element is non-zero, 0 if all zero
 */
bool fp_reduce_or_bool(const int64_t* input, size_t n);

/* ----------------------------------------------------------------------------
 * Additional Utilities
 * ---------------------------------------------------------------------------- */

/**
 * FP: zip [0..] list
 * Haskell: zip [0..] list → [(0,x), (1,y), (2,z), ...]
 *
 * Returns interleaved: [0, x, 1, y, 2, z, ...]
 * Returns: output length (n * 2)
 */
size_t fp_zip_with_index_i64(const int64_t* input, int64_t* output, size_t n);

/**
 * FP: replicate n value  (floating-point version)
 * Haskell: replicate 5 3.14 → [3.14, 3.14, 3.14, 3.14, 3.14]
 */
void fp_replicate_f64(double* output, size_t n, double value);

/**
 * FP: length . filter (== x)
 * Haskell: count (== 5) [1,5,2,5,3] → 2
 *
 * Count occurrences of specific value
 */
size_t fp_count_i64(const int64_t* input, size_t n, int64_t target);

/* ============================================================================
 * Module 11: High-Impact Algorithms - Statistical Computing
 * ============================================================================ */

/**
 * Descriptive Statistics Structure
 * Contains: mean, variance, std_dev, skewness, kurtosis
 */
typedef struct {
    double mean;       // First moment (average)
    double variance;   // Second central moment
    double std_dev;    // Square root of variance
    double skewness;   // Third standardized moment (asymmetry)
    double kurtosis;   // Fourth standardized moment (tail heaviness)
} DescriptiveStats;

/**
 * Calculate descriptive statistics in single pass
 *
 * FP: Uses fused fold to compute all moments simultaneously
 * Haskell-style: foldl (\acc x -> updateMoments acc x) initialAcc data
 *
 * @param data Input array of doubles
 * @param n Number of elements
 * @param stats Output structure with all statistics
 *
 * Complexity: O(n) time, O(1) space
 * Performance: 1.5-2.0x vs naive C (single pass, fused operations)
 */
void fp_descriptive_stats_f64(const double* data, size_t n, DescriptiveStats* stats);

/**
 * Calculate raw statistical moments (internal function)
 *
 * @param data Input array
 * @param n Number of elements
 * @param moments Output: [sum, sum_sq, sum_cube, sum_quad]
 */
void fp_moments_f64(const double* data, size_t n, double* moments);

/* ============================================================================
 * Algorithm #2: Percentile Calculations
 * ============================================================================ */

/**
 * Quartiles structure
 * Contains Q1, median (Q2), Q3, and IQR
 */
typedef struct {
    double q1;       // 25th percentile
    double median;   // 50th percentile (Q2)
    double q3;       // 75th percentile
    double iqr;      // Interquartile range (Q3 - Q1)
} Quartiles;

/**
 * Calculate a single percentile from data
 *
 * PURITY GUARANTEE: Input data is NEVER modified. Function performs internal
 * sorting on a copy of the data, leaving the original unchanged.
 *
 * @param data Input array (can be unsorted - will be sorted internally)
 * @param n Number of elements
 * @param p Percentile (0.0 to 1.0, e.g., 0.5 for median, 0.95 for 95th percentile)
 * @return Percentile value using linear interpolation
 *
 * Complexity: O(n log n) time (internal sorting + O(1) interpolation)
 * Performance: 2.3-3.6x vs naive C (optimized quicksort + linear interpolation)
 *
 * NOTE: If you need multiple percentiles from the same data, use fp_percentiles_f64
 * which sorts once and computes all percentiles in a single pass.
 */
double fp_percentile_f64(const double* data, size_t n, double p);

/**
 * Calculate multiple percentiles at once (batch operation)
 *
 * More efficient than calling fp_percentile_f64 repeatedly.
 *
 * PURITY GUARANTEE: Input data is NEVER modified. Function performs internal
 * sorting on a copy of the data once, then computes all percentiles.
 *
 * @param data Input array (can be unsorted - will be sorted internally)
 * @param n Number of elements
 * @param p_values Array of percentiles to calculate (each 0.0 to 1.0)
 * @param n_percentiles Number of percentiles to calculate
 * @param results Output array (must have space for n_percentiles values)
 *
 * Complexity: O(n log n + k) where k = n_percentiles
 * Performance: 2.3-3.6x vs naive C (sorts once, computes all percentiles)
 *
 * RECOMMENDED: Use this instead of multiple fp_percentile_f64 calls to avoid
 * redundant sorting.
 */
void fp_percentiles_f64(const double* data, size_t n,
                        const double* p_values, size_t n_percentiles,
                        double* results);

/**
 * Calculate quartiles (Q1, median, Q3) and IQR in one call
 *
 * Commonly used for box plots and outlier detection.
 *
 * PURITY GUARANTEE: Input data is NEVER modified. Function performs internal
 * sorting on a copy of the data, leaving the original unchanged.
 *
 * @param data Input array (can be unsorted - will be sorted internally)
 * @param n Number of elements
 * @param quartiles Output structure
 *
 * Complexity: O(n log n) time (internal sorting + O(1) computation)
 * Performance: 2.3-3.6x vs naive C (optimized quicksort + indexing)
 */
void fp_quartiles_f64(const double* data, size_t n, Quartiles* quartiles);

/* ============================================================================
 * Algorithm #3: Correlation & Covariance
 * ============================================================================ */

/**
 * Calculate covariance between two arrays
 *
 * Covariance measures how two variables change together
 * Cov(X,Y) = E[(X - E[X])(Y - E[Y])] = E[XY] - E[X]E[Y]
 *
 * Uses single-pass fused algorithm for efficiency
 *
 * @param x First array
 * @param y Second array
 * @param n Number of elements (must be same for both arrays)
 * @return Covariance value
 *
 * Complexity: O(n) time, O(1) space
 * Performance: 2.0-2.5x vs naive C (fused operations, single pass)
 */
double fp_covariance_f64(const double* x, const double* y, size_t n);

/**
 * Calculate Pearson correlation coefficient
 *
 * Correlation measures linear relationship strength between two variables
 * Returns value in range [-1.0, 1.0]:
 *   -1.0 = perfect negative correlation
 *    0.0 = no linear correlation
 *   +1.0 = perfect positive correlation
 *
 * Formula: r = Cov(X,Y) / (StdDev(X) * StdDev(Y))
 * Uses single-pass fused algorithm for maximum efficiency
 *
 * @param x First array
 * @param y Second array
 * @param n Number of elements
 * @return Correlation coefficient (-1.0 to 1.0, or NaN if undefined)
 *
 * Complexity: O(n) time, O(1) space
 * Performance: 2.0-2.5x vs naive C (single pass, fused operations)
 */
double fp_correlation_f64(const double* x, const double* y, size_t n);

/* ============================================================================
 * Algorithm #4: Linear Regression
 * ============================================================================ */

/**
 * Linear regression model: y = mx + b
 * Contains fitted coefficients and quality metrics
 */
typedef struct {
    double slope;           /**< Slope (m) of the regression line */
    double intercept;       /**< Y-intercept (b) of the regression line */
    double r_squared;       /**< Coefficient of determination (0.0 to 1.0) */
    double std_error;       /**< Standard error of estimate */
} LinearRegression;

/**
 * Compute linear regression: y = mx + b
 *
 * Fits a line to the data using least squares method
 * Computes slope, intercept, R², and standard error
 *
 * Formulas:
 *   slope = Cov(X,Y) / Var(X)
 *   intercept = mean(Y) - slope * mean(X)
 *   R² = (correlation)² (proportion of variance explained)
 *   std_error = sqrt(Σ(y_i - y_pred_i)² / (n-2))
 *
 * Uses two-pass algorithm:
 *   Pass 1: Compute statistics for slope/intercept/R² (single-pass SIMD)
 *   Pass 2: Compute residuals for standard error
 *
 * @param x Independent variable (predictor)
 * @param y Dependent variable (response)
 * @param n Number of data points (must be >= 2)
 * @param result Output struct for regression model
 *
 * Edge cases:
 *   - n < 2: All fields set to NaN
 *   - n == 2: Perfect fit (R²=1.0, std_error=0.0)
 *   - Zero variance in x: All fields set to NaN
 *
 * Complexity: O(n) time, O(1) space
 * Performance: 2.0-2.5x vs naive C (SIMD first pass, scalar second pass)
 */
void fp_linear_regression_f64(const double* x, const double* y, size_t n, LinearRegression* result);

/**
 * Make prediction using linear regression model
 *
 * Given x value, computes y = slope * x + intercept
 *
 * @param x_value Input value for prediction
 * @param model Previously fitted regression model
 * @return Predicted y value
 *
 * Complexity: O(1) time, O(1) space
 * Performance: Same as C (simple arithmetic, no optimization needed)
 */
double fp_predict_f64(double x_value, const LinearRegression* model);

/* ============================================================================
 * Algorithm #5: Outlier Detection
 * ============================================================================ */

/**
 * Detect outliers using Z-score method
 *
 * The Z-score measures how many standard deviations a point is from the mean:
 *   z = (x - mean) / stddev
 *
 * A point is marked as an outlier if |z| > threshold (typically 3.0)
 *
 * Uses two-pass algorithm:
 *   Pass 1: Compute mean and standard deviation (SIMD)
 *   Pass 2: Calculate Z-scores and mark outliers (scalar)
 *
 * @param data Input array (unsorted)
 * @param n Number of elements
 * @param threshold Z-score threshold (typically 2.0-3.0)
 * @param is_outlier Output array (1=outlier, 0=normal), must be size n
 * @return Number of outliers detected
 *
 * Edge cases:
 *   - n < 2: No outliers detected
 *   - All values identical (stddev=0): No outliers
 *
 * Complexity: O(n) time, O(1) space (excluding output)
 * Performance: 2.0-2.5x vs naive C (SIMD first pass)
 */
size_t fp_detect_outliers_zscore_f64(const double* data, size_t n,
                                      double threshold, uint8_t* is_outlier);

/**
 * Detect outliers using IQR (Interquartile Range) method
 *
 * The IQR method is robust to extreme values:
 *   IQR = Q3 - Q1
 *   lower_bound = Q1 - k * IQR
 *   upper_bound = Q3 + k * IQR
 *
 * A point is an outlier if x < lower_bound or x > upper_bound
 * where k is typically 1.5 (outlier) or 3.0 (extreme outlier)
 *
 * PURITY GUARANTEE: Input data is NEVER modified. Function performs internal
 * sorting on a copy of the data for quartile calculation, leaving original unchanged.
 *
 * @param data Input array (can be unsorted - will be sorted internally)
 * @param n Number of elements
 * @param k_factor IQR multiplier (typically 1.5)
 * @param is_outlier Output array (1=outlier, 0=normal), must be size n
 * @return Number of outliers detected
 *
 * Edge cases:
 *   - n < 4: No outliers detected (insufficient data for quartiles)
 *
 * Complexity: O(n log n) time (internal sorting + O(n) marking)
 * Performance: 1.5-2.0x vs naive C (optimized quartile + marking)
 */
size_t fp_detect_outliers_iqr_f64(const double* data, size_t n,
                                   double k_factor, uint8_t* is_outlier);

/* ============================================================================
 * Algorithm #6: Moving Averages (Financial Computing)
 * ============================================================================ */

/**
 * Simple Moving Average (SMA) - Unweighted sliding window
 *
 * Computes the arithmetic mean over a sliding window:
 *   SMA[i] = (data[i] + data[i-1] + ... + data[i-window+1]) / window
 *
 * Uses optimized sliding window algorithm:
 *   - Compute initial window sum
 *   - Slide: subtract oldest value, add newest value
 *   - O(1) per output value after initialization
 *
 * Applications: Trend detection, smoothing, support/resistance levels
 *
 * @param data Time series data (e.g., stock prices)
 * @param n Number of data points
 * @param window Window size (must be > 0 and <= n)
 * @param output Output array (size = n - window + 1)
 *
 * Edge cases:
 *   - window > n: No output produced
 *   - window = 1: Output = input (no smoothing)
 *
 * Complexity: O(n) time, O(1) space (excluding output)
 * Performance: 1.5-2.0x vs naive C (optimized sliding window)
 */
void fp_sma_f64(const double* data, size_t n, size_t window, double* output);

/**
 * Exponential Moving Average (EMA) - Exponentially weighted
 *
 * Gives more weight to recent data points:
 *   EMA[0] = data[0]
 *   EMA[i] = alpha * data[i] + (1-alpha) * EMA[i-1]
 *   where alpha = 2 / (window + 1)
 *
 * More responsive to recent changes than SMA, commonly used in trading signals.
 *
 * Applications: Trend following, crossover strategies, volatility estimation
 *
 * @param data Time series data
 * @param n Number of data points
 * @param window Period for alpha calculation
 * @param output Output array (size = n, starts from first value)
 *
 * Complexity: O(n) time, O(1) space (excluding output)
 * Performance: Similar to C (recursive formula, limited optimization)
 */
void fp_ema_f64(const double* data, size_t n, size_t window, double* output);

/**
 * Weighted Moving Average (WMA) - Linearly weighted
 *
 * Assigns linearly decreasing weights to older data:
 *   WMA[i] = (n*data[i] + (n-1)*data[i-1] + ... + 1*data[i-n+1]) / sum_of_weights
 *   where sum_of_weights = n*(n+1)/2
 *
 * More responsive than SMA but less than EMA.
 *
 * Applications: Price forecasting, technical indicators
 *
 * @param data Time series data
 * @param n Number of data points
 * @param window Window size
 * @param output Output array (size = n - window + 1)
 *
 * Complexity: O(n * window) time, O(1) space (excluding output)
 * Performance: 1.2-1.5x vs naive C (arithmetic optimization)
 */
void fp_wma_f64(const double* data, size_t n, size_t window, double* output);

/* ============================================================================
 * Algorithm #7: Rolling Window Statistics (Functional Composition)
 * ============================================================================
 *
 * Design Philosophy: COMPOSITION OVER REIMPLEMENTATION
 *
 * Instead of reimplementing min/max/sum for each rolling function, we compose
 * existing optimized functions through higher-order function abstraction.
 *
 * rolling(f) = apply f to each sliding window
 * rolling_min = rolling(reduce_min)  <-- COMPOSITION!
 * rolling_max = rolling(reduce_max)  <-- COMPOSITION!
 * rolling_sum = rolling(reduce_add)  <-- COMPOSITION!
 *
 * This is TRUE functional programming in C through function pointers.
 */

/**
 * Generic windowed reduction (higher-order function)
 *
 * Applies any reduction function over sliding windows.
 * Type signature (Haskell-style):
 *   rolling_reduce :: (Array -> Double) -> Int -> Array -> Array
 *
 * @param data Input array
 * @param n Array length
 * @param window Window size
 * @param reduce_fn Reduction function to apply (curried!)
 * @param output Output array (size = n - window + 1)
 */
void fp_rolling_reduce_f64(
    const double* data,
    size_t n,
    size_t window,
    double (*reduce_fn)(const double*, size_t),
    double* output
);

void fp_rolling_reduce_i64(
    const int64_t* data,
    size_t n,
    size_t window,
    int64_t (*reduce_fn)(const int64_t*, size_t),
    int64_t* output
);

/* Thin wrapper functions - Pure composition! Each is a one-liner:
 * rolling_min = rolling ∘ reduce_min
 * rolling_max = rolling ∘ reduce_max
 * rolling_sum = rolling ∘ reduce_add
 */

void fp_rolling_min_f64(const double* data, size_t n, size_t window, double* output);
void fp_rolling_max_f64(const double* data, size_t n, size_t window, double* output);
void fp_rolling_sum_f64(const double* data, size_t n, size_t window, double* output);
void fp_rolling_mean_f64(const double* data, size_t n, size_t window, double* output);

void fp_rolling_min_i64(const int64_t* data, size_t n, size_t window, int64_t* output);
void fp_rolling_max_i64(const int64_t* data, size_t n, size_t window, int64_t* output);
void fp_rolling_sum_i64(const int64_t* data, size_t n, size_t window, int64_t* output);

/* Composed operations - Functions built from other functions!
 * rolling_range = subtract(rolling_max, rolling_min)
 * rolling_std = compose with descriptive_stats
 */

void fp_rolling_range_f64(const double* data, size_t n, size_t window, double* output);
void fp_rolling_std_f64(const double* data, size_t n, size_t window, double* output);
void fp_rolling_variance_f64(const double* data, size_t n, size_t window, double* output);

/* Optimized specializations (for hot paths)
 * These use sliding window tricks: sum[i+1] = sum[i] - data[i] + data[i+window]
 * Complexity: O(1) per step instead of O(window)
 */

void fp_rolling_sum_f64_optimized(const double* data, size_t n, size_t window, double* output);
void fp_rolling_mean_f64_optimized(const double* data, size_t n, size_t window, double* output);

/* ============================================================================
 * Category 11: General Higher-Order Functions (100% FP Equivalence)
 * ============================================================================
 *
 * DESIGN PHILOSOPHY: Achieving TRUE Haskell/Lisp/ML Equivalence
 *
 * The specialized functions (fp_reduce_add, fp_map_abs, etc.) provide optimal
 * performance for 95% of use cases. However, to achieve THEORETICAL EQUIVALENCE
 * with functional programming languages, we need general higher-order functions
 * that accept arbitrary user-defined functions.
 *
 * C LIMITATION: Unlike Haskell/Lisp/ML, C lacks first-class functions and closures.
 * WORKAROUND: Use function pointers + context pointer (void*) for "poor man's closures"
 *
 * PERFORMANCE NOTE: General HOFs have function call overhead. For hot paths, use
 * specialized versions (e.g., fp_reduce_add_i64 instead of fp_foldl_i64 with add function).
 *
 * These 4 functions achieve 100% FP equivalence:
 *   1. foldl  - General reduction (replaces sum, product, max, min for custom logic)
 *   2. map    - General transformation (replaces abs, sqrt, scale for custom logic)
 *   3. filter - General selection (replaces filter_gt for custom predicates)
 *   4. zipWith - General combination (replaces zip_add for custom combiners)
 *
 * ============================================================================ */

/**
 * General left fold (foldl) - The foundation of functional programming
 *
 * Haskell: foldl :: (b -> a -> b) -> b -> [a] -> b
 *          foldl f init [x1, x2, ..., xn] = (...((init `f` x1) `f` x2) ...) `f` xn
 *
 * Example - Sum:
 *   int64_t sum_fn(int64_t acc, int64_t x, void* ctx) { return acc + x; }
 *   int64_t total = fp_foldl_i64(data, n, 0, sum_fn, NULL);
 *
 * Example - Custom logic with context:
 *   typedef struct { int threshold; } Ctx;
 *   int64_t count_gt(int64_t acc, int64_t x, void* ctx) {
 *       Ctx* c = (Ctx*)ctx;
 *       return acc + (x > c->threshold ? 1 : 0);
 *   }
 *   Ctx context = {.threshold = 10};
 *   int64_t count = fp_foldl_i64(data, n, 0, count_gt, &context);
 *
 * @param input Input array
 * @param n Array length
 * @param init Initial accumulator value
 * @param fn Folding function: (accumulator, element, context) -> new_accumulator
 * @param context User context pointer (NULL if not needed)
 * @return Final accumulated value
 *
 * Complexity: O(n) time, O(1) space
 * Performance: ~20-30% slower than specialized versions due to indirect calls
 */
int64_t fp_foldl_i64(const int64_t* input, size_t n, int64_t init,
                     int64_t (*fn)(int64_t acc, int64_t x, void* ctx),
                     void* context);

double fp_foldl_f64(const double* input, size_t n, double init,
                    double (*fn)(double acc, double x, void* ctx),
                    void* context);

/**
 * General map - Transform each element with arbitrary function
 *
 * Haskell: map :: (a -> b) -> [a] -> [b]
 *          map f [x1, x2, ..., xn] = [f x1, f x2, ..., f xn]
 *
 * Example - Double each element:
 *   int64_t double_fn(int64_t x, void* ctx) { return x * 2; }
 *   fp_map_i64(input, output, n, double_fn, NULL);
 *
 * Example - Custom transformation with context:
 *   typedef struct { int64_t multiplier; int64_t offset; } Ctx;
 *   int64_t transform(int64_t x, void* ctx) {
 *       Ctx* c = (Ctx*)ctx;
 *       return x * c->multiplier + c->offset;
 *   }
 *   Ctx context = {.multiplier = 3, .offset = 5};
 *   fp_map_i64(input, output, n, transform, &context);
 *
 * @param input Input array
 * @param output Output array (must be pre-allocated, size n)
 * @param n Array length
 * @param fn Transformation function: (element, context) -> transformed_element
 * @param context User context pointer (NULL if not needed)
 *
 * Complexity: O(n) time, O(1) space (excluding output)
 * Performance: ~20-30% slower than specialized versions due to indirect calls
 */
void fp_map_i64(const int64_t* input, int64_t* output, size_t n,
                int64_t (*fn)(int64_t x, void* ctx),
                void* context);

void fp_map_f64(const double* input, double* output, size_t n,
                double (*fn)(double x, void* ctx),
                void* context);

/**
 * General filter - Select elements matching arbitrary predicate
 *
 * Haskell: filter :: (a -> Bool) -> [a] -> [a]
 *          filter p xs = [x | x <- xs, p x]
 *
 * Example - Filter even numbers:
 *   bool is_even(int64_t x, void* ctx) { return x % 2 == 0; }
 *   size_t count = fp_filter_i64(input, output, n, is_even, NULL);
 *
 * Example - Filter with range (using context):
 *   typedef struct { int64_t min; int64_t max; } Range;
 *   bool in_range(int64_t x, void* ctx) {
 *       Range* r = (Range*)ctx;
 *       return x >= r->min && x <= r->max;
 *   }
 *   Range range = {.min = 10, .max = 100};
 *   size_t count = fp_filter_i64(input, output, n, in_range, &range);
 *
 * @param input Input array
 * @param output Output array (must be pre-allocated, size n)
 * @param n Input array length
 * @param predicate Predicate function: (element, context) -> bool
 * @param context User context pointer (NULL if not needed)
 * @return Number of elements written to output
 *
 * Complexity: O(n) time, O(1) space (excluding output)
 * Performance: ~20-30% slower than specialized versions due to indirect calls
 */
size_t fp_filter_i64(const int64_t* input, int64_t* output, size_t n,
                     bool (*predicate)(int64_t x, void* ctx),
                     void* context);

size_t fp_filter_f64(const double* input, double* output, size_t n,
                     bool (*predicate)(double x, void* ctx),
                     void* context);

/**
 * General zipWith - Combine two arrays with arbitrary function
 *
 * Haskell: zipWith :: (a -> b -> c) -> [a] -> [b] -> [c]
 *          zipWith f [x1,x2,...,xn] [y1,y2,...,yn] = [f x1 y1, f x2 y2, ..., f xn yn]
 *
 * Example - Element-wise maximum:
 *   int64_t max_fn(int64_t x, int64_t y, void* ctx) { return x > y ? x : y; }
 *   fp_zipWith_i64(a, b, output, n, max_fn, NULL);
 *
 * Example - Euclidean distance (with context):
 *   typedef struct { double scale; } Ctx;
 *   double scaled_diff(double x, double y, void* ctx) {
 *       Ctx* c = (Ctx*)ctx;
 *       double diff = x - y;
 *       return c->scale * diff * diff;
 *   }
 *   Ctx context = {.scale = 0.5};
 *   fp_zipWith_f64(a, b, output, n, scaled_diff, &context);
 *
 * @param input_a First input array
 * @param input_b Second input array
 * @param output Output array (must be pre-allocated, size n)
 * @param n Array length (must be same for both inputs)
 * @param fn Combining function: (element_a, element_b, context) -> combined_element
 * @param context User context pointer (NULL if not needed)
 *
 * Complexity: O(n) time, O(1) space (excluding output)
 * Performance: ~20-30% slower than specialized versions due to indirect calls
 */
void fp_zipWith_i64(const int64_t* input_a, const int64_t* input_b, int64_t* output, size_t n,
                    int64_t (*fn)(int64_t x, int64_t y, void* ctx),
                    void* context);

void fp_zipWith_f64(const double* input_a, const double* input_b, double* output, size_t n,
                    double (*fn)(double x, double y, void* ctx),
                    void* context);

#ifdef __cplusplus
}
#endif

