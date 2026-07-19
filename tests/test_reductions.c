/* Validates the reductions ASM kernels against scalar C references on the host
 * ABI. Exit code 0 = all pass. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

extern int64_t fp_reduce_add_i64(const int64_t *in, size_t n);
extern int64_t fp_reduce_max_i64(const int64_t *in, size_t n);
extern int64_t fp_reduce_min_i64(const int64_t *in, size_t n);
extern double  fp_reduce_add_f64(const double *in, size_t n);
extern double  fp_reduce_max_f64(const double *in, size_t n);
extern double  fp_reduce_min_f64(const double *in, size_t n);
extern double  fp_reduce_add_f64_where(const double *x, const int *mask, size_t n);

static int failures = 0;
#define CHECK_I64(expr, want) do { int64_t g=(expr); if (g!=(want)) { \
    printf("FAIL %-28s got=%lld want=%lld\n", #expr, (long long)g, (long long)(want)); failures++; } \
    else printf("ok   %-28s = %lld\n", #expr, (long long)g); } while(0)
#define CHECK_F64(expr, want) do { double g=(expr); if (fabs(g-(want))>1e-9*(1+fabs(want))) { \
    printf("FAIL %-28s got=%.10g want=%.10g\n", #expr, g, (double)(want)); failures++; } \
    else printf("ok   %-28s = %.10g\n", #expr, g); } while(0)

int main(void) {
    enum { N = 1000 };
    int64_t ai[N];
    double  ad[N];
    int     mask[N];
    int64_t si = 0, mxi = -9223372036854775807LL, mni = 9223372036854775807LL;
    double  sd = 0, mxd = -1e300, mnd = 1e300, sdw = 0;

    for (int i = 0; i < N; i++) {
        ai[i] = (int64_t)((i * 2654435761u) % 100000) - 50000;
        ad[i] = (double)ai[i] * 0.5 + 0.25;
        mask[i] = (i % 3 == 0);
        si += ai[i];
        if (ai[i] > mxi) mxi = ai[i];
        if (ai[i] < mni) mni = ai[i];
        sd += ad[i];
        if (ad[i] > mxd) mxd = ad[i];
        if (ad[i] < mnd) mnd = ad[i];
        if (mask[i]) sdw += ad[i];
    }

    CHECK_I64(fp_reduce_add_i64(ai, N), si);
    CHECK_I64(fp_reduce_max_i64(ai, N), mxi);
    CHECK_I64(fp_reduce_min_i64(ai, N), mni);
    CHECK_F64(fp_reduce_add_f64(ad, N), sd);
    CHECK_F64(fp_reduce_max_f64(ad, N), mxd);
    CHECK_F64(fp_reduce_min_f64(ad, N), mnd);
    CHECK_F64(fp_reduce_add_f64_where(ad, mask, N), sdw);

    printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASS",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
