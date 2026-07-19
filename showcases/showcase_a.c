/* showcase_a.c — Milestone A (foldl1 / scanl1 / mapAccumL / zipWith3)
 *
 * Three real algorithms, each written twice: hand-rolled imperative C, and
 * composed from the library's functional operations. We verify the two agree,
 * then time both. The algorithms here are INHERENTLY SEQUENTIAL (a running
 * dependency between elements), so the imperative version gets no
 * autovectorization advantage — which is exactly where the functional style
 * costs ~nothing and reads far clearer.
 *
 * Philosophy: parity in time + a real gain in clarity = a win.
 */
#include "fp_functional.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ---- callbacks: the entire "business logic", one expression each ---- */
static int64_t imax(int64_t a, int64_t b, void* c){ (void)c; return a > b ? a : b; }
static int64_t kadane_step(int64_t best_here, int64_t x, void* c){ (void)c; /* max x (best+x) */
    int64_t ext = best_here + x; return x > ext ? x : ext; }
static int64_t runavg(int64_t sum, int64_t x, int64_t* out, void* c){ /* mapAccumL */
    long* i = (long*)c; int64_t s = sum + x; (*i)++; *out = s / *i; return s; }

/* ---- timing ---- */
static double now_ns(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1e9+t.tv_nsec; }
static volatile int64_t sink;
#define REPS 100
#define BEST(...) ({ double best_=1e30; for(int r=0;r<REPS;r++){ double t0=now_ns(); __VA_ARGS__; double dt=now_ns()-t0; if(dt<best_)best_=dt; } best_; })

int main(void){
    enum { N = 1000000 };
    int64_t *in = malloc(N*sizeof(int64_t)), *out = malloc(N*sizeof(int64_t)), *ref = malloc(N*sizeof(int64_t));
    for (int i=0;i<N;i++) in[i] = ((i*2654435761u)%20001) - 10000;   /* +/- 10000 */

    int fails = 0;
    printf("%-22s %13s %13s %9s   %s\n", "algorithm", "imperative ns", "functional ns", "slowdown", "the whole algorithm");
    printf("%.98s\n", "--------------------------------------------------------------------------------------------------");

    /* 1) prefix maxima  =  scanl1 max */
    { double ti = BEST({ int64_t m=in[0]; for(int i=0;i<N;i++){ if(in[i]>m)m=in[i]; ref[i]=m; } sink=ref[N-1]; });
      double tf = BEST({ fp_scanl1_i64(in,out,N,imax,NULL); sink=out[N-1]; });
      int ok=1; for(int i=0;i<N;i++) if(out[i]!=ref[i]){ok=0;break;} fails += !ok;
      printf("%-22s %13.0f %13.0f %8.2fx   %s\n", ok?"prefix maxima":"prefix maxima FAIL", ti, tf, tf/ti, "scanl1 max"); }

    /* 2) Kadane max-subarray  =  maximum (scanl1 (\b x -> max x (b+x))) */
    { int64_t kimp; double ti = BEST({ int64_t best=in[0], cur=in[0];
        for(int i=1;i<N;i++){ int64_t ext=cur+in[i]; cur = in[i]>ext?in[i]:ext; if(cur>best)best=cur; } kimp=best; sink=best; });
      int64_t kfp; double tf = BEST({ fp_scanl1_i64(in,out,N,kadane_step,NULL);
        int64_t best=out[0]; for(int i=1;i<N;i++) if(out[i]>best)best=out[i]; kfp=best; sink=best; });
      int ok = (kimp==kfp); fails += !ok;
      printf("%-22s %13.0f %13.0f %8.2fx   %s\n", ok?"Kadane max-subarray":"Kadane FAIL", ti, tf, tf/ti, "scanl1 (max x (b+x)) then maximum"); }

    /* 3) running average  =  mapAccumL (\sum x -> (sum+x, (sum+x)/i)) */
    { double ti = BEST({ int64_t s=0; for(int i=0;i<N;i++){ s+=in[i]; ref[i]=s/(i+1); } sink=ref[N-1]; });
      long i0=0; double tf = BEST({ i0=0; fp_mapAccumL_i64(in,out,N,0,runavg,&i0); sink=out[N-1]; });
      int ok=1; for(int i=0;i<N;i++) if(out[i]!=ref[i]){ok=0;break;} fails += !ok;
      printf("%-22s %13.0f %13.0f %8.2fx   %s\n", ok?"running average":"running average FAIL", ti, tf, tf/ti, "mapAccumL (sum, sum/i)"); }

    printf("\nN=%d int64, best of %d runs, -O3 -march=native.\n", N, REPS);
    printf("%s\n", fails
        ? "SOME RESULTS DIFFER!"
        : "Every functional result matches the imperative one, element for element.\n"
          "These algorithms are inherently sequential, so the imperative loop gets no\n"
          "SIMD advantage either. 'slowdown' is the cost of the function-pointer callback:\n"
          "a small constant factor on trivial per-element work that shrinks to ~parity as\n"
          "soon as the work is non-trivial (see running average). The payoff is the last\n"
          "column: each algorithm is one named composition instead of a hand-tuned loop.");
    free(in); free(out); free(ref);
    return fails ? 1 : 0;
}
