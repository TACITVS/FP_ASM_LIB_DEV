/**
 * fp_functional.c — extended functional-programming operations (plain C).
 * See include/fp_functional.h for the API and rationale.
 */
#include "../../include/fp_functional.h"
#include <stdlib.h>   /* malloc/free for the sort temp buffer */

/* ============== Milestone B: ordering & grouping ================== */
#define SORT_BY(T, SUF) \
void fp_sort_by_##SUF(T* a, size_t n, int (*cmp)(T, T, void*), void* ctx) { \
    if (!a || !cmp || n < 2) return; \
    T* tmp = (T*)malloc(n * sizeof(T)); \
    if (!tmp) return; \
    for (size_t w = 1; w < n; w *= 2) { \
        for (size_t i = 0; i < n; i += 2*w) { \
            size_t lo = i, mid = (i+w < n) ? i+w : n, hi = (i+2*w < n) ? i+2*w : n; \
            size_t l = lo, r = mid, k = lo; \
            while (l < mid && r < hi) tmp[k++] = (cmp(a[r], a[l], ctx) < 0) ? a[r++] : a[l++]; /* stable: left on tie */ \
            while (l < mid) tmp[k++] = a[l++]; \
            while (r < hi)  tmp[k++] = a[r++]; \
            for (size_t j = lo; j < hi; j++) a[j] = tmp[j]; \
        } \
    } \
    free(tmp); }
SORT_BY(int64_t, i64)
SORT_BY(double,  f64)

#define GROUP_BY(T, SUF) \
size_t fp_group_by_##SUF(const T* in, size_t n, T* out_flat, size_t* out_lens, bool (*eq)(T, T, void*), void* ctx) { \
    if (!in || !out_flat || !out_lens || !eq || n == 0) return 0; \
    for (size_t i = 0; i < n; i++) out_flat[i] = in[i]; \
    size_t g = 0, run = 1; \
    for (size_t i = 1; i < n; i++) { if (eq(in[i-1], in[i], ctx)) run++; else { out_lens[g++] = run; run = 1; } } \
    out_lens[g++] = run; \
    return g; }
GROUP_BY(int64_t, i64)
GROUP_BY(double,  f64)

#define NUB_BY(T, SUF) \
size_t fp_nub_by_##SUF(const T* in, size_t n, T* out, bool (*eq)(T, T, void*), void* ctx) { \
    if (!in || !out || !eq) return 0; \
    size_t m = 0; \
    for (size_t i = 0; i < n; i++) { int dup = 0; \
        for (size_t j = 0; j < m; j++) if (eq(out[j], in[i], ctx)) { dup = 1; break; } \
        if (!dup) out[m++] = in[i]; } \
    return m; }
NUB_BY(int64_t, i64)
NUB_BY(double,  f64)

#define INTERSPERSE(T, SUF) \
size_t fp_intersperse_##SUF(const T* in, size_t n, T sep, T* out) { \
    if (!in || !out || n == 0) return 0; \
    size_t k = 0; \
    for (size_t i = 0; i < n; i++) { if (i > 0) out[k++] = sep; out[k++] = in[i]; } \
    return k; }
INTERSPERSE(int64_t, i64)
INTERSPERSE(double,  f64)

#define TRANSPOSE(T, SUF) \
size_t fp_transpose_##SUF(const T* in, size_t rows, size_t cols, T* out) { \
    if (!in || !out) return 0; \
    for (size_t r = 0; r < rows; r++) for (size_t c = 0; c < cols; c++) out[c*rows + r] = in[r*cols + c]; \
    return rows * cols; }
TRANSPOSE(int64_t, i64)
TRANSPOSE(double,  f64)


