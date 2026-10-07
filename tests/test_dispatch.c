/* Validates every ISA variant of the dispatched kernels (AVX2, AVX-VNNI,
 * AVX-512) against scalar references, on every tier this CPU supports, for
 * many lengths (all tail sizes), plus:
 *   - the public symbols route to the selected variant, and
 *     fp_dispatch_set_max_tier() really switches it;
 *   - inputs that end exactly at an unmapped page (Linux) — the AVX-512
 *     masked tails must not touch memory past the end.
 * Tiers the CPU lacks are reported as SKIP, never called. */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE 200112L
#endif
#include "fp_core.h"
#include "fp_cpu.h"
#include "fp_dispatch.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#  include <sys/mman.h>
#  include <unistd.h>
#endif

static int failures = 0, checks = 0;

static uint32_t rng = 12345u;
static uint32_t next(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }

/* ---- references ---- */
static double ref_sum_f32(const float* a, size_t n, double* mag) {
    double s = 0, m = 0; size_t i;
    for (i = 0; i < n; i++) { s += a[i]; m += fabs(a[i]); }
    *mag = m; return s;
}
static double ref_sum_f64(const double* a, size_t n, double* mag) {
    double s = 0, m = 0; size_t i;
    for (i = 0; i < n; i++) { s += a[i]; m += fabs(a[i]); }
    *mag = m; return s;
}
static double ref_sq_f32(const float* a, size_t n, double* mag) {
    double s = 0; size_t i;
    for (i = 0; i < n; i++) s += (double)a[i] * a[i];
    *mag = s; return s;
}
static double ref_dot_f32(const float* a, const float* b, size_t n, double* mag) {
    double s = 0, m = 0; size_t i;
    for (i = 0; i < n; i++) { s += (double)a[i] * b[i]; m += fabs((double)a[i] * b[i]); }
    *mag = m; return s;
}
static double ref_dot_f64(const double* a, const double* b, size_t n, double* mag) {
    double s = 0, m = 0; size_t i;
    for (i = 0; i < n; i++) { s += a[i] * b[i]; m += fabs(a[i] * b[i]); }
    *mag = m; return s;
}
/* wrapping integer dot products (the library's contract) */
static uint32_t ref_dot_s8(const int8_t* a, const int8_t* b, size_t n) {
    uint32_t s = 0; size_t i; for (i = 0; i < n; i++) s += (uint32_t)((int32_t)a[i] * b[i]); return s;
}
static uint32_t ref_dot_u8(const uint8_t* a, const uint8_t* b, size_t n) {
    uint32_t s = 0; size_t i; for (i = 0; i < n; i++) s += (uint32_t)a[i] * b[i]; return s;
}
static uint32_t ref_dot_s16(const int16_t* a, const int16_t* b, size_t n) {
    uint32_t s = 0; size_t i; for (i = 0; i < n; i++) s += (uint32_t)((int32_t)a[i] * b[i]); return s;
}
static uint32_t ref_dot_u16(const uint16_t* a, const uint16_t* b, size_t n) {
    uint32_t s = 0; size_t i; for (i = 0; i < n; i++) s += (uint32_t)a[i] * b[i]; return s;
}

/* f32 kernels accumulate in float: allow reassociation error ~ n * eps * sum|terms| */
static void check_f(const char* what, fp_tier t, size_t n, double got, double want, double mag, double eps) {
    double tol = eps * (double)(n + 16) * (mag + 1e-30) + 1e-30;
    checks++;
    if (!(fabs(got - want) <= tol)) {
        printf("FAIL %-18s %-8s n=%-6zu got=%.9g want=%.9g tol=%.3g\n", what, fp_tier_name(t), n, got, want, tol);
        failures++;
    }
}
static void check_i(const char* what, fp_tier t, size_t n, long got, long want) {
    checks++;
    if (got != want) {
        printf("FAIL %-18s %-8s n=%-6zu got=%ld want=%ld\n", what, fp_tier_name(t), n, got, want);
        failures++;
    }
}

static int tier_ok(fp_tier t, int needs_vnni512) {
    if (!fp_dispatch_tier_available(t)) return 0;
    if (t == FP_TIER_AVX512 && needs_vnni512 && !fp_cpu_has(FP_CPU_AVX512_VNNI)) return 0;
    return 1;
}

/* Run every variant of the selected kernel families on buffers a/b.
 * n counts f32 / 8-bit elements; the f64 and 16-bit views use n/2. */
