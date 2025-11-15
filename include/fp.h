#ifndef FP_H
#define FP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "fp_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Callback typedefs for general higher-order functions */
typedef int64_t (*fp_unary_i64)(int64_t value, void* context);
typedef int64_t (*fp_binary_i64)(int64_t acc, int64_t value, void* context);
typedef double  (*fp_unary_f64)(double value, void* context);
typedef double  (*fp_binary_f64)(double acc, double value, void* context);
typedef bool    (*fp_predicate_i64)(int64_t value, void* context);
typedef bool    (*fp_predicate_f64)(double value, void* context);

typedef int64_t (*fp_zip_i64)(int64_t lhs, int64_t rhs, void* context);
typedef double  (*fp_zip_f64)(double lhs, double rhs, void* context);

/* General higher-order functions with canonical naming */
int64_t fp_fold_left_i64(const int64_t* input, size_t n, int64_t init,
                         fp_binary_i64 fn,
                         void* context);

double  fp_fold_left_f64(const double* input, size_t n, double init,
                          fp_binary_f64 fn,
                          void* context);

void    fp_map_apply_i64(const int64_t* input, int64_t* output, size_t n,
                         fp_unary_i64 fn,
                         void* context);

void    fp_map_apply_f64(const double* input, double* output, size_t n,
                         fp_unary_f64 fn,
                         void* context);

size_t  fp_filter_predicate_i64(const int64_t* input, int64_t* output, size_t n,
                                fp_predicate_i64 predicate,
                                void* context);

size_t  fp_filter_predicate_f64(const double* input, double* output, size_t n,
                                fp_predicate_f64 predicate,
                                void* context);

void    fp_zip_apply_i64(const int64_t* input_a, const int64_t* input_b, int64_t* output,
                         size_t n,
                         fp_zip_i64 fn,
                         void* context);

void    fp_zip_apply_f64(const double* input_a, const double* input_b, double* output,
                         size_t n,
                         fp_zip_f64 fn,
                         void* context);

#ifdef __cplusplus
}
#endif
#endif