/* =================== folds/scans without a seed ==================== */
int64_t fp_foldl1_i64(const int64_t* in, size_t n, int64_t (*fn)(int64_t, int64_t, void*), void* ctx) {
    if (!in || !fn || n == 0) return 0;
    int64_t acc = in[0];
    for (size_t i = 1; i < n; i++) acc = fn(acc, in[i], ctx);
    return acc;
}
double fp_foldl1_f64(const double* in, size_t n, double (*fn)(double, double, void*), void* ctx) {
    if (!in || !fn || n == 0) return 0.0;
    double acc = in[0];
    for (size_t i = 1; i < n; i++) acc = fn(acc, in[i], ctx);
    return acc;
}
int64_t fp_foldr1_i64(const int64_t* in, size_t n, int64_t (*fn)(int64_t, int64_t, void*), void* ctx) {
    if (!in || !fn || n == 0) return 0;
    int64_t acc = in[n - 1];
    for (size_t i = n - 1; i-- > 0; ) acc = fn(in[i], acc, ctx);
    return acc;
}
double fp_foldr1_f64(const double* in, size_t n, double (*fn)(double, double, void*), void* ctx) {
    if (!in || !fn || n == 0) return 0.0;
    double acc = in[n - 1];
    for (size_t i = n - 1; i-- > 0; ) acc = fn(in[i], acc, ctx);
    return acc;
}
size_t fp_scanl1_i64(const int64_t* in, int64_t* out, size_t n, int64_t (*fn)(int64_t, int64_t, void*), void* ctx) {
    if (!in || !out || !fn || n == 0) return 0;
    int64_t acc = in[0]; out[0] = acc;
    for (size_t i = 1; i < n; i++) { acc = fn(acc, in[i], ctx); out[i] = acc; }
    return n;
}
size_t fp_scanl1_f64(const double* in, double* out, size_t n, double (*fn)(double, double, void*), void* ctx) {
    if (!in || !out || !fn || n == 0) return 0;
    double acc = in[0]; out[0] = acc;
    for (size_t i = 1; i < n; i++) { acc = fn(acc, in[i], ctx); out[i] = acc; }
    return n;
}
int64_t fp_mapAccumL_i64(const int64_t* in, int64_t* out, size_t n, int64_t acc0,
                         int64_t (*fn)(int64_t, int64_t, int64_t*, void*), void* ctx) {
    if (!in || !out || !fn) return acc0;
    int64_t acc = acc0;
    for (size_t i = 0; i < n; i++) acc = fn(acc, in[i], &out[i], ctx);
    return acc;
}
double fp_mapAccumL_f64(const double* in, double* out, size_t n, double acc0,
                        double (*fn)(double, double, double*, void*), void* ctx) {
    if (!in || !out || !fn) return acc0;
    double acc = acc0;
    for (size_t i = 0; i < n; i++) acc = fn(acc, in[i], &out[i], ctx);
    return acc;
}
size_t fp_zipWith3_i64(const int64_t* a, const int64_t* b, const int64_t* c, int64_t* out, size_t n,
                       int64_t (*fn)(int64_t, int64_t, int64_t, void*), void* ctx) {
    if (!a || !b || !c || !out || !fn) return 0;
    for (size_t i = 0; i < n; i++) out[i] = fn(a[i], b[i], c[i], ctx);
    return n;
}
size_t fp_zipWith3_f64(const double* a, const double* b, const double* c, double* out, size_t n,
                       double (*fn)(double, double, double, void*), void* ctx) {
    if (!a || !b || !c || !out || !fn) return 0;
    for (size_t i = 0; i < n; i++) out[i] = fn(a[i], b[i], c[i], ctx);
    return n;
}

/* ============================ right fold ============================ */
int64_t fp_foldr_i64(const int64_t* in, size_t n, int64_t init,
                     int64_t (*fn)(int64_t, int64_t, void*), void* ctx) {
    if (!in || !fn) return init;
    int64_t acc = init;
    for (size_t i = n; i-- > 0; ) acc = fn(in[i], acc, ctx);
    return acc;
}
double fp_foldr_f64(const double* in, size_t n, double init,
                    double (*fn)(double, double, void*), void* ctx) {
    if (!in || !fn) return init;
    double acc = init;
    for (size_t i = n; i-- > 0; ) acc = fn(in[i], acc, ctx);
    return acc;
}

