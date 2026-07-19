/* showcase_e.c — Milestone E (windows / chunksOf / enumerate)
 *
 * Container/windowing ops materialize or traverse everything. That makes them
 * clear and composable but not always the fastest route — and this showcase is
 * blunt about when to reach for the O(n) shortcut the library also provides.
 */
#include "fp_functional.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern int64_t fp_reduce_add_i64(const int64_t*, size_t);   /* SIMD kernel */

/* mapAccumL callback: O(n) running window sum (the fast functional path) */
static int64_t roll(int64_t s, int64_t x, int64_t* out, void* c){ (void)c; *out = s; return s + x; }

static double now_ns(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1e9+t.tv_nsec; }
static volatile int64_t sink;
#define REPS 60
#define BEST(...) ({ double best_=1e30; for(int r=0;r<REPS;r++){ double t0=now_ns(); __VA_ARGS__; double dt=now_ns()-t0; if(dt<best_)best_=dt; } best_; })

int main(void){
    enum { N = 200000, K = 16 };
    int64_t *in = malloc(N*sizeof(int64_t));
    for (int i=0;i<N;i++) in[i] = (i*2654435761u) % 1000;
    size_t W = N - K + 1;
    int64_t *ref = malloc(W*sizeof(int64_t));
    int fails = 0;

    printf("%-30s %13s %13s %9s   %s\n", "algorithm", "imperative ns", "functional ns", "slowdown", "the whole algorithm");
    printf("%.104s\n", "--------------------------------------------------------------------------------------------------------");

    /* 1a) moving-window sum, naive functional = windows K then reduce each */
    {   int64_t* wflat = malloc(W*K*sizeof(int64_t)); int64_t* outn = malloc(W*sizeof(int64_t));
        double ti = BEST({ int64_t s=fp_reduce_add_i64(in,K); ref[0]=s;
            for(size_t i=0;i+K<(size_t)N;i++){ s += in[i+K]-in[i]; ref[i+1]=s; } sink=ref[0]; });
        double tf = BEST({ fp_windows_i64(in,N,K,wflat);
            for(size_t s=0;s<W;s++) outn[s]=fp_reduce_add_i64(wflat+s*K,K); sink=outn[0]; });
        int ok=1; for(size_t i=0;i<W;i++) if(outn[i]!=ref[i]){ok=0;break;} fails += !ok;
        printf("%-30s %13.0f %13.0f %8.2fx   %s\n", ok?"moving sum (naive: windows)":"moving sum FAIL", ti, tf, tf/ti, "windows K then map sum   (O(n*k))");
        free(wflat); free(outn);
    }
    /* 1b) same result, fast functional path = one mapAccumL running sum (O(n)) */
    {   int64_t* pre = malloc((N+1)*sizeof(int64_t)); int64_t* outf = malloc(W*sizeof(int64_t));
        double tf = BEST({ int64_t acc = fp_mapAccumL_i64(in,pre,N,0,roll,NULL); pre[N]=acc;   /* exclusive prefix sums */
            for(size_t i=0;i<W;i++) outf[i]=pre[i+K]-pre[i]; sink=outf[0]; });
        int ok=1; for(size_t i=0;i<W;i++) if(outf[i]!=ref[i]){ok=0;break;} fails += !ok;
        printf("%-30s %13s %13.0f %9s   %s\n", ok?"  ... fast path (same op set)":"fast path FAIL", "-", tf, "O(n)", "mapAccumL prefix sums, then diff");
        free(pre); free(outf);
    }

    /* 2) batch means: chunksOf 1000 then mean each */
    {   enum { CK = 1000 }; size_t NC = (N + CK - 1) / CK;
        int64_t* flat = malloc(N*sizeof(int64_t)); size_t* lens = malloc(NC*sizeof(size_t));
        int64_t* mi = malloc(NC*sizeof(int64_t)); int64_t* mf = malloc(NC*sizeof(int64_t));
        double ti = BEST({ size_t g=0; for(size_t off=0;off<(size_t)N;off+=CK){ size_t len=(off+CK<=(size_t)N)?CK:(N-off);
            mi[g++]=fp_reduce_add_i64(in+off,len)/(int64_t)len; } sink=mi[0]; });
        double tf = BEST({ size_t nc=fp_chunks_of_i64(in,N,CK,flat,lens); size_t off=0;
            for(size_t g=0;g<nc;g++){ mf[g]=fp_reduce_add_i64(flat+off,lens[g])/(int64_t)lens[g]; off+=lens[g]; } sink=mf[0]; });
        int ok=1; for(size_t g=0;g<NC;g++) if(mi[g]!=mf[g]){ok=0;break;} fails += !ok;
        printf("%-30s %13.0f %13.0f %8.2fx   %s\n", ok?"batch means (chunksOf)":"batch means FAIL", ti, tf, tf/ti, "chunksOf 1000 then map mean");
        free(flat); free(lens); free(mi); free(mf);
    }

    printf("\nN=%d int64, K=%d, best of %d runs, -O3 -march=native.\n", N, K, REPS);
    printf("%s\n", fails
        ? "SOME RESULTS DIFFER!"
        : "All results match. This milestone is a lesson in cost, told honestly (see the\n"
          "numbers above; ratios drift with cache state, the message doesn't):\n"
          "  moving sum (windows) -- many times slower: 'windows K then map sum' is the clearest\n"
          "     thing to write, but it MATERIALIZES every window, so it is O(n*k).\n"
          "  fast path (mapAccumL) -- same library, O(n): within a small factor of the hand loop\n"
          "     (two passes vs one) and roughly an ORDER OF MAGNITUDE faster than naive windows.\n"
          "  batch means (chunksOf) -- a few times slower, because chunksOf COPIES the whole\n"
          "     input; here you only needed the chunk lengths and could reduce slices in place.\n"
          "Takeaway: container ops materialize -- that is the price of turning a traversal into\n"
          "data. Reach for them when you need the pieces AS data to keep composing; when you're\n"
          "about to reduce immediately, work in place or use the incremental scan.");
    free(in); free(ref);
    return fails ? 1 : 0;
}
