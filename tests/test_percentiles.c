/* Validates percentiles: single (xmm2 float arg + rbx fix), batch (5-arg,
 * results on stack/r8), and quartiles. Batch and quartiles exercise the
 * internal fp_percentile_sorted_f64_impl calls (split-entry pattern). */
#include "fp_types.h"
#include <stddef.h>
#include <stdio.h>
#include <math.h>

extern double fp_percentile_sorted_f64(const double*, size_t, double);
extern void   fp_percentiles_sorted_f64(const double*, size_t, const double*, size_t, double*);
extern void   fp_quartiles_sorted_f64(const double*, size_t, Quartiles*);

static int fails = 0;
static void ok(const char* t, double g, double w){
    if (fabs(g-w) > 1e-6) { printf("FAIL %-12s got=%.4f want=%.4f\n", t, g, w); fails++; }
    else printf("ok   %-12s = %.4f\n", t, g);
}

int main(void){
    double d[10]; for (int i=0;i<10;i++) d[i]=i+1;   /* sorted 1..10 */

    ok("median", fp_percentile_sorted_f64(d,10,0.5), 5.5);
    ok("p0",     fp_percentile_sorted_f64(d,10,0.0), 1.0);
    ok("p100",   fp_percentile_sorted_f64(d,10,1.0), 10.0);
    ok("p25",    fp_percentile_sorted_f64(d,10,0.25), 3.25);

    double ps[3]={0.0,0.5,1.0}, res[3];
    fp_percentiles_sorted_f64(d,10,ps,3,res);   /* internal-call loop */
    ok("batch[0]", res[0], 1.0);
    ok("batch[1]", res[1], 5.5);
    ok("batch[2]", res[2], 10.0);

    Quartiles q; fp_quartiles_sorted_f64(d,10,&q);
    ok("q1",       q.q1, 3.25);
    ok("q.median", q.median, 5.5);
    ok("q3",       q.q3, 7.75);
    ok("iqr",      q.iqr, 4.5);

    printf("\n%s (%d failure%s)\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s");
    return fails?1:0;
}