/* ============================== scans ============================== */
size_t fp_scanl_i64(const int64_t* in, int64_t* out, size_t n, int64_t init,
                    int64_t (*fn)(int64_t, int64_t, void*), void* ctx) {
    if (!in || !out || !fn) return 0;
    int64_t acc = init;
    for (size_t i = 0; i < n; i++) { acc = fn(acc, in[i], ctx); out[i] = acc; }
    return n;
}
size_t fp_scanl_f64(const double* in, double* out, size_t n, double init,
                    double (*fn)(double, double, void*), void* ctx) {
    if (!in || !out || !fn) return 0;
    double acc = init;
    for (size_t i = 0; i < n; i++) { acc = fn(acc, in[i], ctx); out[i] = acc; }
    return n;
}
size_t fp_scanr_i64(const int64_t* in, int64_t* out, size_t n, int64_t init,
                    int64_t (*fn)(int64_t, int64_t, void*), void* ctx) {
    if (!in || !out || !fn) return 0;
    int64_t acc = init;
    for (size_t i = n; i-- > 0; ) { acc = fn(in[i], acc, ctx); out[i] = acc; }
    return n;
}
size_t fp_scanr_f64(const double* in, double* out, size_t n, double init,
                    double (*fn)(double, double, void*), void* ctx) {
    if (!in || !out || !fn) return 0;
    double acc = init;
    for (size_t i = n; i-- > 0; ) { acc = fn(in[i], acc, ctx); out[i] = acc; }
    return n;
}

/* ====================== predicate-based slicing ==================== */
#define TAKE_WHILE(T, SUF) \
size_t fp_take_while_##SUF(const T* in, T* out, size_t n, bool (*pred)(T, void*), void* ctx) { \
    if (!in || !out || !pred) return 0; \
    size_t k = 0; \
    while (k < n && pred(in[k], ctx)) { out[k] = in[k]; k++; } \
    return k; }
TAKE_WHILE(int64_t, i64)
TAKE_WHILE(double,  f64)

#define DROP_WHILE(T, SUF) \
size_t fp_drop_while_##SUF(const T* in, T* out, size_t n, bool (*pred)(T, void*), void* ctx) { \
    if (!in || !out || !pred) return 0; \
    size_t i = 0; \
    while (i < n && pred(in[i], ctx)) i++; \
    size_t k = 0; \
    for (; i < n; i++) out[k++] = in[i]; \
    return k; }
DROP_WHILE(int64_t, i64)
DROP_WHILE(double,  f64)

#define SPAN(T, SUF) \
size_t fp_span_##SUF(const T* in, T* take, T* drop, size_t n, bool (*pred)(T, void*), void* ctx) { \
    if (!in || !take || !drop || !pred) return 0; \
    size_t i = 0; \
    while (i < n && pred(in[i], ctx)) { take[i] = in[i]; i++; } \
    size_t split = i, k = 0; \
    for (; i < n; i++) drop[k++] = in[i]; \
    return split; }
SPAN(int64_t, i64)
SPAN(double,  f64)

/* ========================== partition ============================= */
#define PARTITION(T, SUF) \
size_t fp_partition_##SUF(const T* in, T* yes, T* no, size_t n, bool (*pred)(T, void*), void* ctx) { \
    if (!in || !yes || !no || !pred) return 0; \
    size_t ny = 0, nn = 0; \
    for (size_t i = 0; i < n; i++) { if (pred(in[i], ctx)) yes[ny++] = in[i]; else no[nn++] = in[i]; } \
    return ny; }
PARTITION(int64_t, i64)
PARTITION(double,  f64)