enum { F32 = 1, F64 = 2, INT = 4 };
static void run_all(const void* va, const void* vb, size_t n, int which) {
    int t;
    for (t = FP_TIER_AVX2; t < FP_TIER_COUNT; t++) {
        double mag, want;
        if ((which & F32) && t != FP_TIER_AVXVNNI && tier_ok((fp_tier)t, 0)) {
            const float* a = va; const float* b = vb;
            want = ref_sum_f32(a, n, &mag);
            check_f("reduce_add_f32", t, n, t == FP_TIER_AVX512 ? fp_reduce_add_f32_avx512(a, n) : fp_reduce_add_f32_avx2(a, n), want, mag, 1.2e-7);
            want = ref_sq_f32(a, n, &mag);
            check_f("fold_sumsq_f32", t, n, t == FP_TIER_AVX512 ? fp_fold_sumsq_f32_avx512(a, n) : fp_fold_sumsq_f32_avx2(a, n), want, mag, 1.2e-7);
            want = ref_dot_f32(a, b, n, &mag);
            check_f("fold_dotp_f32", t, n, t == FP_TIER_AVX512 ? fp_fold_dotp_f32_avx512(a, b, n) : fp_fold_dotp_f32_avx2(a, b, n), want, mag, 1.2e-7);
        }
        if ((which & F64) && t != FP_TIER_AVXVNNI && tier_ok((fp_tier)t, 0)) {
            const double* da = va; const double* db = vb;
            size_t nd = n / 2;
            want = ref_sum_f64(da, nd, &mag);
            check_f("reduce_add_f64", t, nd, t == FP_TIER_AVX512 ? fp_reduce_add_f64_avx512(da, nd) : fp_reduce_add_f64_avx2(da, nd), want, mag, 2.3e-16);
            want = ref_dot_f64(da, db, nd, &mag);
            check_f("fold_dotp_f64", t, nd, t == FP_TIER_AVX512 ? fp_fold_dotp_f64_avx512(da, db, nd) : fp_fold_dotp_f64_avx2(da, db, nd), want, mag, 2.3e-16);
        }
        if ((which & INT) && tier_ok((fp_tier)t, 1)) {
            const int8_t* s8a = va; const int8_t* s8b = vb;
            const int16_t* s16a = va; const int16_t* s16b = vb;
            size_t n16 = n / 2;
            int8_t r8; uint8_t ru8; int16_t r16; uint16_t ru16;
            switch (t) {
            case FP_TIER_AVX2:
                r8 = fp_fold_dotp_i8_avx2(s8a, s8b, n);   ru8 = fp_fold_dotp_u8_avx2(va, vb, n);
                r16 = fp_fold_dotp_i16_avx2(s16a, s16b, n16); ru16 = fp_fold_dotp_u16_avx2(va, vb, n16); break;
            case FP_TIER_AVXVNNI:
                r8 = fp_fold_dotp_i8_avxvnni(s8a, s8b, n);   ru8 = fp_fold_dotp_u8_avxvnni(va, vb, n);
                r16 = fp_fold_dotp_i16_avxvnni(s16a, s16b, n16); ru16 = fp_fold_dotp_u16_avxvnni(va, vb, n16); break;
            default:
                r8 = fp_fold_dotp_i8_avx512(s8a, s8b, n);   ru8 = fp_fold_dotp_u8_avx512(va, vb, n);
                r16 = fp_fold_dotp_i16_avx512(s16a, s16b, n16); ru16 = fp_fold_dotp_u16_avx512(va, vb, n16); break;
            }
            check_i("fold_dotp_i8",  t, n,   r8,   (int8_t)ref_dot_s8(s8a, s8b, n));
            check_i("fold_dotp_u8",  t, n,   ru8,  (uint8_t)ref_dot_u8(va, vb, n));
            check_i("fold_dotp_i16", t, n16, r16,  (int16_t)ref_dot_s16(s16a, s16b, n16));
            check_i("fold_dotp_u16", t, n16, ru16, (uint16_t)ref_dot_u16(va, vb, n16));
        }
    }
}

static void fill(void* p, size_t bytes, int as_float) {
    size_t i;
    if (as_float) {
        float* f = p;
        for (i = 0; i < bytes / 4; i++) f[i] = (float)((int)(next() % 2001) - 1000) * 0.001f;
    } else {
        unsigned char* c = p;
        for (i = 0; i < bytes; i++) c[i] = (unsigned char)next();
    }
}

