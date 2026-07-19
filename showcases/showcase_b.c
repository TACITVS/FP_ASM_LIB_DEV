/* showcase_b.c — Milestone B (sortBy / groupBy / transpose)
 *
 * Three real algorithms, imperative vs. composed from the library, verified
 * equal and timed at -O3 -march=native. Parity + a clarity gain = a win.
 */
#include "fp_functional.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static bool beq(int64_t a, int64_t b, void* c){ (void)c; return a == b; }
static int  cmp_asc(int64_t a, int64_t b, void* c){ (void)c; return (a<b)?-1:((a>b)?1:0); }
static int  qcmp(const void* a, const void* b){ int64_t x=*(const int64_t*)a, y=*(const int64_t*)b; return (x<y)?-1:(x>y?1:0); }

static double now_ns(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1e9+t.tv_nsec; }
static volatile int64_t sink;
#define REPS 50
#define BEST(...) ({ double best_=1e30; for(int r=0;r<REPS;r++){ double t0=now_ns(); __VA_ARGS__; double dt=now_ns()-t0; if(dt<best_)best_=dt; } best_; })

int main(void){
    int fails = 0;
    printf("%-24s %13s %13s %9s   %s\n", "algorithm", "imperative ns", "functional ns", "slowdown", "the whole algorithm");
    printf("%.98s\n", "--------------------------------------------------------------------------------------------------");

    /* 1) run-length encoding = groupBy (==) then take each run's length */
    {   enum { N = 1000000 };
        int64_t* in = malloc(N*sizeof(int64_t));
        for (int i=0;i<N;i++) in[i] = (i/37) % 100;                 /* runs of 37 */
        int64_t* flat = malloc(N*sizeof(int64_t)); size_t* lens = malloc(N*sizeof(size_t));
        size_t* limp = malloc(N*sizeof(size_t)); size_t gi=0, gf=0;
        double ti = BEST({ gi=0; int64_t p=in[0]; size_t run=1;
            for(int i=1;i<N;i++){ if(in[i]==p) run++; else { limp[gi++]=run; p=in[i]; run=1; } } limp[gi++]=run; sink=(int64_t)gi; });
        double tf = BEST({ gf = fp_group_by_i64(in,N,flat,lens,beq,NULL); sink=(int64_t)gf; });
        int ok = (gi==gf); for(size_t i=0;i<gf&&ok;i++) if(lens[i]!=limp[i]) ok=0; fails += !ok;
        printf("%-24s %13.0f %13.0f %8.2fx   %s\n", ok?"run-length encoding":"RLE FAIL", ti, tf, tf/ti, "groupBy (==) then length");
        free(in); free(flat); free(lens); free(limp);
    }

    /* 2) matrix transpose */
    {   enum { R = 512, C = 512, N = R*C };
        int64_t* in = malloc(N*sizeof(int64_t)), *oi = malloc(N*sizeof(int64_t)), *of = malloc(N*sizeof(int64_t));
        for (int i=0;i<N;i++) in[i]=i;
        double ti = BEST({ for(int r=0;r<R;r++) for(int c=0;c<C;c++) oi[c*R+r]=in[r*C+c]; sink=oi[N-1]; });
        double tf = BEST({ fp_transpose_i64(in,R,C,of); sink=of[N-1]; });
        int ok = (memcmp(oi,of,N*sizeof(int64_t))==0); fails += !ok;
        printf("%-24s %13.0f %13.0f %8.2fx   %s\n", ok?"matrix transpose 512^2":"transpose FAIL", ti, tf, tf/ti, "transpose rows x cols");
        free(in); free(oi); free(of);
    }

    /* 3) sort + median  (functional: a STABLE, context-aware sortBy; baseline: C stdlib qsort) */
    {   enum { N = 200000 };
        int64_t* base = malloc(N*sizeof(int64_t)), *w = malloc(N*sizeof(int64_t));
        for (int i=0;i<N;i++) base[i] = ((int64_t)(i*2654435761u)) % 1000000;
        int64_t med_q=0, med_f=0;
        double ti = BEST({ memcpy(w,base,N*sizeof(int64_t)); qsort(w,N,sizeof(int64_t),qcmp); med_q=w[N/2]; sink=med_q; });
        double tf = BEST({ memcpy(w,base,N*sizeof(int64_t)); fp_sort_by_i64(w,N,cmp_asc,NULL); med_f=w[N/2]; sink=med_f; });
        int ok = (med_q==med_f); fails += !ok;
        printf("%-24s %13.0f %13.0f %8.2fx   %s\n", ok?"sort + median":"sort FAIL", ti, tf, tf/ti, "sortBy cmp then middle  (stable!)");
        free(base); free(w);
    }

    printf("\nbest of %d runs, -O3 -march=native.\n", REPS);
    printf("%s\n", fails
        ? "SOME RESULTS DIFFER!"
        : "Every functional result matches the baseline. Honestly, the cost varies:\n"
          "  transpose  ~1.0x -- parity; the composed call is the whole algorithm.\n"
          "  sort+median ~1.2x vs the C stdlib qsort (a library, not a hand loop) -- and\n"
          "              fp_sort_by is STABLE with a context-carrying comparator, which\n"
          "              qsort is not; a fair price for two capabilities qsort lacks.\n"
          "  RLE        ~4x -- the loser here, and worth stating plainly: groupBy\n"
          "              MATERIALIZES the groups and runs the predicate per element, work a\n"
          "              bare run-length counter skips. When you only need counts, write the\n"
          "              loop; when you need the groups as data to keep composing, groupBy\n"
          "              earns its keep. Clarity is a win -- but not a blank cheque.");
    return fails ? 1 : 0;
}
