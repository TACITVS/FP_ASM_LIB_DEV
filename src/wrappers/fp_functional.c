/**
 * fp_functional.c — extended functional-programming operations (plain C).
 * See include/fp_functional.h for the API and rationale.
 */
#include "../../include/fp_functional.h"

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