int main(void) {
    enum { MAXN = 4096 };
    static float fa[MAXN + 16], fb[MAXN + 16];
    static unsigned char ia[MAXN * 2 + 64], ib[MAXN * 2 + 64];
    size_t n;
    int t;

    printf("CPU: %s (%s)\n", fp_cpu_info()->brand, fp_cpu_info()->uarch);
    for (t = 0; t < FP_TIER_COUNT; t++)
        printf("tier %-8s %s\n", fp_tier_name((fp_tier)t),
               fp_dispatch_tier_available((fp_tier)t) ? "TESTED" : "SKIP (not supported by this CPU)");

    /* every length 0..600 (all tails, all loop splits), then a few big ones;
     * odd offsets exercise unaligned loads */
    for (n = 0; n <= MAXN; n = n < 600 ? n + 1 : n * 2 + 7) {
        fill(fa, sizeof fa, 1); fill(fb, sizeof fb, 1);
        fill(ia, sizeof ia, 0); fill(ib, sizeof ib, 0);
        run_all(fa, fb, n, F32);
        run_all(ia + 1, ib + 3, n, INT);
    }
    /* doubles need double-valued data: reuse the float buffers as doubles */
    {
        static double da[MAXN], db[MAXN];
        for (n = 0; n < MAXN; n++) { da[n] = (double)((int)(next() % 2001) - 1000) * 1e-3; db[n] = (double)((int)(next() % 2001) - 1000) * 1e-3; }
        for (n = 0; n <= 2 * 600; n += 1) run_all(da, db, n, F64);
    }
    printf("variants: %d checks\n", checks);

    /* Public symbols follow the tier cap. */
    if (fp_dispatch_enabled()) {
        static const int8_t a8[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        fp_tier prev = fp_dispatch_set_max_tier(FP_TIER_AVX2);
        checks++;
        if (strcmp(fp_dispatch_selected("fp_fold_dotp_i8"), "avx2") != 0) { puts("FAIL cap avx2 not applied"); failures++; }
        checks++;
        if (fp_fold_dotp_i8(a8, a8, 10) != (int8_t)385) { puts("FAIL public dotp_i8 @avx2"); failures++; }
        fp_dispatch_set_max_tier(FP_TIER_AUTO);
        checks++;
        if (strcmp(fp_dispatch_selected("fp_fold_dotp_i8"), fp_tier_name(fp_dispatch_best_tier())) != 0 &&
            !(fp_dispatch_best_tier() == FP_TIER_AVX512 && !fp_cpu_has(FP_CPU_AVX512_VNNI))) {
            printf("FAIL auto selected %s, best %s\n", fp_dispatch_selected("fp_fold_dotp_i8"),
                   fp_tier_name(fp_dispatch_best_tier()));
            failures++;
        }
        checks++;
        if (fp_fold_dotp_i8(a8, a8, 10) != (int8_t)385) { puts("FAIL public dotp_i8 @auto"); failures++; }
        fp_dispatch_set_max_tier(prev);
        fp_dispatch_report(stdout);
    } else {
        puts("dispatch disabled in this build: public symbols are the AVX2 kernels");
    }

#if defined(__linux__)
    /* Buffers that end exactly at a PROT_NONE page: any over-read faults. */
    {
        long pg = sysconf(_SC_PAGESIZE);
        unsigned char* m = mmap(NULL, (size_t)pg * 4, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (m != MAP_FAILED && mprotect(m + pg, (size_t)pg, PROT_NONE) == 0 &&
            mprotect(m + 3 * pg, (size_t)pg, PROT_NONE) == 0) {
            unsigned char* enda = m + pg;        /* first guard page */
            unsigned char* endb = m + 3 * pg;    /* second guard page */
            size_t k;
            {   /* doubles in [-1,1]: valid as f64 and (pairwise) finite as f32 */
                double* d1 = (double*)m; double* d2 = (double*)(m + 2 * pg);
                for (k = 0; k < (size_t)pg / 8; k++) {
                    d1[k] = (double)((int)(next() % 2001) - 1000) * 1e-3;
                    d2[k] = (double)((int)(next() % 2001) - 1000) * 1e-3;
                }
            }
            for (k = 0; k < 300; k++) {
                size_t bytes = k * 8;            /* multiple of 8: valid for every element type */
                run_all(enda - bytes, endb - bytes, k * 2, F64);
                run_all(enda - bytes, endb - bytes, k * 8, INT);
            }
            {   /* and again with float data for the f32 kernels */
                float* f1 = (float*)m; float* f2 = (float*)(m + 2 * pg);
                for (k = 0; k < (size_t)pg / 4; k++) {
                    f1[k] = (float)((int)(next() % 2001) - 1000) * 1e-3f;
                    f2[k] = (float)((int)(next() % 2001) - 1000) * 1e-3f;
                }
                for (k = 0; k < 600; k++)
                    run_all(enda - 4 * k, endb - 4 * k, k, F32);
            }
            puts("guard-page tails: no out-of-bounds reads");
        }
        if (m != MAP_FAILED) munmap(m, (size_t)pg * 4);
    }
#endif

    printf("\n%s (%d checks, %d failures)\n", failures ? "SOME FAILED" : "ALL PASS", checks, failures);
    return failures != 0;
}