/* ========================= Foldable queries ======================= */
#define ALL(T, SUF) \
bool fp_all_##SUF(const T* in, size_t n, bool (*pred)(T, void*), void* ctx) { \
    if (!in || !pred) return true; \
    for (size_t i = 0; i < n; i++) if (!pred(in[i], ctx)) return false; \
    return true; }
ALL(int64_t, i64)
ALL(double,  f64)

#define ANY(T, SUF) \
bool fp_any_##SUF(const T* in, size_t n, bool (*pred)(T, void*), void* ctx) { \
    if (!in || !pred) return false; \
    for (size_t i = 0; i < n; i++) if (pred(in[i], ctx)) return true; \
    return false; }
ANY(int64_t, i64)
ANY(double,  f64)

#define COUNT_IF(T, SUF) \
size_t fp_count_if_##SUF(const T* in, size_t n, bool (*pred)(T, void*), void* ctx) { \
    if (!in || !pred) return 0; \
    size_t c = 0; \
    for (size_t i = 0; i < n; i++) if (pred(in[i], ctx)) c++; \
    return c; }
COUNT_IF(int64_t, i64)
COUNT_IF(double,  f64)

Maybe fp_find_i64(const int64_t* in, size_t n, bool (*pred)(int64_t, void*), void* ctx) {
    if (in && pred) for (size_t i = 0; i < n; i++) if (pred(in[i], ctx)) return fp_just_i64(in[i]);
    return fp_nothing();
}
Maybe fp_find_f64(const double* in, size_t n, bool (*pred)(double, void*), void* ctx) {
    if (in && pred) for (size_t i = 0; i < n; i++) if (pred(in[i], ctx)) return fp_just_f64(in[i]);
    return fp_nothing();
}

/* ===================== list monad bind (concatMap) ================= */
size_t fp_concat_map_i64(const int64_t* in, size_t n, int64_t* out, size_t out_cap,
                         int64_t* scratch, size_t scratch_cap,
                         size_t (*fn)(int64_t, int64_t*, size_t, void*), void* ctx) {
    if (!in || !out || !scratch || !fn) return 0;
    size_t total = 0;
    for (size_t i = 0; i < n; i++) {
        size_t k = fn(in[i], scratch, scratch_cap, ctx);
        if (k > scratch_cap) k = scratch_cap;            /* defensive clamp */
        if (total + k > out_cap) return (size_t)-1;      /* overflow signal */
        for (size_t j = 0; j < k; j++) out[total + j] = scratch[j];
        total += k;
    }
    return total;
}

/* ========================== generators ============================ */
size_t fp_iterate_i64(int64_t* out, size_t n, int64_t seed, int64_t (*fn)(int64_t, void*), void* ctx) {
    if (!out || !fn || n == 0) return 0;
    out[0] = seed;
    for (size_t i = 1; i < n; i++) out[i] = fn(out[i - 1], ctx);
    return n;
}
size_t fp_iterate_f64(double* out, size_t n, double seed, double (*fn)(double, void*), void* ctx) {
    if (!out || !fn || n == 0) return 0;
    out[0] = seed;
    for (size_t i = 1; i < n; i++) out[i] = fn(out[i - 1], ctx);
    return n;
}
size_t fp_unfoldr_i64(int64_t* out, size_t max, int64_t seed,
                      bool (*fn)(int64_t*, int64_t*, void*), void* ctx) {
    if (!out || !fn) return 0;
    int64_t state = seed, val; size_t c = 0;
    while (c < max && fn(&state, &val, ctx)) out[c++] = val;
    return c;
}
size_t fp_unfoldr_f64(double* out, size_t max, double seed,
                      bool (*fn)(double*, double*, void*), void* ctx) {
    if (!out || !fn) return 0;
    double state = seed, val; size_t c = 0;
    while (c < max && fn(&state, &val, ctx)) out[c++] = val;
    return c;
}

/* mapMaybe is provided by fp_monads.c (fp_map_maybe_i64 / fp_map_maybe_f64). */
