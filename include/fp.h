#ifndef FP_H
#define FP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t (*fp_unary_i64)(int64_t);
typedef int64_t (*fp_binary_i64)(int64_t, int64_t);

/* Generic, callback-based kernels */
size_t  fp_map_i64   (const int64_t *in, int64_t *out, size_t n, fp_unary_i64 f);
int64_t fp_reduce_i64(const int64_t *a,  size_t n, int64_t init, fp_binary_i64 op);

/* Specialized fast paths (no callbacks) */
size_t  fp_map_square_i64(const int64_t *in, int64_t *out, size_t n);
int64_t fp_reduce_add_i64(const int64_t *a,  size_t n, int64_t init);

/* Fused fold (one pass): sum_{i}(in[i]^2) + init */
int64_t fp_foldmap_sumsq_i64(const int64_t *in, size_t n, int64_t init);

#ifdef __cplusplus
}
#endif
#endif
